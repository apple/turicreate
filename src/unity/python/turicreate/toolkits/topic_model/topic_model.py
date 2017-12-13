# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Methods for creating a topic model and predicting the topics of new documents.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import turicreate as _turicreate
from turicreate.toolkits._model import Model as _Model
from turicreate.data_structures.sframe import SFrame as _SFrame
from turicreate.data_structures.sarray import SArray as _SArray
from turicreate.toolkits.text_analytics._util import _check_input
from turicreate.toolkits.text_analytics._util import random_split as _random_split
from turicreate.toolkits._internal_utils import _check_categorical_option_type, \
                                            _map_unity_proxy_to_object, \
                                            _precomputed_field, \
                                            _toolkit_repr_print


import sys as _sys
if _sys.version_info.major == 3:
    _izip = zip
    _xrange = range
else:
    from itertools import izip as _izip
    _xrange = xrange

import operator as _operator
import array as _array

def create(dataset,
           num_topics=10,
           initial_topics=None,
           alpha=None,
           beta=.1,
           num_iterations=10,
           num_burnin=5,
           associations=None,
           verbose=False,
           print_interval=10,
           validation_set=None,
           method='auto'):
    """
    Create a topic model from the given data set. A topic model assumes each
    document is a mixture of a set of topics, where for each topic some words
    are more likely than others. One statistical approach to do this is called a
    "topic model". This method learns a topic model for the given document
    collection.

    Parameters
    ----------
    dataset : SArray of type dict or SFrame with a single column of type dict
        A bag of words representation of a document corpus.
        Each element is a dictionary representing a single document, where
        the keys are words and the values are the number of times that word
        occurs in that document.

    num_topics : int, optional
        The number of topics to learn.

    initial_topics : SFrame, optional
        An SFrame with a column of unique words representing the vocabulary
        and a column of dense vectors representing
        probability of that word given each topic. When provided,
        these values are used to initialize the algorithm.

    alpha : float, optional
        Hyperparameter that controls the diversity of topics in a document.
        Smaller values encourage fewer topics per document.
        Provided value must be positive. Default value is 50/num_topics.

    beta : float, optional
        Hyperparameter that controls the diversity of words in a topic.
        Smaller values encourage fewer words per topic. Provided value
        must be positive.

    num_iterations : int, optional
        The number of iterations to perform.

    num_burnin : int, optional
        The number of iterations to perform when inferring the topics for
        documents at prediction time.

    verbose : bool, optional
        When True, print most probable words for each topic while printing
        progress.

    print_interval : int, optional
        The number of iterations to wait between progress reports.

    associations : SFrame, optional
        An SFrame with two columns named "word" and "topic" containing words
        and the topic id that the word should be associated with. These words
        are not considered during learning.

    validation_set : SArray of type dict or SFrame with a single column
        A bag of words representation of a document corpus, similar to the
        format required for `dataset`. This will be used to monitor model
        performance during training. Each document in the provided validation
        set is randomly split: the first portion is used estimate which topic
        each document belongs to, and the second portion is used to estimate
        the model's performance at predicting the unseen words in the test data.

    method : {'cgs', 'alias'}, optional
        The algorithm used for learning the model.

        - *cgs:* Collapsed Gibbs sampling
        - *alias:* AliasLDA method.

    Returns
    -------
    out : TopicModel
        A fitted topic model. This can be used with
        :py:func:`~TopicModel.get_topics()` and
        :py:func:`~TopicModel.predict()`. While fitting is in progress, several
        metrics are shown, including:

        +------------------+---------------------------------------------------+
        |      Field       | Description                                       |
        +==================+===================================================+
        | Elapsed Time     | The number of elapsed seconds.                    |
        +------------------+---------------------------------------------------+
        | Tokens/second    | The number of unique words processed per second   |
        +------------------+---------------------------------------------------+
        | Est. Perplexity  | An estimate of the model's ability to model the   |
        |                  | training data. See the documentation on evaluate. |
        +------------------+---------------------------------------------------+

    See Also
    --------
    TopicModel, TopicModel.get_topics, TopicModel.predict,
    turicreate.SArray.dict_trim_by_keys, TopicModel.evaluate

    References
    ----------
    - `Wikipedia - Latent Dirichlet allocation
      <http://en.wikipedia.org/wiki/Latent_Dirichlet_allocation>`_

    - Alias method: Li, A. et al. (2014) `Reducing the Sampling Complexity of
      Topic Models. <http://www.sravi.org/pubs/fastlda-kdd2014.pdf>`_.
      KDD 2014.

    Examples
    --------
    The following example includes an SArray of documents, where
    each element represents a document in "bag of words" representation
    -- a dictionary with word keys and whose values are the number of times
    that word occurred in the document:

    >>> docs = turicreate.SArray('https://static.turi.com/datasets/nytimes')

    Once in this form, it is straightforward to learn a topic model.

    >>> m = turicreate.topic_model.create(docs)

    It is also easy to create a new topic model from an old one  -- whether
    it was created using Turi Create or another package.

    >>> m2 = turicreate.topic_model.create(docs, initial_topics=m['topics'])

    To manually fix several words to always be assigned to a topic, use
    the `associations` argument. The following will ensure that topic 0
    has the most probability for each of the provided words:

    >>> from turicreate import SFrame
    >>> associations = SFrame({'word':['hurricane', 'wind', 'storm'],
                               'topic': [0, 0, 0]})
    >>> m = turicreate.topic_model.create(docs,
                                        associations=associations)

    More advanced usage allows you  to control aspects of the model and the
    learning method.

    >>> import turicreate as tc
    >>> m = tc.topic_model.create(docs,
                                  num_topics=20,       # number of topics
                                  num_iterations=10,   # algorithm parameters
                                  alpha=.01, beta=.1)  # hyperparameters

    To evaluate the model's ability to generalize, we can create a train/test
    split where a portion of the words in each document are held out from
    training.

    >>> train, test = tc.text_analytics.random_split(.8)
    >>> m = tc.topic_model.create(train)
    >>> results = m.evaluate(test)
    >>> print results['perplexity']

    """
    dataset = _check_input(dataset)

    _check_categorical_option_type("method", method, ['auto', 'cgs', 'alias'])
    if method == 'cgs' or method == 'auto':
        model_name = 'cgs_topic_model'
    else:
        model_name = 'alias_topic_model'

    # If associations are provided, check they are in the proper format
    if associations is None:
        associations = _turicreate.SFrame({'word': [], 'topic': []})
    if isinstance(associations, _turicreate.SFrame) and \
       associations.num_rows() > 0:
        assert set(associations.column_names()) == set(['word', 'topic']), \
            "Provided associations must be an SFrame containing a word column\
             and a topic column."
        assert associations['word'].dtype == str, \
            "Words must be strings."
        assert associations['topic'].dtype == int, \
            "Topic ids must be of int type."
    if alpha is None:
        alpha = float(50) / num_topics

    if validation_set is not None:
        _check_input(validation_set)  # Must be a single column
        if isinstance(validation_set, _turicreate.SFrame):
            column_name = validation_set.column_names()[0]
            validation_set = validation_set[column_name]
        (validation_train, validation_test) = _random_split(validation_set)
    else:
        validation_train = _SArray()
        validation_test = _SArray()

    opts = {'model_name': model_name,
            'data': dataset,
            'num_topics': num_topics,
            'num_iterations': num_iterations,
            'print_interval': print_interval,
            'alpha': alpha,
            'beta': beta,
            'num_burnin': num_burnin,
            'associations': associations}

    # Initialize the model with basic parameters
    response = _turicreate.toolkits._main.run("text_topicmodel_init", opts)
    m = TopicModel(response['model'])

    # If initial_topics provided, load it into the model
    if isinstance(initial_topics, _turicreate.SFrame):
        assert set(['vocabulary', 'topic_probabilities']) ==              \
               set(initial_topics.column_names()),                        \
            "The provided initial_topics does not have the proper format, \
             e.g. wrong column names."
        observed_topics = initial_topics['topic_probabilities'].apply(lambda x: len(x))
        assert all(observed_topics == num_topics),                        \
            "Provided num_topics value does not match the number of provided initial_topics."

        # Rough estimate of total number of words
        weight = len(dataset) * 1000

        opts = {'model': m.__proxy__,
                'topics': initial_topics['topic_probabilities'],
                'vocabulary': initial_topics['vocabulary'],
                'weight': weight}
        response = _turicreate.toolkits._main.run("text_topicmodel_set_topics", opts)
        m = TopicModel(response['model'])

    # Train the model on the given data set and retrieve predictions
    opts = {'model': m.__proxy__,
            'data': dataset,
            'verbose': verbose,
            'validation_train': validation_train,
            'validation_test': validation_test}

    response = _turicreate.toolkits._main.run("text_topicmodel_train", opts)
    m = TopicModel(response['model'])

    return m


class TopicModel(_Model):
    """
    TopicModel objects can be used to predict the underlying topic of a
    document.

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.topic_model.create` to create an instance
    of this model. A detailed list of parameter options and code samples
    are available in the documentation for the create function.
    """

    def __init__(self, model_proxy):
        self.__proxy__ = model_proxy

    @classmethod
    def _native_name(cls):
        return ["cgs_topic_model", "alias_topic_model"]

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the model.
        """
        return self.__repr__()

    def _get_summary_struct(self):
        """
        Returns a structured description of the model, including (where relevant)
        the schema of the training data, description of the training data,
        training statistics, and model hyperparameters.

        Returns
        -------
        sections : list (of list of tuples)
            A list of summary sections.
              Each section is a list.
                Each item in a section list is a tuple of the form:
                  ('<label>','<field>')
        section_titles: list
            A list of section titles.
              The order matches that of the 'sections' object.
        """
        section_titles=['Schema','Settings']

        vocab_length = len(self.vocabulary)
        verbose = self.verbose == 1

        sections=[
                    [
                        ('Vocabulary Size',_precomputed_field(vocab_length))
                    ],
                    [
                        ('Number of Topics', 'num_topics'),
                        ('alpha','alpha'),
                        ('beta','beta'),
                        ('Iterations', 'num_iterations'),
                        ('Training time', 'training_time'),
                        ('Verbose', _precomputed_field(verbose))
                    ]
                ]
        return (sections, section_titles)

    def __repr__(self):
        """
        Print a string description of the model when the model name is entered
        in the terminal.
        """
        key_str = "{:<{}}: {}"
        width = 30

        (sections, section_titles) = self._get_summary_struct()
        out = _toolkit_repr_print(self, sections, section_titles, width=width)

        extra = []
        extra.append(key_str.format("Accessible fields", width, ""))
        extra.append(key_str.format("m.topics",width,"An SFrame containing the topics."))
        extra.append(key_str.format("m.vocabulary",width,"An SArray containing the words in the vocabulary."))
        extra.append(key_str.format("Useful methods", width, ""))
        extra.append(key_str.format("m.get_topics()",width,"Get the most probable words per topic."))
        extra.append(key_str.format("m.predict(new_docs)",width,"Make predictions for new documents."))

        return out + '\n' + '\n'.join(extra)


    def _get(self, field):
        """
        Return the value of a given field. The list of all queryable fields is
        detailed below, and can be obtained with the
        :py:func:`~TopicModel._list_fields` method.

        +-----------------------+----------------------------------------------+
        |      Field            | Description                                  |
        +=======================+==============================================+
        | topics                | An SFrame containing a column with the unique|
        |                       | words observed during training, and a column |
        |                       | of arrays containing the probability values  |
        |                       | for each word given each of the topics.      |
        +-----------------------+----------------------------------------------+
        | vocabulary            | An SArray containing the words used. This is |
        |                       | same as the vocabulary column in the topics  |
        |                       | field above.                                 |
        +-----------------------+----------------------------------------------+

        Parameters
        ----------
        field : string
            Name of the field to be retrieved.

        Returns
        -------
        out
            Value of the requested field.
        """

        opts = {'model': self.__proxy__, 'field': field}
        response = _turicreate.toolkits._main.run("text_topicmodel_get_value", opts)
        if field == 'vocabulary':
            return _SArray(None, _proxy=response['value'])
        elif field == 'topics':
            return _SFrame(None, _proxy=response['value'])
        return response['value']

    def _training_stats(self):
        """
        Return a dictionary of statistics collected during creation of the
        model. These statistics are also available with the ``get`` method and
        are described in more detail in that method's documentation.

        Returns
        -------
        out : dict
            Dictionary of statistics compiled during creation of the
            TopicModel.

        See Also
        --------
        summary

        Examples
        --------
        >>> docs = turicreate.SArray('https://static.turi.com/datasets/nips-text')
        >>> m = turicreate.topic_model.create(docs)
        >>> m._training_stats()
        {'training_iterations': 20,
         'training_time': 20.5034}
        """

        fields = self._list_fields()
        stat_fields = ['training_time',
                       'training_iterations']
        if 'validation_perplexity' in fields:
            stat_fields.append('validation_perplexity')

        ret = {k : self._get(k) for k in stat_fields}
        return ret

    def get_topics(self, topic_ids=None, num_words=5, cdf_cutoff=1.0,
                   output_type='topic_probabilities'):

        """
        Get the words associated with a given topic. The score column is the
        probability of choosing that word given that you have chosen a
        particular topic.

        Parameters
        ----------
        topic_ids : list of int, optional
            The topics to retrieve words. Topic ids are zero-based.
            Throws an error if greater than or equal to m['num_topics'], or
            if the requested topic name is not present.

        num_words : int, optional
            The number of words to show.

        cdf_cutoff : float, optional
            Allows one to only show the most probable words whose cumulative
            probability is below this cutoff. For example if there exist
            three words where

            .. math::
               p(word_1 | topic_k) = .1

               p(word_2 | topic_k) = .2

               p(word_3 | topic_k) = .05

            then setting :math:`cdf_{cutoff}=.3` would return only
            :math:`word_1` and :math:`word_2` since
            :math:`p(word_1 | topic_k) + p(word_2 | topic_k) <= cdf_{cutoff}`

        output_type : {'topic_probabilities' | 'topic_words'}, optional
            Determine the type of desired output. See below.

        Returns
        -------
        out : SFrame
            If output_type is 'topic_probabilities', then the returned value is
            an SFrame with a column of words ranked by a column of scores for
            each topic. Otherwise, the returned value is a SArray where
            each element is a list of the most probable words for each topic.

        Examples
        --------
        Get the highest ranked words for all topics.

        >>> docs = turicreate.SArray('https://static.turi.com/datasets/nips-text')
        >>> m = turicreate.topic_model.create(docs,
                                            num_iterations=50)
        >>> m.get_topics()
        +-------+----------+-----------------+
        | topic |   word   |      score      |
        +-------+----------+-----------------+
        |   0   |   cell   |  0.028974400831 |
        |   0   |  input   | 0.0259470208503 |
        |   0   |  image   | 0.0215721599763 |
        |   0   |  visual  | 0.0173635081992 |
        |   0   |  object  | 0.0172447874156 |
        |   1   | function | 0.0482834508265 |
        |   1   |  input   | 0.0456270024091 |
        |   1   |  point   | 0.0302662839454 |
        |   1   |  result  | 0.0239474934631 |
        |   1   | problem  | 0.0231750116011 |
        |  ...  |   ...    |       ...       |
        +-------+----------+-----------------+

        Get the highest ranked words for topics 0 and 1 and show 15 words per
        topic.

        >>> m.get_topics([0, 1], num_words=15)
        +-------+----------+------------------+
        | topic |   word   |      score       |
        +-------+----------+------------------+
        |   0   |   cell   |  0.028974400831  |
        |   0   |  input   | 0.0259470208503  |
        |   0   |  image   | 0.0215721599763  |
        |   0   |  visual  | 0.0173635081992  |
        |   0   |  object  | 0.0172447874156  |
        |   0   | response | 0.0139740298286  |
        |   0   |  layer   | 0.0122585145062  |
        |   0   | features | 0.0115343177265  |
        |   0   | feature  | 0.0103530459301  |
        |   0   | spatial  | 0.00823387994361 |
        |  ...  |   ...    |       ...        |
        +-------+----------+------------------+

        If one wants to instead just get the top words per topic, one may
        change the format of the output as follows.

        >>> topics = m.get_topics(output_type='topic_words')
        dtype: list
        Rows: 10
        [['cell', 'image', 'input', 'object', 'visual'],
         ['algorithm', 'data', 'learning', 'method', 'set'],
         ['function', 'input', 'point', 'problem', 'result'],
         ['model', 'output', 'pattern', 'set', 'unit'],
         ['action', 'learning', 'net', 'problem', 'system'],
         ['error', 'function', 'network', 'parameter', 'weight'],
         ['information', 'level', 'neural', 'threshold', 'weight'],
         ['control', 'field', 'model', 'network', 'neuron'],
         ['hidden', 'layer', 'system', 'training', 'vector'],
         ['component', 'distribution', 'local', 'model', 'optimal']]
        """
        _check_categorical_option_type('output_type', output_type,
            ['topic_probabilities', 'topic_words'])

        if topic_ids is None:
            topic_ids = list(range(self._get('num_topics')))

        assert isinstance(topic_ids, list), \
            "The provided topic_ids is not a list."

        if any([type(x) == str for x in topic_ids]):
            raise ValueError("Only integer topic_ids can be used at this point in time.")
        if not all([x >= 0 and x < self.num_topics for x in topic_ids]):
            raise ValueError("Topic id values must be non-negative and less than the " + \
                "number of topics used to fit the model.")

        opts = {'model': self.__proxy__,
                'topic_ids': topic_ids,
                'num_words': num_words,
                'cdf_cutoff': cdf_cutoff}
        response = _turicreate.toolkits._main.run('text_topicmodel_get_topic',
                                               opts)
        ret = _map_unity_proxy_to_object(response['top_words'])

        def sort_wordlist_by_prob(z):
            words = sorted(z.items(), key=_operator.itemgetter(1), reverse=True)
            return [word for (word, prob) in words]

        if output_type != 'topic_probabilities':
            ret = ret.groupby('topic',
                    {'word': _turicreate.aggregate.CONCAT('word', 'score')})
            words = ret.sort('topic')['word'].apply(sort_wordlist_by_prob)
            ret = _SFrame({'words': words})

        return ret

    def predict(self, dataset, output_type='assignment', num_burnin=None):
        """
        Use the model to predict topics for each document. The provided
        `dataset` should be an SArray object where each element is a dict
        representing a single document in bag-of-words format, where keys
        are words and values are their corresponding counts. If `dataset` is
        an SFrame, then it must contain a single column of dict type.

        The current implementation will make inferences about each document
        given its estimates of the topics learned when creating the model.
        This is done via Gibbs sampling.

        Parameters
        ----------
        dataset : SArray, SFrame of type dict
            A set of documents to use for making predictions.

        output_type : str, optional
            The type of output desired. This can either be

            - assignment: the returned values are integers in [0, num_topics)
            - probability: each returned prediction is a vector with length
              num_topics, where element k represents the probability that
              document belongs to topic k.

        num_burnin : int, optional
            The number of iterations of Gibbs sampling to perform when
            inferring the topics for documents at prediction time.
            If provided this will override the burnin value set during
            training.

        Returns
        -------
        out : SArray

        See Also
        --------
        evaluate

        Examples
        --------
        Make predictions about which topic each document belongs to.

        >>> docs = turicreate.SArray('https://static.turi.com/datasets/nips-text')
        >>> m = turicreate.topic_model.create(docs)
        >>> pred = m.predict(docs)

        If one is interested in the probability of each topic

        >>> pred = m.predict(docs, output_type='probability')

        Notes
        -----
        For each unique word w in a document d, we sample an assignment to
        topic k with probability proportional to

        .. math::
            p(z_{dw} = k) \propto (n_{d,k} + \\alpha) * \Phi_{w,k}

        where

        - :math:`W` is the size of the vocabulary,
        - :math:`n_{d,k}` is the number of other times we have assigned a word in
          document to d to topic :math:`k`,
        - :math:`\Phi_{w,k}` is the probability under the model of choosing word
          :math:`w` given the word is of topic :math:`k`. This is the matrix
          returned by calling `m['topics']`.

        This represents a collapsed Gibbs sampler for the document assignments
        while we keep the topics learned during training fixed.
        This process is done in parallel across all documents, five times per
        document.

        """
        dataset = _check_input(dataset)

        if num_burnin is None:
            num_burnin = self.num_burnin

        opts = {'model': self.__proxy__,
                'data': dataset,
                'num_burnin': num_burnin}
        response = _turicreate.toolkits._main.run("text_topicmodel_predict", opts)
        preds = _SArray(None, _proxy=response['predictions'])

        # Get most likely topic if probabilities are not requested
        if output_type not in ['probability', 'probabilities', 'prob']:
            # equivalent to numpy.argmax(x)
            preds = preds.apply(lambda x: max(_izip(x, _xrange(len(x))))[1])

        return preds

    
    def evaluate(self, train_data, test_data=None, metric='perplexity'):
        """
        Estimate the model's ability to predict new data. Imagine you have a
        corpus of books. One common approach to evaluating topic models is to
        train on the first half of all of the books and see how well the model
        predicts the second half of each book.

        This method returns a metric called perplexity, which  is related to the
        likelihood of observing these words under the given model. See
        :py:func:`~turicreate.topic_model.perplexity` for more details.

        The provided `train_data` and `test_data` must have the same length,
        i.e., both data sets must have the same number of documents; the model
        will use train_data to estimate which topic the document belongs to, and
        this is used to estimate the model's performance at predicting the
        unseen words in the test data.

        See :py:func:`~turicreate.topic_model.TopicModel.predict` for details
        on how these predictions are made, and see
        :py:func:`~turicreate.text_analytics.random_split` for a helper function
        that can be used for making train/test splits.

        Parameters
        ----------
        train_data : SArray or SFrame
            A set of documents to predict topics for.

        test_data : SArray or SFrame, optional
            A set of documents to evaluate performance on.
            By default this will set to be the same as train_data.

        metric : str
            The chosen metric to use for evaluating the topic model.
            Currently only 'perplexity' is supported.

        Returns
        -------
        out : dict
            The set of estimated evaluation metrics.

        See Also
        --------
        predict, turicreate.toolkits.text_analytics.random_split

        Examples
        --------
        >>> docs = turicreate.SArray('https://static.turi.com/datasets/nips-text')
        >>> train_data, test_data = turicreate.text_analytics.random_split(docs)
        >>> m = turicreate.topic_model.create(train_data)
        >>> m.evaluate(train_data, test_data)
        {'perplexity': 2467.530370396021}

        """
        train_data = _check_input(train_data)

        if test_data is None:
            test_data = train_data
        else:
            test_data = _check_input(test_data)

        predictions = self.predict(train_data, output_type='probability')
        topics = self.topics

        ret = {}
        ret['perplexity'] = perplexity(test_data,
                                       predictions,
                                       topics['topic_probabilities'],
                                       topics['vocabulary'])
        return ret

    @classmethod
    def _get_queryable_methods(cls):
        '''Returns a list of method names that are queryable through Predictive
        Service'''
        return {'predict':{'dataset':'sarray'}}


def perplexity(test_data, predictions, topics, vocabulary):
    """
    Compute the perplexity of a set of test documents given a set
    of predicted topics.

    Let theta be the matrix of document-topic probabilities, where
    theta_ik = p(topic k | document i). Let Phi be the matrix of term-topic
    probabilities, where phi_jk = p(word j | topic k).

    Then for each word in each document, we compute for a given word w
    and document d

    .. math::
        p(word | \theta[doc_id,:], \phi[word_id,:]) =
       \sum_k \theta[doc_id, k] * \phi[word_id, k]

    We compute loglikelihood to be:

    .. math::
        l(D) = \sum_{i \in D} \sum_{j in D_i} count_{i,j} * log Pr(word_{i,j} | \theta, \phi)

    and perplexity to be

    .. math::
        \exp \{ - l(D) / \sum_i \sum_j count_{i,j} \}

    Parameters
    ----------
    test_data : SArray of type dict or SFrame with a single column of type dict
        Documents in bag-of-words format.

    predictions : SArray
        An SArray of vector type, where each vector contains estimates of the
        probability that this document belongs to each of the topics.
        This must have the same size as test_data; otherwise an exception
        occurs. This can be the output of
        :py:func:`~turicreate.topic_model.TopicModel.predict`, for example.

    topics : SFrame
        An SFrame containing two columns: 'vocabulary' and 'topic_probabilities'.
        The value returned by m['topics'] is a valid input for this argument,
        where m is a trained :py:class:`~turicreate.topic_model.TopicModel`.

    vocabulary : SArray
        An SArray of words to use. All words in test_data that are not in this
        vocabulary will be ignored.

    Notes
    -----
    For more details, see equations 13-16 of [PattersonTeh2013].


    References
    ----------
    .. [PERP] `Wikipedia - perplexity <http://en.wikipedia.org/wiki/Perplexity>`_

    .. [PattersonTeh2013] Patterson, Teh. `"Stochastic Gradient Riemannian
       Langevin Dynamics on the Probability Simplex"
       <http://www.stats.ox.ac.uk/~teh/research/compstats/PatTeh2013a.pdf>`_
       NIPS, 2013.

    Examples
    --------
    >>> from turicreate import topic_model
    >>> train_data, test_data = turicreate.text_analytics.random_split(docs)
    >>> m = topic_model.create(train_data)
    >>> pred = m.predict(train_data)
    >>> topics = m['topics']
    >>> p = topic_model.perplexity(test_data, pred,
                                   topics['topic_probabilities'],
                                   topics['vocabulary'])
    >>> p
    1720.7  # lower values are better
    """
    test_data = _check_input(test_data)
    assert isinstance(predictions, _SArray), \
        "Predictions must be an SArray of vector type."
    assert predictions.dtype == _array.array, \
        "Predictions must be probabilities. Try using m.predict() with " + \
        "output_type='probability'."

    opts = {'test_data': test_data,
            'predictions': predictions,
            'topics': topics,
            'vocabulary': vocabulary}
    response = _turicreate.toolkits._main.run("text_topicmodel_get_perplexity",
                                           opts)
    return response['perplexity']
