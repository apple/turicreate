# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

def check_fitted(model, func):
    """Check if a model is fitted. Raise error if not.

    Parameters
    ----------
    model: model
        Any scikit-learn model

    func: model
        Function to check if a model is not trained.
    """
    if not func(model):
        raise TypeError("Expected a 'fitted' model for conversion")

def check_expected_type(model, expected_type):
    """Check if a model is of the right type. Raise error if not.

    Parameters
    ----------
    model: model
        Any scikit-learn model

    expected_type: Type
        Expected type of the scikit-learn.
    """
    if (model.__class__.__name__ != expected_type.__name__):
        raise TypeError("Expected model of type '%s' (got %s)" % \
                (expected_type.__name__, model.__class__.__name__))

