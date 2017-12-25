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
    chosen, not_chosen = unique_sessions.random_split(fraction, seed)
    train = dataset.filter_by(chosen['session'], session_id)
    valid = dataset.filter_by(not_chosen['session'], session_id)
    return train, valid
