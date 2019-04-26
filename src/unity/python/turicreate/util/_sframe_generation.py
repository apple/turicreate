# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Wraps utilities for generating random sframes for testing and
benchmarking.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
def generate_random_sframe(num_rows, column_codes, random_seed = 0):
    """
    Creates a random SFrame with `num_rows` rows and randomly
    generated column types determined by `column_codes`.  The output
    SFrame is deterministic based on `random_seed`.
 
     `column_types` is a string with each character denoting one type
     of column, with the output SFrame having one column for each
     character in the string.  The legend is as follows:

        n:  numeric column, uniform 0-1 distribution. 
        N:  numeric column, uniform 0-1 distribution, 1% NaNs.
        r:  numeric column, uniform -100 to 100 distribution. 
        R:  numeric column, uniform -10000 to 10000 distribution, 1% NaNs.
        b:  binary integer column, uniform distribution
        z:  integer column with random integers between 1 and 10.
        Z:  integer column with random integers between 1 and 100.
        s:  categorical string column with 10 different unique short strings. 
        S:  categorical string column with 100 different unique short strings. 
        c:  categorical column with short string keys and 1000 unique values, triangle distribution.
        C:  categorical column with short string keys and 100000 unique values, triangle distribution.
        x:  categorical column with 128bit hex hashes and 1000 unique values. 
        X:  categorical column with 256bit hex hashes and 100000 unique values. 
        h:  column with unique 128bit hex hashes.
        H:  column with unique 256bit hex hashes.

        l:  categorical list with between 0 and 10 unique integer elements from a pool of 100 unique values. 
        L:  categorical list with between 0 and 100 unique integer elements from a pool of 1000 unique values.
        M:  categorical list with between 0 and 10 unique string elements from a pool of 100 unique values. 
        m:  categorical list with between 0 and 100 unique string elements from a pool of 1000 unique values.

        v:  numeric vector with 10 elements and uniform 0-1 elements.
        V:  numeric vector with 1000 elements and uniform 0-1 elements.
        w:  numeric vector with 10 elements and uniform 0-1 elements, 1% NANs.
        W:  numeric vector with 1000 elements and uniform 0-1 elements, 1% NANs.

        d: dictionary with with between 0 and 10 string keys from a
           pool of 100 unique keys, and random 0-1 values.

        D: dictionary with with between 0 and 100 string keys from a
           pool of 1000 unique keys, and random 0-1 values.

    For example::

      X = generate_random_sframe(10, 'nnv')

    will generate a 10 row SFrame with 2 floating point columns and
    one column of length 10 vectors.
    """

    from ..extensions import _generate_random_sframe

    assert isinstance(column_codes, str)
    assert isinstance(num_rows, int)
    assert isinstance(random_seed, int)
    
    X = _generate_random_sframe(num_rows, column_codes, random_seed, False, 0)
    X.__materialize__()
    return X

def generate_random_regression_sframe(num_rows, column_codes, random_seed = 0, target_noise_level = 0.25):
    """
    Creates a random SFrame with `num_rows` rows and randomly
    generated column types determined by `column_codes`.  The output
    SFrame is deterministic based on `random_seed`.  In addition, a
    target column is generated with values dependent on the randomly
    generated features in a given row.
 
     `column_types` is a string with each character denoting one type
     of column, with the output SFrame having one column for each
     character in the string.  The legend is as follows:

        n:  numeric column, uniform 0-1 distribution. 
        N:  numeric column, uniform 0-1 distribution, 1% NaNs.
        r:  numeric column, uniform -100 to 100 distribution. 
        R:  numeric column, uniform -10000 to 10000 distribution, 1% NaNs.
        b:  binary integer column, uniform distribution
        z:  integer column with random integers between 1 and 10.
        Z:  integer column with random integers between 1 and 100.
        s:  categorical string column with 10 different unique short strings. 
        S:  categorical string column with 100 different unique short strings. 
        c:  categorical column with short string keys and 1000 unique values, triangle distribution.
        C:  categorical column with short string keys and 100000 unique values, triangle distribution.
        x:  categorical column with 128bit hex hashes and 1000 unique values. 
        X:  categorical column with 256bit hex hashes and 100000 unique values. 
        h:  column with unique 128bit hex hashes.
        H:  column with unique 256bit hex hashes.

        l:  categorical list with between 0 and 10 unique integer elements from a pool of 100 unique values. 
        L:  categorical list with between 0 and 100 unique integer elements from a pool of 1000 unique values.
        M:  categorical list with between 0 and 10 unique string elements from a pool of 100 unique values. 
        m:  categorical list with between 0 and 100 unique string elements from a pool of 1000 unique values.

        v:  numeric vector with 10 elements and uniform 0-1 elements.
        V:  numeric vector with 1000 elements and uniform 0-1 elements.
        w:  numeric vector with 10 elements and uniform 0-1 elements, 1% NANs.
        W:  numeric vector with 1000 elements and uniform 0-1 elements, 1% NANs.

        d: dictionary with with between 0 and 10 string keys from a
           pool of 100 unique keys, and random 0-1 values.

        D: dictionary with with between 0 and 100 string keys from a
           pool of 1000 unique keys, and random 0-1 values.

    For example::

      X = generate_random_sframe(10, 'nnv')

    will generate a 10 row SFrame with 2 floating point columns and
    one column of length 10 vectors.
    
    Target Generation
    -----------------

    the target value is a linear
    combination of the features chosen for each row plus uniform noise.
    
    - For each numeric and vector columns, each value, with the range
      scaled to [-0.5, 0.5] (so r and R type values affect the target just
      as much as n an N), is added to the target value.  NaNs are ignored.
    
    - For each categorical or string values, it is hash-mapped to a lookup
      table of 512 randomly chosen values, each in [-0.5, 0.5], and the
      result is added to the target.
    
    - For dictionary columns, the keys are treated as adding a categorical
      value and the values are treated as adding a numeric value. 
    
    At the end, a uniform random value is added to the target in the
    range [(max_target - min_target) * noise_level], where max_target
    and min_target are the maximum and minimum target values generated
    by the above process.
   
    The final target values are then scaled to [0, 1].
    """

    from ..extensions import _generate_random_sframe

    assert isinstance(column_codes, str)
    assert isinstance(num_rows, int)
    assert isinstance(random_seed, int)
        
    X = _generate_random_sframe(num_rows, column_codes, random_seed, True, target_noise_level)
    X.__materialize__()
    return X
    
def generate_random_classification_sframe(num_rows, column_codes, num_classes,
                                          misclassification_spread = 0.25,
                                          num_extra_class_bins = None,
                                          random_seed = 0):
    """
    Creates a random SFrame with `num_rows` rows and randomly
    generated column types determined by `column_codes`.  The output
    SFrame is deterministic based on `random_seed`.  In addition, a
    target column is generated with values dependent on the randomly
    generated features in a given row.
 
     `column_types` is a string with each character denoting one type
     of column, with the output SFrame having one column for each
     character in the string.  The legend is as follows:

        n:  numeric column, uniform 0-1 distribution. 
        N:  numeric column, uniform 0-1 distribution, 1% NaNs.
        r:  numeric column, uniform -100 to 100 distribution. 
        R:  numeric column, uniform -10000 to 10000 distribution, 1% NaNs.
        b:  binary integer column, uniform distribution
        z:  integer column with random integers between 1 and 10.
        Z:  integer column with random integers between 1 and 100.
        s:  categorical string column with 10 different unique short strings. 
        S:  categorical string column with 100 different unique short strings. 
        c:  categorical column with short string keys and 1000 unique values, triangle distribution.
        C:  categorical column with short string keys and 100000 unique values, triangle distribution.
        x:  categorical column with 128bit hex hashes and 1000 unique values. 
        X:  categorical column with 256bit hex hashes and 100000 unique values. 
        h:  column with unique 128bit hex hashes.
        H:  column with unique 256bit hex hashes.

        l:  categorical list with between 0 and 10 unique integer elements from a pool of 100 unique values. 
        L:  categorical list with between 0 and 100 unique integer elements from a pool of 1000 unique values.
        M:  categorical list with between 0 and 10 unique string elements from a pool of 100 unique values. 
        m:  categorical list with between 0 and 100 unique string elements from a pool of 1000 unique values.

        v:  numeric vector with 10 elements and uniform 0-1 elements.
        V:  numeric vector with 1000 elements and uniform 0-1 elements.
        w:  numeric vector with 10 elements and uniform 0-1 elements, 1% NANs.
        W:  numeric vector with 1000 elements and uniform 0-1 elements, 1% NANs.

        d: dictionary with with between 0 and 10 string keys from a
           pool of 100 unique keys, and random 0-1 values.

        D: dictionary with with between 0 and 100 string keys from a
           pool of 1000 unique keys, and random 0-1 values.

    For example::

      X = generate_random_sframe(10, 'nnv')

    will generate a 10 row SFrame with 2 floating point columns and
    one column of length 10 vectors.
    
    Target Generation
    -----------------

    The target column, called "target", is an integer value that
    represents the binning of the output of a noisy linear function of
    the chosen random variables into `num_classes + num_extra_class_bins`
    bins, shuffled, and then each bin is mapped to a class.  This
    means that some non-linearity is present if num_extra_class_bins > 0.
    The default value for num_extra_class_bins is 2*num_classes.

    The `misclassification_probability` controls the spread of the
    binning -- if misclassification_spread equals 0.25, then a random
    variable of 0.25 * bin_width is added to the numeric prediction of
    the class, meaning the actual class may be mispredicted.
    """

    from ..extensions import _generate_random_classification_sframe
            
    if num_classes < 2:
        raise ValueError("num_classes must be >= 2.")

    if num_extra_class_bins is None:
        num_extra_class_bins = 2*num_classes

    if num_extra_class_bins < 0:
        raise ValueError("num_extra_class_bins must be >= 0.")

    if misclassification_spread < 0:
        raise ValueError("misclassification_spread must be >= 0.")
    
    assert isinstance(column_codes, str)
    assert isinstance(num_rows, int)
    assert isinstance(random_seed, int)
    assert isinstance(num_classes, int)
    assert isinstance(num_extra_class_bins, int)

            
    X = _generate_random_classification_sframe(
        num_rows, column_codes, random_seed,
        num_classes, num_extra_class_bins, misclassification_spread)
    
    X.__materialize__()
    return X
    
