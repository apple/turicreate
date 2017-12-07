# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
This module provides utilities for doing text processing.

Note that standard SArray utilities can be used for transforming text data
into "bag of words" format, where a document is represented as a
dictionary mapping unique words with the number of times that word occurs
in the document. See :py:func:`~turicreate.text_analytics.count_words`
for more details. Also, see :py:func:`~turicreate.SFrame.pack_columns` and
:py:func:`~turicreate.SFrame.unstack` for ways of creating SArrays
containing dictionary types.

We provide methods for learning topic models, which can be useful for modeling
large document collections. See :py:func:`~turicreate.topic_model.create` for
more info.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

__all__ = ['tf_idf', 'bm25', 'stopwords', 'count_words',
           'count_ngrams', 'random_split', 'parse_sparse',
           'parse_docword', 'tokenize', 'trim_rare_words','split_by_sentence',
           'extract_parts_of_speech']
def __dir__():
  return ['tf_idf', 'bm25', 'stopwords', 'count_words',
          'count_ngrams', 'random_split', 'parse_sparse',
          'parse_docword', 'tokenize', 'trim_rare_words', 'split_by_sentence',
          'extract_parts_of_speech']

from ._util import tf_idf
from ._util import bm25
from ._util import stopwords
from ._util import count_words
from ._util import count_ngrams
from ._util import random_split
from ._util import parse_sparse
from ._util import parse_docword
from ._util import tokenize
from ._util import trim_rare_words
