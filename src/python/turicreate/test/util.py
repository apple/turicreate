# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import random
import tempfile
import shutil
import math
import string
import numpy as np
from pandas.util.testing import assert_frame_equal
from .. import SArray
import turicreate as tc
import sys
from six import StringIO


class SFrameComparer:
    """
    Helper class for comparing sframe and sarrays

    Adapted from test_sframe.py
    """

    def _assert_sgraph_equal(self, sg1, sg2):
        self._assert_sframe_equal(sg1.vertices, sg2.vertices)
        self._assert_sframe_equal(sg1.edges, sg2.edges)

    def _assert_sframe_equal(self, sf1, sf2):
        assert sf1.num_rows() == sf2.num_rows()
        assert sf1.num_columns() == sf2.num_columns()
        assert set(sf1.column_names()) == set(sf2.column_names())
        assert_frame_equal(sf1.to_dataframe(), sf2.to_dataframe())

    def _assert_sarray_equal(self, sa1, sa2):
        l1 = list(sa1)
        l2 = list(sa2)
        assert len(l1) == len(l2)
        for i in range(len(l1)):
            v1 = l1[i]
            v2 = l2[i]
            if v1 is None:
                assert v2 is None
            else:
                if type(v1) == dict:
                    assert len(v1) == len(v2)
                    for key in v1:
                        assert key in v1
                        assert v1[key] == v2[key]

                elif hasattr(v1, "__iter__"):
                    assert len(v1) == len(v2)
                    for j in range(len(v1)):
                        t1 = v1[j]
                        t2 = v2[j]
                        if type(t1) == float:
                            if math.isnan(t1):
                                assert math.isnan(t2)
                            else:
                                assert t1 == t2
                        else:
                            assert t1 == t2
                else:
                    assert v1 == v2


class SubstringMatcher:
    """
    Helper class for testing substring matching

    Code adapted from http://www.michaelpollmeier.com/python-mock-how-to-assert-a-substring-of-logger-output/
    """

    def __init__(self, containing):
        self.containing = containing.lower()

    def __eq__(self, other):
        return other.lower().find(self.containing) > -1

    def __unicode__(self):
        return 'a string containing "%s"' % self.containing

    def __str__(self):
        return unicode(self).encode("utf-8")

    __repr__ = __unicode__


class TempDirectory:
    name = None

    def __init__(self):
        self.name = tempfile.mkdtemp()

    def __enter__(self):
        return self.name

    def __exit__(self, type, value, traceback):
        if self.name is not None:
            shutil.rmtree(self.name)


def uniform_string_column(n, word_length, alphabet_size, missingness=0.0):
    """
    Return an SArray of strings constructed uniformly randomly from the first
    'num_letters' of the lower case alphabet.

    Parameters
    ----------
    n : int
        Number of entries in the output SArray.

    word_length : int
        Number of characters in each string.

    alphabet_size : int
        Number of characters in the alphabet.

    missingness : float, optional
        Probability that a given entry in the output is missing.

    Returns
    -------
    out : SArray
        One string "word" in each entry of the output SArray.
    """
    result = []
    letters = list(string.ascii_letters[:alphabet_size])

    for i in range(n):
        missing_flag = random.random()
        if missing_flag < missingness:
            result.append(None)
        else:
            word = []
            for j in range(word_length):
                word.append(np.random.choice(letters))
            result.append("".join(word))

    return SArray(result)


def uniform_numeric_column(n, col_type=float, range=(0, 1), missingness=0.0):
    """
    Return an SArray of uniformly random numeric values.

    Parameters
    ----------
    n : int
        Number of entries in the output SArray.

    col_type : type, optional
        Type of the output SArray. Default is floats.

    range : tuple[int, int], optional
        Minimum and maximum of the uniform distribution from which values are
        chosen.

    missingness : float, optional
        Probability that a given entry in the output is missing.

    Returns
    -------
    out : SArray
    """
    if col_type == int:
        v = np.random.randint(low=range[0], high=range[1], size=n).astype(float)
    else:
        v = np.random.rand(n)
        v = v * (range[1] - range[0]) + range[0]

    idx_na = np.random.rand(n) < missingness
    v[idx_na] = None
    v = np.where(np.isnan(v), None, v)

    return SArray(v, dtype=col_type)


def assert_longer_verbose_logs(function_call, args, kwargs):
    # Run command with verbose=False
    kwargs["verbose"] = False
    old_stdout = sys.stdout
    sys.stdout = stdout_without_verbose = StringIO()
    function_call(*args, **kwargs)
    sys.stdout = old_stdout
    without_verbose = stdout_without_verbose.getvalue()
    # Run command with verbose=True
    kwargs["verbose"] = True
    old_stdout = sys.stdout
    sys.stdout = stdout_with_verbose = StringIO()
    function_call(*args, **kwargs)
    sys.stdout = old_stdout
    with_verbose = stdout_with_verbose.getvalue()
    # Assert that verbose logs are longer
    assert len(with_verbose) > len(without_verbose)
