# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import turicreate as tc
from turicreate.toolkits.activity_classifier import _sframe_sequence_iterator as sframe_sequence_iterator


class SFrameActivityIteratorTest(unittest.TestCase):
    @classmethod
    def setUp(self):
        self.features = ['feature_1', 'feature_2']
        self.target = 'target'
        self.session_id = 'session_id'
        self.prediction_window = 2
        self.predictions_in_chunk = 5

        self.data = tc.SFrame({self.session_id: [0] * 15 + [1] * 30 + [2] * 45})
        self.data[self.features[0]] = 1
        self.data[self.features[1]] = 2
        self.data[self.target] = 0

    def test_prep_data(self):
        observed , num_sessions = sframe_sequence_iterator.prep_data(
            self.data, self.features, self.session_id, self.prediction_window,
            self.predictions_in_chunk, target=self.target)

        chunk_size = self.prediction_window * self.predictions_in_chunk
        chunk_targets = [0.0] * self.predictions_in_chunk
        full_chunk_weights = [1.0] * self.predictions_in_chunk
        padded_chunk_weights = [1.0] * 3 + [0.0] * 2

        full_chunk_features = [1.0, 2.0] * chunk_size
        padded_chunk_features = [1.0, 2.0] * 5 + [0.0, 0.0] * 5

        expected = tc.SFrame({
            'session_id': [0] * 2 + [1] * 3 + [2] * 5,
            'chunk_len': [10, 5, 10, 10, 10, 10, 10, 10, 10, 5],
            'features': [full_chunk_features, padded_chunk_features] +
                        [full_chunk_features] * 7 + [padded_chunk_features],
            'target': [chunk_targets] * 10,
            'weights': [full_chunk_weights, padded_chunk_weights] +
                       [full_chunk_weights] * 7 + [padded_chunk_weights]
        })

        tc.util._assert_sframe_equal(expected, observed, check_column_order=False)
        self.assertEqual(num_sessions , len(self.data[self.session_id].unique()))

    def test_ceil_dev(self):
        observed = sframe_sequence_iterator._ceil_dev(10, 3)
        expected = 4
        self.assertEqual(expected, observed)

        observed = sframe_sequence_iterator._ceil_dev(6, 2)
        expected = 3
        self.assertEqual(expected, observed)
