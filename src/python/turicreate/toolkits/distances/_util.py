# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Private utility functions for working with distance specifications and
functions.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import copy as _copy
import array as _array
import six as _six
from operator import iadd
import turicreate as _tc

import sys as _sys
if _sys.version_info.major == 3:
    from functools import reduce

def compute_composite_distance(distance, x, y):
    """
    Compute the value of a composite distance function on two dictionaries,
    typically SFrame rows.

    Parameters
    ----------
    distance : list[list]
        A composite distance function. Composite distance functions are a
        weighted sum of standard distance functions, each of which applies to
        its own subset of features. Composite distance functions are specified
        as a list of distance components, each of which is itself a list
        containing three items:

          1. list or tuple of feature names (strings)

          2. standard distance name (string)

          3. scaling factor (int or float)

    x, y : dict
        Individual observations, typically rows of an SFrame, in dictionary
        form. Must include the features specified by `distance`.

    Returns
    -------
    out : float
        The distance between `x` and `y`, as specified by `distance`.

    Examples
    --------
    >>> sf = turicreate.SFrame({'X1': [0.98, 0.62, 0.11],
    ...                       'X2': [0.69, 0.58, 0.36],
    ...                       'species': ['cat', 'dog', 'fossa']})
    ...
    >>> dist_spec = [[('X1', 'X2'), 'euclidean', 2],
    ...              [('species',), 'levenshtein', 0.4]]
    ...
    >>> d = turicreate.distances.compute_composite_distance(dist_spec, sf[0], sf[1])
    >>> print d
    1.95286120899
    """

    ## Validate inputs
    _validate_composite_distance(distance)
    distance = _convert_distance_names_to_functions(distance)

    if not isinstance(x, dict) or not isinstance(y, dict):
        raise TypeError("Inputs 'x' and 'y' must be in dictionary form. " +
                        "Selecting individual rows of an SFrame yields the " +
                        "correct format.")

    ans = 0.

    for d in distance:
        ftrs, dist, weight = d

        ## Special check for multiple columns with levenshtein distance.
        if dist == _tc.distances.levenshtein and len(ftrs) > 1:
            raise ValueError("levenshtein distance cannot be used with multiple" +
                             "columns. Please concatenate strings into a single " +
                             "column before computing the distance.")

        ## Extract values for specified features.
        a = {}
        b = {}

        for ftr in ftrs:
            if type(x[ftr]) != type(y[ftr]):
                if not isinstance(x[ftr], (int, float)) or not isinstance(y[ftr], (int, float)):
                    raise ValueError("Input data has different types.")

            if isinstance(x[ftr], (int, float, str)):
                a[ftr] = x[ftr]
                b[ftr] = y[ftr]

            elif isinstance(x[ftr], dict):
                for key, val in _six.iteritems(x[ftr]):
                    a['{}.{}'.format(ftr, key)] = val

                for key, val in _six.iteritems(y[ftr]):
                    b['{}.{}'.format(ftr, key)] = val

            elif isinstance(x[ftr], (list, _array.array)):
                for i, val in enumerate(x[ftr]):
                    a[i] = val

                for i, val in enumerate(y[ftr]):
                    b[i] = val

            else:
                raise TypeError("Type of feature '{}' not understood.".format(ftr))


        ## Pull out the raw values for levenshtein
        if dist == _tc.distances.levenshtein:
            a = list(a.values())[0]
            b = list(b.values())[0]

        ## Compute component distance and add to the total distance.
        ans += weight * dist(a, b)

    return ans


def _validate_composite_distance(distance):
    """
    Check that composite distance function is in valid form. Don't modify the
    composite distance in any way.
    """

    if not isinstance(distance, list):
        raise TypeError("Input 'distance' must be a composite distance.")

    if len(distance) < 1:
        raise ValueError("Composite distances must have a least one distance "
                         "component, consisting of a list of feature names, "
                         "a distance function (string or function handle), "
                         "and a weight.")

    for d in distance:

        ## Extract individual pieces of the distance component
        try:
            ftrs, dist, weight = d
        except:
            raise TypeError("Elements of a composite distance function must " +
                            "have three items: a set of feature names (tuple or list), " +
                            "a distance function (string or function handle), " +
                            "and a weight.")

        ## Validate feature names
        if len(ftrs) == 0:
            raise ValueError("An empty list of features cannot be passed " +\
                             "as part of a composite distance function.")

        if not isinstance(ftrs, (list, tuple)):
            raise TypeError("Feature names must be specified in a list or tuple.")

        if not all([isinstance(x, str) for x in ftrs]):
            raise TypeError("Feature lists must contain only strings.")


        ## Validate standard distance function
        if not isinstance(dist, str) and not hasattr(dist, '__call__'):
            raise ValueError("Standard distances must be the name of a distance " +
                             "function (string) or a distance function handle")

        if isinstance(dist, str):
            try:
                _tc.distances.__dict__[dist]
            except:
                raise ValueError("Distance '{}' not recognized".format(dist))


        ## Validate weight
        if not isinstance(weight, (int, float)):
            raise ValueError(
                "The weight of each distance component must be a single " +\
                "integer or a float value.")

        if weight < 0:
            raise ValueError("The weight on each distance component must be " +
                             "greater than or equal to zero.")


def _scrub_composite_distance_features(distance, feature_blacklist):
    """
    Remove feature names from the feature lists in a composite distance
    function.
    """
    dist_out = []

    for i, d in enumerate(distance):
        ftrs, dist, weight = d
        new_ftrs = [x for x in ftrs if x not in feature_blacklist]
        if len(new_ftrs) > 0:
            dist_out.append([new_ftrs, dist, weight])

    return dist_out


def _convert_distance_names_to_functions(distance):
    """
    Convert function names in a composite distance function into function
    handles.
    """
    dist_out = _copy.deepcopy(distance)

    for i, d in enumerate(distance):
        _, dist, _ = d
        if isinstance(dist, str):
            try:
                dist_out[i][1] = _tc.distances.__dict__[dist]
            except:
                raise ValueError("Distance '{}' not recognized.".format(dist))

    return dist_out


def _get_composite_distance_features(distance):
    """
    Return the union of feature names across all components in a composite
    distance specification.
    """
    return list(set(reduce(iadd, [x[0] for x in distance], [])))


def build_address_distance(number=None, street=None, city=None, state=None,
                           zip_code=None):
    """
    Construct a composite distance appropriate for matching address data. NOTE:
    this utility function does not guarantee that the output composite distance
    will work with a particular dataset and model. When the composite distance
    is applied in a particular context, the feature types and individual
    distance functions must be appropriate for the given model.

    Parameters
    ----------
    number, street, city, state, zip_code : string, optional
        Name of the SFrame column for the feature corresponding to the address
        component. Each feature name is mapped to an appropriate distance
        function and scalar multiplier.

    Returns
    -------
    dist : list
        A composite distance function, mapping sets of feature names to distance
        functions.

    Examples
    --------
    >>> homes = turicreate.SFrame({'sqft': [1230, 875, 1745],
    ...                          'street': ['phinney', 'fairview', 'cottage'],
    ...                          'city': ['seattle', 'olympia', 'boston'],
    ...                          'state': ['WA', 'WA', 'MA']})
    ...
    >>> my_dist = turicreate.distances.build_address_distance(street='street',
    ...                                                     city='city',
    ...                                                     state='state')
    >>> my_dist
    [[['street'], 'jaccard', 5],
     [['state'], 'jaccard', 5],
     [['city'], 'levenshtein', 1]]
    """

    ## Validate inputs
    for param in [number, street, city, state, zip_code]:
        if param is not None and not isinstance(param, str):
            raise TypeError("All inputs must be strings. Each parameter is " +
                            "intended to be the name of an SFrame column.")

    ## Figure out features for levenshtein distance.
    string_features = []

    if city:
        string_features.append(city)

    if zip_code:
        string_features.append(zip_code)


    ## Compile the distance components.
    dist = []

    if number:
        dist.append([[number], 'jaccard', 1])

    if street:
        dist.append([[street], 'jaccard', 5])

    if state:
        dist.append([[state], 'jaccard', 5])

    if len(string_features) > 0:
        dist.append([string_features, 'levenshtein', 1])

    return dist
