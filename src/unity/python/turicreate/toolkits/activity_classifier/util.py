# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from turicreate import SFrame as _SFrame
from turicreate.util import _raise_error_if_not_of_type
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._internal_utils import _numeric_param_check_range
from random import Random

_MIN_NUM_SESSIONS_FOR_SPLIT = 100

def random_split_by_session(dataset, session_id, fraction=0.9, seed=None):
    """
    Randomly split an SFrame into two SFrames based on the `session_id` such
    that one split contains data for a `fraction` of the sessions while the
    second split contains all data for the rest of the sessions.

    Parameters
    ----------
    dataset : SFrame
        Dataset to split. It must contain a column of session ids.

    session_id : string, optional
        The name of the column in `dataset` that corresponds to the
        a unique identifier for each session.

    fraction : float, optional
        Fraction of the sessions to fetch for the first returned SFrame.  Must
        be between 0 and 1. Once the sessions are split, all data from a single
        session is in the same SFrame.

    seed : int, optional
        Seed for the random number generator used to split.

    Examples
    --------

    .. sourcecode:: python

        # Split the data so that train has 90% of the users.
        >>> train, valid = tc.activity_classifier.util.random_split_by_session(
        ...     dataset, session_id='session_id', fraction=0.9)

        # For example: If dataset has 2055 sessions
        >>> len(dataset['session_id'].unique())
        2055

        # The training set now has 90% of the sessions
        >>> len(train['session_id'].unique())
        1850

        # The validation set has the remaining 10% of the sessions
        >>> len(valid['session_id'].unique())
        205
    """

    _raise_error_if_not_of_type(dataset, _SFrame, 'dataset')
    _raise_error_if_not_of_type(session_id, str, 'session_id')
    _raise_error_if_not_of_type(fraction, float, 'fraction')
    _raise_error_if_not_of_type(seed, [int, type(None)], 'seed')
    _numeric_param_check_range('fraction', fraction, 0, 1)

    if session_id not in dataset.column_names():
        raise _ToolkitError(
            'Input "dataset" must contain a column called %s.' % session_id)

    unique_sessions = _SFrame({'session': dataset[session_id].unique()})
    if len(unique_sessions) < _MIN_NUM_SESSIONS_FOR_SPLIT:
        print ("The dataset has less than the minimum of", _MIN_NUM_SESSIONS_FOR_SPLIT, "sessions required for train-validation split. Continuing without validation set")
        return dataset, None

    # We need an actual seed number, which we will later use in the apply function (see below).
    # If the user didn't provide a seed - we can generate one based on current system time
    # (similarly to mechanism behind random.seed(None) )
    if seed is None:
        import time
        seed = long(time.time() * 256)
    
    random = Random()
    
    # Create a random binary filter (boolean SArray), using the same probability across all lines
    # that belong to the same session. In expectancy - the desired fraction of the sessions will
    # go to the training set.
    # Since boolean filters preserve order - there is no need to re-sort the lines within each session.
    def random_session_pick(session_id):
        # If we will use only the session_id as the seed - the split will be constant for the
        # same dataset across different runs, which is of course undesired
        random.seed(hash(session_id) + seed)
        return random.uniform(0, 1) < fraction
    
    chosen_filter = dataset[session_id].apply(random_session_pick)
    train = dataset[chosen_filter]
    valid = dataset[1 - chosen_filter]
    return train, valid