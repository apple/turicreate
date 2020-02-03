# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
# This software may be modified and distributed under the terms
# of the BSD license. See the LICENSE file for details.

# This file tests validity of the JSON export produced by SFrame/SArray.
# NOTE: Complex types like datetime and Image are likely broken.
# TODO: When https://github.com/apple/turicreate/issues/89 is fixed,
# also check inverse (exported JSON, when loaded, should produce original
# SFrame.)

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import json
import numpy as np
import struct
import tempfile
import unittest

import turicreate as tc

_TEST_CASE_SIZE = 1000


class JSONExporterTest(unittest.TestCase):

    # tests int/float/str
    def test_simple_types(self):
        np.random.seed(42)
        sf = tc.SFrame()
        sf["idx"] = range(_TEST_CASE_SIZE)
        sf["ints"] = np.random.randint(-100000, 100000, _TEST_CASE_SIZE)
        sf["strings"] = sf["ints"].astype(str)
        sf["floats"] = np.random.random(_TEST_CASE_SIZE)

        # TODO: nans and infs will break JSON - what should we do about this?
        # sf['nans_and_infs'] = sf['idx'].apply(lambda x: float('nan') if x > 0 else float('inf'))

        with tempfile.NamedTemporaryFile(mode="w", suffix=".json") as json_file:
            sf.save(json_file.name, format="json")
            with open(json_file.name) as json_data:
                # will throw if JSON export doesn't work
                loaded = json.load(json_data)

    def test_array_dtype(self):
        np.random.seed(42)
        sf = tc.SFrame()
        sf["arr"] = np.random.rand(100, 3)
        with tempfile.NamedTemporaryFile(mode="w", suffix=".json") as json_file:
            sf.save(json_file.name, format="json")
            with open(json_file.name) as json_data:
                # will throw if JSON export doesn't work
                loaded = json.load(json_data)
