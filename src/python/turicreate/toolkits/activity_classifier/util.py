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

import sys as _sys

if _sys.version_info.major > 2:
    long = int

_MIN_NUM_SESSIONS_FOR_SPLIT = 100


def random_split_by_session(dataset, session_id, fraction=0.9, seed=None):
    """
    Randomly split an SFrame into two SFrames based on the `session_id` such
    that one split contains all data for a `fraction` of the sessions while
    the second split contains all data for the rest of the sessions.

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
    from random import Random

    _raise_error_if_not_of_type(dataset, _SFrame, "dataset")
    _raise_error_if_not_of_type(session_id, str, "session_id")
    _raise_error_if_not_of_type(fraction, float, "fraction")
    _raise_error_if_not_of_type(seed, [int, type(None)], "seed")
    _numeric_param_check_range("fraction", fraction, 0, 1)

    if session_id not in dataset.column_names():
        raise _ToolkitError(
            'Input "dataset" must contain a column called %s.' % session_id
        )

    if seed is None:
        # Include the nanosecond component as well.
        import time

        seed = abs(hash("%0.20f" % time.time())) % (2 ** 31)

    # The cython bindings require this to be an int, so cast if we can.
    try:
        seed = int(seed)
    except ValueError:
        raise ValueError("The 'seed' parameter must be of type int.")

    random = Random()

    # Create a random binary filter (boolean SArray), using the same probability across all lines
    # that belong to the same session. In expectancy - the desired fraction of the sessions will
    # go to the training set.
    # Since boolean filters preserve order - there is no need to re-sort the lines within each session.
    # The boolean filter is a pseudorandom function of the session_id and the
    # global seed above, allowing the train-test split to vary across runs using
    # the same dataset.
    def random_session_pick(session_id_hash):
        random.seed(session_id_hash)
        return random.uniform(0, 1) < fraction

    chosen_filter = dataset[session_id].hash(seed).apply(random_session_pick)

    train = dataset[chosen_filter]
    valid = dataset[1 - chosen_filter]
    return train, valid
