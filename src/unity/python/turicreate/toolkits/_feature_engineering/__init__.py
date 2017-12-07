# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
A transformer is a stateful object that transforms input data (as an SFrame)
from one form to another. Transformers are commonly used for feature
engineering. In addition to the modules provided in Turi create, users can
write transformers that integrate seamlessly with already existing ones.


Each transformer has the following methods:

    +---------------+---------------------------------------------------+
    |   Method      | Description                                       |
    +===============+===================================================+
    | __init__      | Construct the object.                             |
    +---------------+---------------------------------------------------+
    | fit           | Fit the object using training data.               |
    +---------------+---------------------------------------------------+
    | transform     | Transform the object on training/test data.       |
    +---------------+---------------------------------------------------+
    | fit_transform | First perform fit() and then transform() on data. |
    +---------------+---------------------------------------------------+
    | save          | Save the model to a Turi Create archive.      |
    +---------------+---------------------------------------------------+
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ._feature_engineering import Transformer as _Transformer
from ._feature_engineering import _SampleTransformer

from ._feature_engineering import TransformerBase
from ._transformer_chain import TransformerChain
from ._tokenizer import Tokenizer
from ._tfidf import TFIDF
from ._bm25 import BM25
from ._word_counter import WordCounter
from ._ngram_counter import NGramCounter
from ._word_trimmer import RareWordTrimmer
from ._transform_to_flat_dictionary import TransformToFlatDictionary
from ._autovectorizer import AutoVectorizer
from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe

def create(dataset, transformers):
    """
    Create a Transformer object to transform data for feature engineering.

    Parameters
    ----------
    dataset : SFrame
        The dataset to use for training the model.

    transformers: Transformer  | list[Transformer]
        An Transformer or a list of Transformers.

    See Also
    --------
    turicreate.toolkits.feature_engineering._feature_engineering._TransformerBase

    Examples
    --------

    .. sourcecode:: python

        # Create data.
        >>> sf = turicreate.SFrame({'a': [1,2,3], 'b' : [2,3,4]})

        >>> from turicreate.feature_engineering import FeatureHasher, \
                                               QuadraticFeatures, OneHotEncoder

        # Create a single transformer.
        >>> encoder = turicreate.feature_engineering.create(sf,
                                 OneHotEncoder(max_categories = 10))

        # Create a chain of transformers.
        >>> chain = turicreate.feature_engineering.create(sf, [
                                    QuadraticFeatures(),
                                    FeatureHasher()
                                  ])

        # Create a chain of transformers with names for each of the steps.
        >>> chain = turicreate.feature_engineering.create(sf, [
                                    ('quadratic', QuadraticFeatures()),
                                    ('hasher', FeatureHasher())
                                  ])


    """
    err_msg = "The parameters 'transformers' must be a valid Transformer object."
    cls = transformers.__class__

    _raise_error_if_not_sframe(dataset, "dataset")

    # List of transformers.
    if (cls == list):
        transformers = TransformerChain(transformers)
    # Transformer.
    else:
        if not issubclass(cls, TransformerBase):
            raise TypeError(err_msg)
    # Fit and return
    transformers.fit(dataset)
    return transformers
