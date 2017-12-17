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
import numpy as np
from turicreate.toolkits.activity_classifier import _sframe_sequence_iterator as sframe_sequence_iterator


class SFrameActivityIteratorTest(unittest.TestCase):
    @classmethod
    def setUp(self):
        self.features = ['feature_1', 'feature_2']
        self.target = 'target'
        self.session_id = 'session_id'
        self.prediction_window = 3
        self.predictions_in_chunk = 2
        self.data_length = 18
        self.num_sessions = 3

        self.data = tc.SFrame({'feature_1': range(self.data_length), 'feature_2': range(self.data_length), 'session_id': ["s1"] * 4 + ["s2"] * 6 + ["s3"] * 8})
        self.data['feature_2'] *= 10
        self.data['target'] = [1, 1, 2, 2, 1, 2, 1, 3, 3, 3, 1, 1, 2, 3, 3, 2, 2, 2]

        self._build_expected_outputs()

    def test_prep_data_case_1(self):
        # This case, which uses prediction_window = 3, chunk_len = 6, covers edge cases:
        # One session whos length < chunk_len, and length % p_d == 0
        # One session whos length == chunk_len
        # One session > chunk_len. Second part << chunk_len and == p_d
        # Last session % chunk_len != 0
        observed , num_sessions = sframe_sequence_iterator.prep_data(
            self.data, self.features, self.session_id, self.prediction_window,
            self.predictions_in_chunk, target=self.target)

        print (observed.column_types())
        print (self.expected_chunked_3_2.column_types())

        tc.util._assert_sframe_equal(self.expected_chunked_3_2, observed, check_column_order=False)
        self.assertEqual(num_sessions , len(self.data[self.session_id].unique()))

    def test_prep_data_case_2(self):
        # This case, which uses prediction_window = 2, chunk_len = 6, covers edge cases:
        # One session whos length < chunk_len, and length % p_d != 0
        # One session whos length == chunk_len, , tie within to p_d
        # One session > chunk_len. Second part << chunk_len and < p_d
        # Last session % chunk_len != 0 (same as case 1)
        observed , num_sessions = sframe_sequence_iterator.prep_data(
            self.data, self.features, self.session_id, 2, 3, target=self.target)

        tc.util._assert_sframe_equal(self.expected_chunked_2_3, observed, check_column_order=False)
        self.assertEqual(num_sessions , len(self.data[self.session_id].unique()))

    def test_prep_data_case_3(self):
        # This case, which uses prediction_window = 4, chunk_len = 8, covers edge cases:
        # One session with exactly one p_d
        # Last session % chunk_len == 0
        observed , num_sessions = sframe_sequence_iterator.prep_data(
            self.data, self.features, self.session_id, 4, 2, target=self.target)


        tc.util._assert_sframe_equal(self.expected_chunked_4_2, observed, check_column_order=False)
        self.assertEqual(num_sessions , len(self.data[self.session_id].unique()))

    def test_ceil_dev(self):
        observed = sframe_sequence_iterator._ceil_dev(10, 3)
        expected = 4
        self.assertEqual(expected, observed)

        observed = sframe_sequence_iterator._ceil_dev(6, 2)
        expected = 3
        self.assertEqual(expected, observed)

    @classmethod
    def _build_expected_outputs(self):
        # 3 , 2
        # session < chunk
        # window < p_d
        # session == chunk
        # last session % chunk != 0
        builder = tc.SFrameBuilder([np.ndarray , int , str , np.ndarray , np.ndarray] , ['features' , 'chunk_len' , 'session_id' , 'target' , 'weights'])
        builder.append([[0, 0, 1, 10, 2, 20, 3, 30] + [0] * 4 , 4 , 's1' , [1 , 2] , [1 , 1] ])
        builder.append([[4, 40, 5, 50, 6, 60, 7, 70, 8, 80, 9, 90] , 6 , 's2' , [1 , 3], [1 , 1] ])
        builder.append([[10, 100, 11, 110, 12, 120, 13, 130, 14, 140, 15, 150] , 6 , 's3' , [1 , 3], [1 , 1] ])
        builder.append([[16, 160, 17, 170] + [0] * 8 , 2 , 's3' , [2 , 0], [1 , 0] ])
        self.expected_chunked_3_2 = builder.close()

        # 2, 3
        #
        builder = tc.SFrameBuilder([np.ndarray , int , str , np.ndarray , np.ndarray] , ['features' , 'chunk_len' , 'session_id' , 'target' , 'weights'])
        builder.append([[0, 0, 1, 10, 2, 20, 3, 30] + [0] * 4 , 4 , 's1' , [1 , 2 , 0] , [1 , 1 , 0] ])
        builder.append([[4, 40, 5, 50, 6, 60, 7, 70, 8, 80, 9, 90] , 6 , 's2' , [1 , 1 , 3], [1 , 1 , 1] ])
        builder.append([[10, 100, 11, 110, 12, 120, 13, 130, 14, 140, 15, 150] , 6 , 's3' , [1 ,2, 3], [1 , 1, 1] ])
        builder.append([[16, 160, 17, 170] + [0] * 8, 2 , 's3' , [2 ,0 , 0], [1 , 0, 0] ])
        self.expected_chunked_2_3 = builder.close()

        # 4, 2
        # session with exactly one p_d
        # last session == chunk_len
        # window < half of p_d

        builder = tc.SFrameBuilder([np.ndarray , int , str , np.ndarray , np.ndarray] , ['features' , 'chunk_len' , 'session_id' , 'target' , 'weights'])
        builder.append([[0, 0, 1, 10, 2, 20, 3, 30] + [0] * 8 , 4 , 's1' , [1 , 0] , [1 , 0] ])
        builder.append([[4, 40, 5, 50, 6, 60, 7, 70, 8, 80, 9, 90] + [0] * 4, 6 , 's2' , [1 , 3], [1 , 1] ])
        builder.append([[10, 100, 11, 110, 12, 120, 13, 130, 14, 140, 15, 150, 16, 160, 17, 170] , 8 , 's3' , [1 , 2], [1 , 1] ])
        self.expected_chunked_4_2 = builder.close()

        # num_padded_lines = 18
        # padding_for_batch_size_32 = tc.SFrame({'features' : [[0] * 12] * num_padded_lines , 'chunk_len': [0] * num_padded_lines,
        #                                        'session_id': ['na'] * num_padded_lines , })
        # self.expected_batch_size_32_batch_1