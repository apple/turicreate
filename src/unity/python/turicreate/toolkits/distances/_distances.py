# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Distance functions and utilities.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import turicreate as _tc


### --------------------------- ###
### Standard distance functions ###
### --------------------------- ###

def euclidean(x, y):
    """
    Compute the Euclidean distance between two dictionaries or two lists
    of equal length. Suppose `x` and `y` each contain :math:`d`
    variables:

    .. math:: D(x, y) = \\sqrt{\sum_i^d (x_i - y_i)^2}

    Parameters
    ----------
    x : dict or list
        First input vector.

    y : dict or list
        Second input vector.

    Returns
    -------
    out : float
        Euclidean distance between `x` and `y`.

    Notes
    -----
    - If the input vectors are in dictionary form, keys missing in one
      of the two dictionaries are assumed to have value 0.

    References
    ----------
    - `Wikipedia - Euclidean distance
      <http://en.wikipedia.org/wiki/Euclidean_distance>`_

    Examples
    --------
    >>> tc.distances.euclidean([1, 2, 3], [4, 5, 6])
    5.196152422706632
    ...
    >>> tc.distances.euclidean({'a': 2, 'c': 4}, {'b': 3, 'c': 12})
    8.774964387392123
    """
    return _tc.extensions._distances.euclidean(x, y)

def gaussian_kernel(x, y):
    """
    Compute a Gaussian-type distance between two dictionaries or two lists
    of equal length. Suppose `x` and `y` each contain :math:`d`
    variables:

    .. math:: D(x, y) = 1 - \\exp{-\sum_i^d (x_i - y_i)^2}

    Parameters
    ----------
    x : dict or list
        First input vector.

    y : dict or list
        Second input vector.

    Returns
    -------
    out : float
        Gaussian distance between `x` and `y`.

    Notes
    -----
    - If the input vectors are in dictionary form, keys missing in one
      of the two dictionaries are assumed to have value 0.

    References
    ----------
    - `Wikipedia - Euclidean distance
      <http://en.wikipedia.org/wiki/Euclidean_distance>`_

    Examples
    --------
    >>> tc.distances.gaussian([.1, .2, .3], [.4, .5, .6])
    5.196152422706632
    ...
    >>> tc.distances.euclidean({'a': 2, 'c': 4}, {'b': 3, 'c': 12})
    8.774964387392123
    """
    return _tc.extensions._distances.gaussian_kernel(x, y)

def squared_euclidean(x, y):
    """
    Compute the squared Euclidean distance between two dictionaries or
    two lists of equal length. Suppose `x` and `y` each contain
    :math:`d` variables:

    .. math:: D(x, y) = \sum_i^d (x_i - y_i)^2

    Parameters
    ----------
    x : dict or list
        First input vector.

    y : dict or list
        Second input vector.

    Returns
    -------
    out : float
        Squared Euclidean distance between `x` and `y`.

    Notes
    -----
    - If the input vectors are in dictionary form, keys missing in one
      of the two dictionaries are assumed to have value 0.

    - Squared Euclidean distance does not satisfy the triangle
      inequality, so it is not a metric. This means the ball tree cannot
      be used to compute nearest neighbors based on this distance.

    References
    ----------
    - `Wikipedia - Euclidean distance
      <http://en.wikipedia.org/wiki/Euclidean_distance>`_

    Examples
    --------
    >>> tc.distances.squared_euclidean([1, 2, 3], [4, 5, 6])
    27.0
    ...
    >>> tc.distances.squared_euclidean({'a': 2, 'c': 4},
    ...                                {'b': 3, 'c': 12})
    77.0
    """
    return _tc.extensions._distances.squared_euclidean(x, y)

def manhattan(x, y):
    """
    Compute the Manhattan distance between between two dictionaries or
    two lists of equal length. Suppose `x` and `y` each contain
    :math:`d` variables:

    .. math:: D(x, y) = \\sum_i^d |x_i - y_i|

    Parameters
    ----------
    x : dict or list
        First input vector.

    y : dict or list
        Second input vector.

    Returns
    -------
    out : float
        Manhattan distance between `x` and `y`.

    Notes
    -----
    - If the input vectors are in dictionary form, keys missing in one
      of the two dictionaries are assumed to have value 0.

    - Manhattan distance is also known as "city block" or "taxi cab"
      distance.

    References
    ----------
    - `Wikipedia - taxicab geometry
      <http://en.wikipedia.org/wiki/Taxicab_geometry>`_

    Examples
    --------
    >>> tc.distances.manhattan([1, 2, 3], [4, 5, 6])
    9.0
    ...
    >>> tc.distances.manhattan({'a': 2, 'c': 4}, {'b': 3, 'c': 12})
    13.0
    """
    return _tc.extensions._distances.manhattan(x, y)

def cosine(x, y):
    """
    Compute the cosine distance between between two dictionaries or two
    lists of equal length. Suppose `x` and `y` each contain
    :math:`d` variables:

    .. math::

        D(x, y) = 1 - \\frac{\sum_i^d x_i y_i}
        {\sqrt{\sum_i^d x_i^2}\sqrt{\sum_i^d y_i^2}}

    Parameters
    ----------
    x : dict or list
        First input vector.

    y : dict or list
        Second input vector.

    Returns
    -------
    out : float
        Cosine distance between `x` and `y`.

    Notes
    -----
    - If the input vectors are in dictionary form, keys missing in one
      of the two dictionaries are assumed to have value 0.

    - Cosine distance is not a metric. This means the ball tree cannot
      be used to compute nearest neighbors based on this distance.

    References
    ----------
    - `Wikipedia - cosine similarity
      <http://en.wikipedia.org/wiki/Cosine_similarity>`_

    Examples
    --------
    >>> tc.distances.cosine([1, 2, 3], [4, 5, 6])
    0.025368153802923787
    ...
    >>> tc.distances.cosine({'a': 2, 'c': 4}, {'b': 3, 'c': 12})
    0.13227816872537534
    """
    return _tc.extensions._distances.cosine(x, y)

def levenshtein(x, y):
    """
    Compute the Levenshtein distance between between strings. The
    distance is the number of insertion, deletion, and substitution edits
    needed to transform string `x` into string `y`. The mathematical
    definition of Levenshtein is recursive:

    .. math::

        D(x, y) = d(|x|, |y|)

        d(i, j) = \max(i, j), \quad \mathrm{if } \min(i, j) = 0

        d(i, j) = \min \Big \{d(i-1, j) + 1, \ d(i, j-1) + 1, \ d(i-1, j-1) + I(x_i \\neq y_i) \Big \}, \quad \mathrm{else}


    Parameters
    ----------
    x : string
        First input string.

    y : string
        Second input string.

    Returns
    -------
    out : float
        Levenshtein distance between `x` and `y`.

    References
    ----------
    - `Wikipedia - Levenshtein distance
      <http://en.wikipedia.org/wiki/Levenshtein_distance>`_

    Examples
    --------
    >>> tc.distances.levenshtein("fossa", "fossil")
    2.0
    """
    return _tc.extensions._distances.levenshtein(x, y)

def dot_product(x, y):
    """
    Compute the dot_product between two dictionaries or two lists of
    equal length. Suppose `x` and `y` each contain :math:`d` variables:

    .. math:: D(x, y) = \\frac{1}{\sum_i^d x_i y_i}

    .. warning::

        The 'dot_product' distance is deprecated and will be removed in future
        versions of Turi Create. Please use 'transformed_dot_product'
        distance instead, although note that this is more than a name change; it
        is a *different* transformation of the dot product of two vectors.
        Please see the distances module documentation for more details.

    Parameters
    ----------
    x : dict or list
        First input vector.

    y : dict or list
        Second input vector.

    Returns
    -------
    out : float

    Notes
    -----
    - If the input vectors are in dictionary form, keys missing in one
      of the two dictionaries are assumed to have value 0.

    - Dot product distance is not a metric. This means the ball tree
      cannot be used to compute nearest neighbors based on this distance.

    Examples
    --------
    >>> tc.distances.dot_product([1, 2, 3], [4, 5, 6])
    0.03125
    ...
    >>> tc.distances.dot_product({'a': 2, 'c': 4}, {'b': 3, 'c': 12})
    0.020833333333333332
    """
    return _tc.extensions._distances.dot_product(x, y)

def transformed_dot_product(x, y):
    """
    Compute the "transformed_dot_product" distance between two dictionaries or
    two lists of equal length. This is a way to transform the dot product of the
    two inputs---a similarity measure---into a distance measure. Suppose `x` and
    `y` each contain :math:`d` variables:

    .. math:: D(x, y) = \log\{1 + \exp\{-\sum_i^d x_i y_i\}\}

    .. warning::

        The 'dot_product' distance is deprecated and will be removed in future
        versions of Turi Create. Please use 'transformed_dot_product'
        distance instead, although note that this is more than a name change; it
        is a *different* transformation of the dot product of two vectors.
        Please see the distances module documentation for more details.

    Parameters
    ----------
    x : dict or list
        First input vector.

    y : dict or list
        Second input vector.

    Returns
    -------
    out : float

    Notes
    -----
    - If the input vectors are in dictionary form, keys missing in one
      of the two dictionaries are assumed to have value 0.

    - Transformed dot product distance is not a metric because the distance from
      a point to itself is not 0. This means the ball tree cannot be used to
      compute nearest neighbors based on this distance.

    Examples
    --------
    >>> tc.distances.transformed_dot_product([1, 2, 3], [4, 5, 6])
    0.03125
    ...
    >>> tc.distances.transformed_dot_product({'a': 2, 'c': 4}, {'b': 3, 'c': 12})
    0.020833333333333332
    """
    return _tc.extensions._distances.transformed_dot_product(x, y)

def jaccard(x, y):
    """
    Compute the Jaccard distance between between two dictionaries.
    Suppose :math:`K_x` and :math:`K_y` are the sets of keys from the
    two input dictionaries.

    .. math:: D(x, y) = 1 - \\frac{|K_x \cap K_y|}{|K_x \cup K_y|}

    Parameters
    ----------
    x : dict
        First input dictionary.

    y : dict
        Second input dictionary.

    Returns
    -------
    out : float
        Jaccard distance between `x` and `y`.

    Notes
    -----
    - Jaccard distance treats the keys in the input dictionaries as
      sets, and ignores the values in the input dictionaries.

    References
    ----------
    - `Wikipedia - Jaccard distance
      <http://en.wikipedia.org/wiki/Jaccard_index>`_

    Examples
    --------
    >>> tc.distances.jaccard({'a': 2, 'c': 4}, {'b': 3, 'c': 12})
    0.6666666666666667
    """
    return _tc.extensions._distances.jaccard(x, y)

def weighted_jaccard(x, y):
    """
    Compute the weighted Jaccard distance between between two
    dictionaries. Suppose :math:`K_x` and :math:`K_y` are the sets of
    keys from the two input dictionaries, while :math:`x_k` and
    :math:`y_k` are the values associated with key :math:`k` in the
    respective dictionaries. Typically these values are counts, i.e. of
    words or n-grams.

    .. math::

        D(x, y) = 1 - \\frac{\sum_{k \in K_x \cup K_y} \min\{x_k, y_k\}}
        {\sum_{k \in K_x \cup K_y} \max\{x_k, y_k\}}

    Parameters
    ----------
    x : dict
        First input dictionary.

    y : dict
        Second input dictionary.

    Returns
    -------
    out : float
        Weighted jaccard distance between `x` and `y`.

    Notes
    -----
    - If a key is missing in one of the two dictionaries, it is assumed
      to have value 0.

    References
    ----------
    - Weighted Jaccard distance: Chierichetti, F., et al. (2010)
      `Finding the Jaccard Median
      <http://theory.stanford.edu/~sergei/papers/soda10-jaccard.pdf>`_.
      Proceedings of the Twenty-First Annual ACM-SIAM Symposium on
      Discrete Algorithms. Society for Industrial and Applied
      Mathematics.

    Examples
    --------
    >>> tc.distances.weighted_jaccard({'a': 2, 'c': 4},
    ...                               {'b': 3, 'c': 12})
    0.7647058823529411
    """
    return _tc.extensions._distances.weighted_jaccard(x, y)
