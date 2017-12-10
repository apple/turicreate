# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Methods for creating and querying a nearest neighbors model.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import turicreate as _turicreate
from turicreate.toolkits._model import Model as _Model
from turicreate.data_structures.sframe import SFrame as _SFrame
from turicreate.data_structures.sgraph import SGraph as _SGraph
import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits._private_utils import _validate_row_label
from turicreate.toolkits._private_utils import _validate_lists
from turicreate.toolkits._private_utils import _robust_column_name
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._private_utils import _validate_lists

from turicreate.toolkits.distances._util import _convert_distance_names_to_functions
from turicreate.toolkits.distances._util import _validate_composite_distance
from turicreate.toolkits.distances._util import _scrub_composite_distance_features
from turicreate.toolkits.distances._util import _get_composite_distance_features

import array
import copy as _copy
import six as _six

def _construct_auto_distance(feature_names, column_names, column_types, sample):
    """
    Construct composite distance parameters based on selected features and their
    types.
    """

    ## Make a dictionary from the column_names and column_types
    col_type_dict = {k: v for k, v in zip(column_names, column_types)}

    ## Loop through feature names, appending a distance component if the
    #  feature's type is *not* numeric. If the type *is* numeric, append it to
    #  the numeric_cols list, then at the end make a numeric columns distance
    #  component.
    composite_distance_params = []
    numeric_cols = []

    for c in feature_names:
        if col_type_dict[c] == str:
            composite_distance_params.append([[c], _turicreate.distances.levenshtein, 1])
        elif col_type_dict[c] == dict:
            composite_distance_params.append([[c], _turicreate.distances.jaccard, 1])
        elif col_type_dict[c] == array.array:
            composite_distance_params.append([[c], _turicreate.distances.euclidean, 1])
        elif col_type_dict[c] == list:
            only_str_lists = _validate_lists(sample[c], allowed_types=[str])
            if not only_str_lists:
                raise TypeError("Only lists of all str objects are currently supported")
            composite_distance_params.append([[c], _turicreate.distances.jaccard, 1])
        elif col_type_dict[c] in [int, float, array.array, list]:
            numeric_cols.append(c)
        else:
            raise TypeError("Unable to automatically determine a distance "+\
                "for column {}".format(c))

    # Make the standalone numeric column distance component
    if len(numeric_cols) > 0:
        composite_distance_params.append([numeric_cols, _turicreate.distances.euclidean, 1])

    return composite_distance_params


def create(dataset, label=None, features=None, distance=None, method='auto',
           verbose=True, **kwargs):
    """
    Create a nearest neighbor model, which can be searched efficiently and
    quickly for the nearest neighbors of a query observation. If the `method`
    argument is specified as `auto`, the type of model is chosen automatically
    based on the type of data in `dataset`.

    .. warning::

        The 'dot_product' distance is deprecated and will be removed in future
        versions of Turi Create. Please use 'transformed_dot_product'
        distance instead, although note that this is more than a name change;
        it is a *different* transformation of the dot product of two vectors.
        Please see the distances module documentation for more details.

    Parameters
    ----------
    dataset : SFrame
        Reference data. If the features for each observation are numeric, they
        may be in separate columns of 'dataset' or a single column with lists
        of values. The features may also be in the form of a column of sparse
        vectors (i.e. dictionaries), with string keys and numeric values.

    label : string, optional
        Name of the SFrame column with row labels. If 'label' is not specified,
        row numbers are used to identify reference dataset rows when the model
        is queried.

    features : list[string], optional
        Name of the columns with features to use in computing distances between
        observations and the query points. 'None' (the default) indicates that
        all columns except the label should be used as features. Each column
        can be one of the following types:

        - *Numeric*: values of numeric type integer or float.

        - *Array*: list of numeric (integer or float) values. Each list element
          is treated as a separate variable in the model.

        - *Dictionary*: key-value pairs with numeric (integer or float) values.
          Each key indicates a separate variable in the model.

        - *List*: list of integer or string values. Each element is treated as
          a separate variable in the model.

        - *String*: string values.

        Please note: if a composite distance is also specified, this parameter
        is ignored.

    distance : string, function, or list[list], optional
        Function to measure the distance between any two input data rows. This
        may be one of three types:

        - *String*: the name of a standard distance function. One of
          'euclidean', 'squared_euclidean', 'manhattan', 'levenshtein',
          'jaccard', 'weighted_jaccard', 'cosine', 'dot_product' (deprecated),
          or 'transformed_dot_product'.

        - *Function*: a function handle from the
          :mod:`~turicreate.toolkits.distances` module.

        - *Composite distance*: the weighted sum of several standard distance
          functions applied to various features. This is specified as a list of
          distance components, each of which is itself a list containing three
          items:

          1. list or tuple of feature names (strings)

          2. standard distance name (string)

          3. scaling factor (int or float)

        For more information about Turi Create distance functions, please
        see the :py:mod:`~turicreate.toolkits.distances` module.

        If 'distance' is left unspecified or set to 'auto', a composite
        distance is constructed automatically based on feature types.

    method : {'auto', 'ball_tree', 'brute_force', 'lsh'}, optional
        Method for computing nearest neighbors. The options are:

        - *auto* (default): the method is chosen automatically, based on the
          type of data and the distance. If the distance is 'manhattan' or
          'euclidean' and the features are numeric or vectors of numeric
          values, then the 'ball_tree' method is used. Otherwise, the
          'brute_force' method is used.

        - *ball_tree*: use a tree structure to find the k-closest neighbors to
          each query point. The ball tree model is slower to construct than the
          brute force model, but queries are faster than linear time. This
          method is not applicable for the cosine and dot product distances.
          See `Liu, et al (2004)
          <http://papers.nips.cc/paper/2666-an-investigation-of-p
          ractical-approximat e-nearest-neighbor-algorithms>`_ for
          implementation details.

        - *brute_force*: compute the distance from a query point to all
          reference observations. There is no computation time for model
          creation with the brute force method (although the reference data is
          held in the model, but each query takes linear time.

        - *lsh*: use Locality Sensitive Hashing (LSH) to find approximate
          nearest neighbors efficiently. The LSH model supports 'euclidean',
          'squared_euclidean', 'manhattan', 'cosine', 'jaccard', 'dot_product'
          (deprecated), and 'transformed_dot_product' distances. Two options
          are provided for LSH -- ``num_tables`` and
          ``num_projections_per_table``. See the notes below for details.

    verbose: bool, optional
        If True, print progress updates and model details.

    **kwargs : optional
        Options for the distance function and query method.

        - *leaf_size*: for the ball tree method, the number of points in each
          leaf of the tree. The default is to use the max of 1,000 and
          n/(2^11), which ensures a maximum tree depth of 12.

        - *num_tables*: For the LSH method, the number of hash tables
          constructed. The default value is 20. We recommend choosing values
          from 10 to 30.

        - *num_projections_per_table*: For the LSH method, the number of
          projections/hash functions for each hash table. The default value is
          4 for 'jaccard' distance, 16 for 'cosine' distance and 8 for other
          distances. We recommend using number 2 ~ 6 for 'jaccard' distance, 8
          ~ 20 for 'cosine' distance and 4 ~ 12 for other distances.

    Returns
    -------
    out : NearestNeighborsModel
        A structure for efficiently computing the nearest neighbors in 'dataset'
        of new query points.

    See Also
    --------
    NearestNeighborsModel.query, turicreate.toolkits.distances

    Notes
    -----
    - Missing data is not allowed in the 'dataset' provided to this function.
      Please use the :func:`turicreate.SFrame.fillna` and
      :func:`turicreate.SFrame.dropna` utilities to handle missing data before
      creating a nearest neighbors model.

    - Missing keys in sparse vectors are assumed to have value 0.

    - The `composite_params` parameter was removed as of Turi Create
      version 1.5. The `distance` parameter now accepts either standard or
      composite distances. Please see the :mod:`~turicreate.toolkits.distances`
      module documentation for more information on composite distances.

    - If the features should be weighted equally in the distance calculations
      but are measured on different scales, it is important to standardize the
      features. One way to do this is to subtract the mean of each column and
      divide by the standard deviation.

    **Locality Sensitive Hashing (LSH)**

    There are several efficient nearest neighbors search algorithms that work
    well for data with low dimensions :math:`d` (approximately 50). However,
    most of the solutions suffer from either space or query time that is
    exponential in :math:`d`. For large :math:`d`, they often provide little,
    if any, improvement over the 'brute_force' method. This is a well-known
    consequence of the phenomenon called `The Curse of Dimensionality`.

    `Locality Sensitive Hashing (LSH)
    <https://en.wikipedia.org/wiki/Locality-sensitive_hashing>`_ is an approach
    that is designed to efficiently solve the *approximate* nearest neighbor
    search problem for high dimensional data. The key idea of LSH is to hash
    the data points using several hash functions, so that the probability of
    collision is much higher for data points which are close to each other than
    those which are far apart.

    An LSH family is a family of functions :math:`h` which map points from the
    metric space to a bucket, so that

    - if :math:`d(p, q) \\leq R`, then :math:`h(p) = h(q)` with at least probability :math:`p_1`.
    - if :math:`d(p, q) \\geq cR`, then :math:`h(p) = h(q)` with probability at most :math:`p_2`.

    LSH for efficient approximate nearest neighbor search:

    - We define a new family of hash functions :math:`g`, where each
      function :math:`g` is obtained by concatenating :math:`k` functions
      :math:`h_1, ..., h_k`, i.e., :math:`g(p)=[h_1(p),...,h_k(p)]`.
      The algorithm constructs :math:`L` hash tables, each of which
      corresponds to a different randomly chosen hash function :math:`g`.
      There are :math:`k \\cdot L` hash functions used in total.

    - In the preprocessing step, we hash all :math:`n` reference points
      into each of the :math:`L` hash tables.

    - Given a query point :math:`q`, the algorithm iterates over the
      :math:`L` hash functions :math:`g`. For each :math:`g` considered, it
      retrieves the data points that are hashed into the same bucket as q.
      These data points from all the :math:`L` hash tables are considered as
      candidates that are then re-ranked by their real distances with the query
      data.

    **Note** that the number of tables :math:`L` and the number of hash
    functions per table :math:`k` are two main parameters. They can be set
    using the options ``num_tables`` and ``num_projections_per_table``
    respectively.

    Hash functions for different distances:

    - `euclidean` and `squared_euclidean`:
      :math:`h(q) = \\lfloor \\frac{a \\cdot q + b}{w} \\rfloor` where
      :math:`a` is a vector, of which the elements are independently
      sampled from normal distribution, and :math:`b` is a number
      uniformly sampled from :math:`[0, r]`. :math:`r` is a parameter for the
      bucket width. We set :math:`r` using the average all-pair `euclidean`
      distances from a small randomly sampled subset of the reference data.

    - `manhattan`: The hash function of `manhattan` is similar with that of
      `euclidean`. The only difference is that the elements of `a` are sampled
      from Cauchy distribution, instead of normal distribution.

    - `cosine`: Random Projection is designed to approximate the cosine
      distance between vectors. The hash function is :math:`h(q) = sgn(a \\cdot
      q)`, where :math:`a` is randomly sampled normal unit vector.

    - `jaccard`: We use a recently proposed method one permutation hashing by
      Shrivastava and Li. See the paper `[Shrivastava and Li, UAI 2014]
      <http://www.auai.org/uai2014/proceedings/individuals/225.pdf>`_ for
      details.

    - `dot_product`: The reference data points are first transformed to
      fixed-norm vectors, and then the minimum `dot_product` distance search
      problem can be solved via finding the reference data with smallest
      `cosine` distances. See the paper `[Neyshabur and Srebro, ICML 2015]
      <http://proceedings.mlr.press/v37/neyshabur15.html>`_ for details.

    References
    ----------
    - `Wikipedia - nearest neighbor
      search <http://en.wikipedia.org/wiki/Nearest_neighbor_search>`_

    - `Wikipedia - ball tree <http://en.wikipedia.org/wiki/Ball_tree>`_

    - Ball tree implementation: Liu, T., et al. (2004) `An Investigation of
      Practical Approximate Nearest Neighbor Algorithms
      <http://papers.nips.cc/paper/2666-an-investigation-of-p
      ractical-approximat e-nearest-neighbor-algorithms>`_. Advances in Neural
      Information Processing Systems pp. 825-832.

    - `Wikipedia - Jaccard distance
      <http://en.wikipedia.org/wiki/Jaccard_index>`_

    - Weighted Jaccard distance: Chierichetti, F., et al. (2010) `Finding the
      Jaccard Median
      <http://theory.stanford.edu/~sergei/papers/soda10-jaccard.pdf>`_.
      Proceedings of the Twenty-First Annual ACM-SIAM Symposium on Discrete
      Algorithms. Society for Industrial and Applied Mathematics.

    - `Wikipedia - Cosine distance
      <http://en.wikipedia.org/wiki/Cosine_similarity>`_

    - `Wikipedia - Levenshtein distance
      <http://en.wikipedia.org/wiki/Levenshtein_distance>`_

    - Locality Sensitive Hashing : Chapter 3 of the book `Mining Massive
      Datasets <http://infolab.stanford.edu/~ullman/mmds/ch3.pdf>`_.

    Examples
    --------
    Construct a nearest neighbors model with automatically determined method
    and distance:

    >>> sf = turicreate.SFrame({'X1': [0.98, 0.62, 0.11],
    ...                       'X2': [0.69, 0.58, 0.36],
    ...                       'str_feature': ['cat', 'dog', 'fossa']})
    >>> model = turicreate.nearest_neighbors.create(sf, features=['X1', 'X2'])

    For datasets with a large number of rows and up to about 100 variables, the
    ball tree method often leads to much faster queries.

    >>> model = turicreate.nearest_neighbors.create(sf, features=['X1', 'X2'],
    ...                                           method='ball_tree')

    Often the final determination of a neighbor is based on several distance
    computations over different sets of features. Each part of this composite
    distance may have a different relative weight.

    >>> my_dist = [[['X1', 'X2'], 'euclidean', 2.],
    ...            [['str_feature'], 'levenshtein', 3.]]
    ...
    >>> model = turicreate.nearest_neighbors.create(sf, distance=my_dist)
    """

    ## Validate the 'dataset' input
    _tkutl._raise_error_if_not_sframe(dataset, "dataset")
    _tkutl._raise_error_if_sframe_empty(dataset, "dataset")

    ## Basic validation of the features input
    if features is not None and not isinstance(features, list):
        raise TypeError("If specified, input 'features' must be a list of " +
                        "strings.")

    ## Clean the method options and create the options dictionary
    allowed_kwargs = ['leaf_size', 'num_tables', 'num_projections_per_table']
    _method_options = {}

    for k, v in kwargs.items():
        if k in allowed_kwargs:
            _method_options[k] = v
        else:
            raise _ToolkitError("'{}' is not a valid keyword argument".format(k) +
                                " for the nearest neighbors model. Please " +
                                "check for capitalization and other typos.")


    ## Exclude inappropriate combinations of method an distance
    if method == 'ball_tree' and (distance == 'cosine'
                                  or distance == _turicreate.distances.cosine
                                  or distance == 'dot_product'
                                  or distance == _turicreate.distances.dot_product
                                  or distance == 'transformed_dot_product'
                                  or distance == _turicreate.distances.transformed_dot_product):
        raise TypeError("The ball tree method does not work with 'cosine' " +
                        "'dot_product', or 'transformed_dot_product' distance." +
                        "Please use the 'brute_force' method for these distances.")


    if method == 'lsh' and ('num_projections_per_table' not in _method_options):
        if distance == 'jaccard' or distance == _turicreate.distances.jaccard:
            _method_options['num_projections_per_table'] = 4
        elif distance == 'cosine' or distance == _turicreate.distances.cosine:
            _method_options['num_projections_per_table'] = 16
        else:
            _method_options['num_projections_per_table'] = 8

    ## Initial validation and processing of the label
    if label is None:
        _label = _robust_column_name('__id', dataset.column_names())
        _dataset = dataset.add_row_number(_label)
    else:
        _label = label
        _dataset = _copy.copy(dataset)

    col_type_map = {c:_dataset[c].dtype for c in _dataset.column_names()}
    _validate_row_label(_label, col_type_map)
    ref_labels = _dataset[_label]


    ## Determine the internal list of available feature names (may still include
    #  the row label name).
    if features is None:
        _features = _dataset.column_names()
    else:
        _features = _copy.deepcopy(features)


    ## Check if there's only one feature and it's the same as the row label.
    #  This would also be trapped by the composite distance validation, but the
    #  error message is not very informative for the user.
    free_features = set(_features).difference([_label])
    if len(free_features) < 1:
        raise _ToolkitError("The only available feature is the same as the " +
                            "row label column. Please specify features " +
                            "that are not also row labels.")


    ### Validate and preprocess the distance function
    ### ---------------------------------------------
    # - The form of the 'distance' controls how we interact with the 'features'
    #   parameter as well.
    # - At this point, the row label 'label' may still be in the list(s) of
    #   features.

    ## Convert any distance function input into a single composite distance.
    # distance is already a composite distance
    if isinstance(distance, list):
        distance = _copy.deepcopy(distance)

    # distance is a single name (except 'auto') or function handle.
    elif (hasattr(distance, '__call__') or
        (isinstance(distance, str) and not distance == 'auto')):
        distance = [[_features, distance, 1]]

    # distance is unspecified and needs to be constructed.
    elif distance is None or distance == 'auto':
        sample = _dataset.head()
        distance = _construct_auto_distance(_features,
                                            _dataset.column_names(),
                                            _dataset.column_types(),
                                            sample)

    else:
        raise TypeError("Input 'distance' not understood. The 'distance' "
                        " argument must be a string, function handle, or " +
                        "composite distance.")

    ## Basic composite distance validation, remove the row label from all
    #  feature lists, and convert string distance names into distance functions.
    distance = _scrub_composite_distance_features(distance, [_label])
    distance = _convert_distance_names_to_functions(distance)
    _validate_composite_distance(distance)

    ## Raise an error if any distances are used with non-lists
    list_features_to_check = []
    sparse_distances = ['jaccard', 'weighted_jaccard', 'cosine', 'dot_product', 'transformed_dot_product']
    sparse_distances = [_turicreate.distances.__dict__[k] for k in sparse_distances]
    for d in distance:
        feature_names, dist, _ = d
        list_features = [f for f in feature_names if _dataset[f].dtype == list]
        for f in list_features:
            if dist in sparse_distances:
                list_features_to_check.append(f)
            else:
                raise TypeError("The chosen distance cannot currently be used " +
                                "on list-typed columns.")
    for f in list_features_to_check:
        only_str_lists = _validate_lists(_dataset[f], [str])
        if not only_str_lists:
            raise TypeError("Distances for sparse data, such as jaccard " +
                            "and weighted_jaccard, can only be used on " +
                            "lists containing only strings. Please modify " +
                            "any list features accordingly before creating " +
                            "the nearest neighbors model.")

    ## Raise an error if any component has string features are in single columns
    for d in distance:
        feature_names, dist, _ = d

        if (len(feature_names) > 1) and (dist == _turicreate.distances.levenshtein):
            raise ValueError("Levenshtein distance cannot be used with multiple " +
                             "columns. Please concatenate strings into a single " +
                             "column before creating the nearest neighbors model.")

    ## Get the union of feature names and make a clean dataset.
    clean_features = _get_composite_distance_features(distance)
    sf_clean = _tkutl._toolkits_select_columns(_dataset, clean_features)


    ## Decide which method to use
    ## - If more than one distance component (specified either directly or
    #  generated automatically because distance set to 'auto'), then do brute
    #  force.
    if len(distance) > 1:
        _method = 'brute_force'

        if method != 'brute_force' and verbose is True:
            print("Defaulting to brute force instead of ball tree because " +\
                "there are multiple distance components.")

    else:
        if method == 'auto':

            # get the total number of variables. Assume the number of elements in
            # array type columns does not change
            num_variables = sum([len(x) if hasattr(x, '__iter__') else 1
                for x in _six.itervalues(sf_clean[0])])

            # flag if all the features in the single composite are of numeric
            # type.
            numeric_type_flag = all([x in [int, float, list, array.array]
                for x in sf_clean.column_types()])

            ## Conditions necessary for ball tree to work and be worth it
            if ((distance[0][1] in ['euclidean',
                                    'manhattan',
                                    _turicreate.distances.euclidean,
                                    _turicreate.distances.manhattan])
                    and numeric_type_flag is True
                    and num_variables <= 200):

                    _method = 'ball_tree'

            else:
                _method = 'brute_force'

        else:
            _method = method


    ## Pick the right model name for the method
    if _method == 'ball_tree':
        model_name = 'nearest_neighbors_ball_tree'

    elif _method == 'brute_force':
        model_name = 'nearest_neighbors_brute_force'

    elif _method == 'lsh':
        model_name = 'nearest_neighbors_lsh'

    else:
        raise ValueError("Method must be 'auto', 'ball_tree', 'brute_force', " +
                         "or 'lsh'.")


    ## Package the model options
    opts = {}
    opts.update(_method_options)
    opts.update(
        {'model_name': model_name,
        'ref_labels': ref_labels,
        'label': label,
        'sf_features': sf_clean,
        'composite_params': distance})

    ## Construct the nearest neighbors model
    if not verbose:
        _turicreate.connect.main.get_server().set_log_progress(False)

    result = _turicreate.extensions._nearest_neighbors.train(opts)

    _turicreate.connect.main.get_server().set_log_progress(True)

    model_proxy = result['model']
    model = NearestNeighborsModel(model_proxy)

    return model


class NearestNeighborsModel(_Model):
    """
    The NearestNeighborsModel represents rows of an SFrame in a structure that
    is used to quickly and efficiently find the nearest neighbors of a query
    point.

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.nearest_neighbors.create` to create an instance of this
    model. A detailed list of parameter options and code samples are available
    in the documentation for the create function.
    """

    def __init__(self, model_proxy):
        """___init__(self)"""
        self.__proxy__ = model_proxy
        self.__name__ = 'nearest_neighbors'

    @classmethod
    def _native_name(cls):
        return ["nearest_neighbors_ball_tree", "nearest_neighbors_brute_force", "nearest_neighbors_lsh"]

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the NearestNeighborsModel.
        """
        return self.__repr__()

    def _get_summary_struct(self):
        """
        Returns a structured description of the model, including (where
        relevant) the schema of the training data, description of the training
        data, training statistics, and model hyperparameters.

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

        model_fields = [
            ("Method", 'method'),
            ("Number of distance components", 'num_distance_components'),
            ("Number of examples", 'num_examples'),
            ("Number of feature columns", 'num_features'),
            ("Number of unpacked features", 'num_unpacked_features'),
            ("Total training time (seconds)", 'training_time')]

        ball_tree_fields = [
            ("Tree depth", 'tree_depth'),
            ("Leaf size", 'leaf_size')]

        lsh_fields = [
            ("Number of hash tables", 'num_tables'),
            ("Number of projections per table", 'num_projections_per_table')]

        sections = [model_fields]
        section_titles = ['Attributes']

        if (self.method == 'ball_tree'):
            sections.append(ball_tree_fields)
            section_titles.append('Ball Tree Attributes')

        if (self.method == 'lsh'):
            sections.append(lsh_fields)
            section_titles.append('LSH Attributes')

        return (sections, section_titles)

    def __repr__(self):
        """
        Print a string description of the model when the model name is entered
        in the terminal.
        """

        (sections, section_titles) = self._get_summary_struct()
        return _tkutl._toolkit_repr_print(self, sections, section_titles, width=30)

    def _list_fields(self):
        """
        List the fields stored in the model, including data, model, and
        training options. Each field can be queried with the ``get`` method.

        Returns
        -------
        out : list
            List of fields queryable with the ``get`` method.
        """
        opts = {'model': self.__proxy__, 'model_name': self.__name__}
        response = _turicreate.toolkits._main.run('_nearest_neighbors.list_keys',
                                               opts)

        return sorted(response.keys())

    def _get(self, field):
        """
        Return the value of a given field. The list of all queryable fields is
        detailed below, and can be obtained with the
        :func:`~turicreate.nearest_neighbors.NearestNeighborsModel._list_fields`
        method.

        +-----------------------+----------------------------------------------+
        |      Field            | Description                                  |
        +=======================+==============================================+
        | distance              | Measure of dissimilarity between two points  |
        +-----------------------+----------------------------------------------+
        | features              | Feature column names                         |
        +-----------------------+----------------------------------------------+
        | unpacked_features     | Names of the individual features used        |
        +-----------------------+----------------------------------------------+
        | label                 | Label column names                           |
        +-----------------------+----------------------------------------------+
        | leaf_size             | Max size of leaf nodes (ball tree only)      |
        +-----------------------+----------------------------------------------+
        | method                | Method of organizing reference data          |
        +-----------------------+----------------------------------------------+
        | num_examples          | Number of reference data observations        |
        +-----------------------+----------------------------------------------+
        | num_features          | Number of features for distance computation  |
        +-----------------------+----------------------------------------------+
        | num_unpacked_features | Number of unpacked features                  |
        +-----------------------+----------------------------------------------+
        | num_variables         | Number of variables for distance computation |
        +-----------------------+----------------------------------------------+
        | training_time         | Time to create the reference structure       |
        +-----------------------+----------------------------------------------+
        | tree_depth            | Number of levels in the tree (ball tree only)|
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
        opts = {'model': self.__proxy__,
                'model_name': self.__name__,
                'field': field}
        response = _turicreate.toolkits._main.run('_nearest_neighbors.get_value',
                                                opts)
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
            NearestNeighborsModel.

        See Also
        --------
        summary

        Examples
        --------
        >>> sf = turicreate.SFrame({'label': range(3),
        ...                       'feature1': [0.98, 0.62, 0.11],
        ...                       'feature2': [0.69, 0.58, 0.36]})
        >>> model = turicreate.nearest_neighbors.create(sf, 'label')
        >>> model.training_stats()
        {'features': 'feature1, feature2',
         'label': 'label',
         'leaf_size': 1000,
         'num_examples': 3,
         'num_features': 2,
         'num_variables': 2,
         'training_time': 0.023223,
         'tree_depth': 1}
        """

        opts = {'model': self.__proxy__, 'model_name': self.__name__}
        return _turicreate.toolkits._main.run("_nearest_neighbors.training_stats",
                opts)

    def query(self, dataset, label=None, k=5, radius=None, verbose=True):
        """
        For each row of the input 'dataset', retrieve the nearest neighbors
        from the model's stored data. In general, the query dataset does not
        need to be the same as the reference data stored in the model, but if
        it is, the 'include_self_edges' parameter can be set to False to
        exclude results that match query points to themselves.

        Parameters
        ----------
        dataset : SFrame
            Query data. Must contain columns with the same names and types as
            the features used to train the model. Additional columns are
            allowed, but ignored. Please see the nearest neighbors
            :func:`~turicreate.nearest_neighbors.create` documentation for more
            detail on allowable data types.

        label : str, optional
            Name of the query SFrame column with row labels. If 'label' is not
            specified, row numbers are used to identify query dataset rows in
            the output SFrame.

        k : int, optional
            Number of nearest neighbors to return from the reference set for
            each query observation. The default is 5 neighbors, but setting it
            to ``None`` will return all neighbors within ``radius`` of the
            query point.

        radius : float, optional
            Only neighbors whose distance to a query point is smaller than this
            value are returned. The default is ``None``, in which case the
            ``k`` nearest neighbors are returned for each query point,
            regardless of distance.

        verbose: bool, optional
            If True, print progress updates and model details.

        Returns
        -------
        out : SFrame
            An SFrame with the k-nearest neighbors of each query observation.
            The result contains four columns: the first is the label of the
            query observation, the second is the label of the nearby reference
            observation, the third is the distance between the query and
            reference observations, and the fourth is the rank of the reference
            observation among the query's k-nearest neighbors.

        See Also
        --------
        similarity_graph

        Notes
        -----
        - The `dataset` input to this method *can* have missing values (in
          contrast to the reference dataset used to create the nearest
          neighbors model). Missing numeric values are imputed to be the mean
          of the corresponding feature in the reference dataset, and missing
          strings are imputed to be empty strings.

        - If both ``k`` and ``radius`` are set to ``None``, each query point
          returns all of the reference set. If the reference dataset has
          :math:`n` rows and the query dataset has :math:`m` rows, the output
          is an SFrame with :math:`nm` rows.

        - For models created with the 'lsh' method, the query results may have
          fewer query labels than input query points. Because LSH is an
          approximate method, a query point may have fewer than 'k' neighbors.
          If LSH returns no neighbors at all for a query, the query point is
          omitted from the results.

        Examples
        --------
        First construct a toy SFrame and create a nearest neighbors model:

        >>> sf = turicreate.SFrame({'label': range(3),
        ...                       'feature1': [0.98, 0.62, 0.11],
        ...                       'feature2': [0.69, 0.58, 0.36]})
        >>> model = turicreate.nearest_neighbors.create(sf, 'label')

        A new SFrame contains query observations with same schema as the
        reference SFrame. This SFrame is passed to the ``query`` method.

        >>> queries = turicreate.SFrame({'label': range(3),
        ...                            'feature1': [0.05, 0.61, 0.99],
        ...                            'feature2': [0.06, 0.97, 0.86]})
        >>> model.query(queries, 'label', k=2)
        +-------------+-----------------+----------------+------+
        | query_label | reference_label |    distance    | rank |
        +-------------+-----------------+----------------+------+
        |      0      |        2        | 0.305941170816 |  1   |
        |      0      |        1        | 0.771556867638 |  2   |
        |      1      |        1        | 0.390128184063 |  1   |
        |      1      |        0        | 0.464004310325 |  2   |
        |      2      |        0        | 0.170293863659 |  1   |
        |      2      |        1        | 0.464004310325 |  2   |
        +-------------+-----------------+----------------+------+
        """

        ## Validate the 'dataset' input
        _tkutl._raise_error_if_not_sframe(dataset, "dataset")
        _tkutl._raise_error_if_sframe_empty(dataset, "dataset")

        ## Get model features
        ref_features = self.features
        sf_features = _tkutl._toolkits_select_columns(dataset, ref_features)

        ## Validate and preprocess the 'label' input
        if label is None:
            query_labels = _turicreate.SArray.from_sequence(len(dataset))

        else:
            if not label in dataset.column_names():
                raise ValueError(
                    "Input 'label' must be a string matching the name of a " +\
                    "column in the reference SFrame 'dataset'.")

            if not dataset[label].dtype == str and not dataset[label].dtype == int:
                raise TypeError("The label column must contain integers or strings.")

            if label in ref_features:
                raise ValueError("The label column cannot be one of the features.")

            query_labels = dataset[label]


        ## Validate neighborhood parameters 'k' and 'radius'
        if k is not None:
            if not isinstance(k, int):
                raise ValueError("Input 'k' must be an integer.")

            if k <= 0:
                raise ValueError("Input 'k' must be larger than 0.")

        if radius is not None:
            if not isinstance(radius, (int, float)):
                raise ValueError("Input 'radius' must be an integer or float.")

            if radius < 0:
                raise ValueError("Input 'radius' must be non-negative.")


        ## Set k and radius to special values to indicate 'None'
        if k is None:
            k = -1

        if radius is None:
            radius = -1.0

        opts = {'model': self.__proxy__,
                'model_name': self.__name__,
                'features': sf_features,
                'query_labels': query_labels,
                'k': k,
                'radius': radius}

        result = _turicreate.toolkits._main.run('_nearest_neighbors.query', opts,
                                             verbose)
        return _SFrame(None, _proxy=result['neighbors'])

    def similarity_graph(self, k=5, radius=None, include_self_edges=False,
                         output_type='SGraph', verbose=True):
        """
        Construct the similarity graph on the reference dataset, which is
        already stored in the model. This is conceptually very similar to
        running `query` with the reference set, but this method is optimized
        for the purpose, syntactically simpler, and automatically removes
        self-edges.

        Parameters
        ----------
        k : int, optional
            Maximum number of neighbors to return for each point in the
            dataset. Setting this to ``None`` deactivates the constraint, so
            that all neighbors are returned within ``radius`` of a given point.

        radius : float, optional
            For a given point, only neighbors within this distance are
            returned. The default is ``None``, in which case the ``k`` nearest
            neighbors are returned for each query point, regardless of
            distance.

        include_self_edges : bool, optional
            For most distance functions, each point in the model's reference
            dataset is its own nearest neighbor. If this parameter is set to
            False, this result is ignored, and the nearest neighbors are
            returned *excluding* the point itself.

        output_type : {'SGraph', 'SFrame'}, optional
            By default, the results are returned in the form of an SGraph,
            where each point in the reference dataset is a vertex and an edge A
            -> B indicates that vertex B is a nearest neighbor of vertex A. If
            'output_type' is set to 'SFrame', the output is in the same form as
            the results of the 'query' method: an SFrame with columns
            indicating the query label (in this case the query data is the same
            as the reference data), reference label, distance between the two
            points, and the rank of the neighbor.

        verbose : bool, optional
            If True, print progress updates and model details.

        Returns
        -------
        out : SFrame or SGraph
            The type of the output object depends on the 'output_type'
            parameter. See the parameter description for more detail.

        Notes
        -----
        - If both ``k`` and ``radius`` are set to ``None``, each data point is
          matched to the entire dataset. If the reference dataset has
          :math:`n` rows, the output is an SFrame with :math:`n^2` rows (or an
          SGraph with :math:`n^2` edges).

        - For models created with the 'lsh' method, the output similarity graph
          may have fewer vertices than there are data points in the original
          reference set. Because LSH is an approximate method, a query point
          may have fewer than 'k' neighbors. If LSH returns no neighbors at all
          for a query and self-edges are excluded, the query point is omitted
          from the results.

        Examples
        --------
        First construct an SFrame and create a nearest neighbors model:

        >>> sf = turicreate.SFrame({'x1': [0.98, 0.62, 0.11],
        ...                       'x2': [0.69, 0.58, 0.36]})
        ...
        >>> model = turicreate.nearest_neighbors.create(sf, distance='euclidean')

        Unlike the ``query`` method, there is no need for a second dataset with
        ``similarity_graph``.

        >>> g = model.similarity_graph(k=1)  # an SGraph
        >>> g.edges
        +----------+----------+----------------+------+
        | __src_id | __dst_id |    distance    | rank |
        +----------+----------+----------------+------+
        |    0     |    1     | 0.376430604494 |  1   |
        |    2     |    1     | 0.55542776308  |  1   |
        |    1     |    0     | 0.376430604494 |  1   |
        +----------+----------+----------------+------+
        """
        ## Validate inputs.
        if k is not None:
            if not isinstance(k, int):
                raise ValueError("Input 'k' must be an integer.")

            if k <= 0:
                raise ValueError("Input 'k' must be larger than 0.")

        if radius is not None:
            if not isinstance(radius, (int, float)):
                raise ValueError("Input 'radius' must be an integer or float.")

            if radius < 0:
                raise ValueError("Input 'radius' must be non-negative.")


        ## Set k and radius to special values to indicate 'None'
        if k is None:
            k = -1

        if radius is None:
            radius = -1.0

        opts = {'model': self.__proxy__,
                'model_name': self.__name__,
                'k': k,
                'radius': radius,
                'include_self_edges': include_self_edges}

        result = _turicreate.toolkits._main.run('_nearest_neighbors.similarity_graph',
                                              opts, verbose)

        knn = _SFrame(None, _proxy=result['neighbors'])

        if output_type == "SFrame":
            return knn

        else:
            sg = _SGraph(edges=knn, src_field='query_label',
                         dst_field='reference_label')
            return sg


    @classmethod
    def _get_queryable_methods(cls):
        '''Returns a list of method names that are queryable through Predictive
        Service'''
        return {'query':{'dataset':'sframe'}}
