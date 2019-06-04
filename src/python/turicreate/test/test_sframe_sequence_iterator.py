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
import array
from turicreate.toolkits.activity_classifier import _sframe_sequence_iterator as sframe_sequence_iterator


class SFrameActivityIteratorTest(unittest.TestCase):
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

        self._prepare_expected_chunked_dataset()
        self._prepare_expected_batches()

    def test_prep_data_case_1(self):
        # This case, which uses prediction_window = 3, chunk_len = 6, covers edge cases:
        # One session whos length < chunk_len, and length % p_w == 0
        # One session whos length == chunk_len
        # One session > chunk_len. Second part << chunk_len and == p_w
        # Last session % chunk_len != 0
        observed , num_sessions = sframe_sequence_iterator.prep_data(
            self.data, self.features, self.session_id, self.prediction_window,
            self.predictions_in_chunk, target=self.target)

        tc.util._assert_sframe_equal(self.expected_chunked_3_2, observed, check_column_order=False)
        self.assertEqual(num_sessions , len(self.data[self.session_id].unique()))

    def test_prep_data_case_2(self):
        # This case, which uses prediction_window = 2, chunk_len = 6, covers edge cases:
        # One session whos length < chunk_len, and length % p_w != 0
        # One session whos length == chunk_len, , tie within to p_w
        # One session > chunk_len. Second part << chunk_len and < p_w
        # Last session % chunk_len != 0 (same as case 1)
        observed , num_sessions = sframe_sequence_iterator.prep_data(
            self.data, self.features, self.session_id, 2, 3, target=self.target)

        tc.util._assert_sframe_equal(self.expected_chunked_2_3, observed, check_column_order=False)
        self.assertEqual(num_sessions , len(self.data[self.session_id].unique()))

    def test_prep_data_case_3(self):
        # This case, which uses prediction_window = 4, chunk_len = 8, covers edge cases:
        # One session with exactly one p_w
        # Last session % chunk_len == 0
        observed , num_sessions = sframe_sequence_iterator.prep_data(
            self.data, self.features, self.session_id, 4, 2, target=self.target)

        tc.util._assert_sframe_equal(self.expected_chunked_4_2, observed, check_column_order=False)
        self.assertEqual(num_sessions , len(self.data[self.session_id].unique()))

    def test_prep_data_no_target(self):
        observed , num_sessions = sframe_sequence_iterator.prep_data(
            self.data, self.features, self.session_id, self.prediction_window,
            self.predictions_in_chunk)

        expected = self.expected_chunked_3_2.remove_columns(['target' , 'weights'])
        tc.util._assert_sframe_equal(expected, observed, check_column_order=False)
        self.assertEqual(num_sessions , len(self.data[self.session_id].unique()))

    def test_ceil_dev(self):
        observed = sframe_sequence_iterator._ceil_dev(10, 3)
        expected = 4
        self.assertEqual(expected, observed)

        observed = sframe_sequence_iterator._ceil_dev(6, 2)
        expected = 3
        self.assertEqual(expected, observed)

    def test_next_batch_size_32_no_target(self):
        # The chunked dataset is smaller than batch size, first batch will be padded
        self._test_next(32 , 1 , self.expected_batches_32, use_taget=False)

    def test_next_batch_size_32_with_target(self):
        # The chunked dataset is smaller than batch size, first batch will be padded
        self._test_next(32 , 1 , self.expected_batches_32)

    def test_next_batch_size_3(self):
        # The chunked dataset length > batch size and < 2 * batch_size, the second batch will be padded
        self._test_next(3 , 2 , self.expected_batches_3)

    def test_next_batch_size_3_with_padding(self):
        self._test_next(3 , 2 , self.expected_batches_3, use_pad=True)

    def test_next_batch_size_4_with_padding(self):
        # The chunked dataset length == batch size
        self._test_next(4 , 1 , self.expected_batches_4)

    def _test_next(self , batch_size , expected_num_batches , expected_batches, use_taget=True, use_pad=False):
        chunked , num_sessions = sframe_sequence_iterator.prep_data(
            self.data, self.features, self.session_id, self.prediction_window,
            self.predictions_in_chunk, target=self.target)

        seq_iter = sframe_sequence_iterator.SFrameSequenceIter(chunked , len(self.features) , self.prediction_window,
                                                               self.predictions_in_chunk , batch_size ,
                                                               use_target=use_taget, use_pad=use_pad)

        self.assertTrue(seq_iter.batch_size == batch_size)
        self.assertTrue(seq_iter.num_batches == expected_num_batches)

        for batch_num ,expected_batch in enumerate(expected_batches):
            observed_batch = seq_iter.next()

            np.testing.assert_array_equal(expected_batch['features'] , observed_batch.data[0].asnumpy(),
                                          "Error - features in batch %d" % batch_num)

            if use_taget:
                np.testing.assert_array_equal(expected_batch['target'] , observed_batch.label[0].asnumpy(),
                                              "Error - target in batch %d" % batch_num)

                np.testing.assert_array_equal(expected_batch['weights'] , observed_batch.label[1].asnumpy(),
                                              "Error - weights in batch %d" % batch_num)
            else:
                self.assertTrue(observed_batch.label is None)

            if use_pad and ("padding" in expected_batch):
                self.assertTrue(observed_batch.pad == expected_batch["padding"])
            else:
                self.assertTrue(observed_batch.pad == 0)

        # The iterator length should be in the same length as the expected_batches list.
        # We expect a "StopIteration" exception to be raised on the next call to next()
        stop_iteration_raised = False
        try:
            seq_iter.next()
        except StopIteration:
            stop_iteration_raised = True
        self.assertTrue(stop_iteration_raised , "Error - the iterator is longer than expected")

    def _prepare_expected_chunked_dataset(self):
        # Expcted result of prep_data with p_w = 3, predictions_in_chunk = 2
        builder = tc.SFrameBuilder([array.array , int , str , array.array , array.array] , ['features' , 'chunk_len' , 'session_id' , 'target' , 'weights'])
        builder.append([[0, 0, 1, 10, 2, 20, 3, 30] + [0] * 4 , 4 , 's1' , [1 , 2] , [1 , 1] ])
        builder.append([[4, 40, 5, 50, 6, 60, 7, 70, 8, 80, 9, 90] , 6 , 's2' , [1 , 3], [1 , 1] ])
        builder.append([[10, 100, 11, 110, 12, 120, 13, 130, 14, 140, 15, 150] , 6 , 's3' , [1 , 3], [1 , 1] ])
        builder.append([[16, 160, 17, 170] + [0] * 8 , 2 , 's3' , [2 , 0], [1 , 0] ])
        self.expected_chunked_3_2 = builder.close()

        # Expcted result of prep_data with p_w = 2, predictions_in_chunk = 3
        builder = tc.SFrameBuilder([array.array , int , str , array.array , array.array] , ['features' , 'chunk_len' , 'session_id' , 'target' , 'weights'])
        builder.append([[0, 0, 1, 10, 2, 20, 3, 30] + [0] * 4 , 4 , 's1' , [1 , 2 , 0] , [1 , 1 , 0] ])
        builder.append([[4, 40, 5, 50, 6, 60, 7, 70, 8, 80, 9, 90] , 6 , 's2' , [1 , 1 , 3], [1 , 1 , 1] ])
        builder.append([[10, 100, 11, 110, 12, 120, 13, 130, 14, 140, 15, 150] , 6 , 's3' , [1 ,2, 2], [1 , 1, 1] ])
        builder.append([[16, 160, 17, 170] + [0] * 8, 2 , 's3' , [2 ,0 , 0], [1 , 0, 0] ])
        self.expected_chunked_2_3 = builder.close()

        # Expcted result of prep_data with p_w = 4, predictions_in_chunk = 2
        builder = tc.SFrameBuilder([array.array , int , str , array.array , array.array] , ['features' , 'chunk_len' , 'session_id' , 'target' , 'weights'])
        builder.append([[0, 0, 1, 10, 2, 20, 3, 30] + [0] * 8 , 4 , 's1' , [1 , 0] , [1 , 0] ])
        builder.append([[4, 40, 5, 50, 6, 60, 7, 70, 8, 80, 9, 90] + [0] * 4, 6 , 's2' , [1 , 3], [1 , 1] ])
        builder.append([[10, 100, 11, 110, 12, 120, 13, 130, 14, 140, 15, 150, 16, 160, 17, 170] , 8 , 's3' , [1 , 2], [1 , 1] ])
        self.expected_chunked_4_2 = builder.close()

    def _prepare_expected_batches(self):
        chunk_len = self.prediction_window * self.predictions_in_chunk
        num_features = len(self.features)

        # Convert prep_data's expected output to numpy, for comparison with the output of the iterator's next()
        # Reshape them to the same format as the iterator returns
        expected_features_np = self.expected_chunked_3_2['features'].to_numpy()
        expected_features_np = np.reshape(expected_features_np , (-1 , chunk_len, num_features))
        expected_target_np = self.expected_chunked_3_2['target'].to_numpy()
        expected_target_np = np.reshape(expected_target_np , (-1 , self.predictions_in_chunk, 1))
        expected_weights_np = self.expected_chunked_3_2['weights'].to_numpy()
        expected_weights_np = np.reshape(expected_weights_np , (-1 , self.predictions_in_chunk, 1))

        def pad_batch(batch , num_padded_lines):
            batch['features'] = np.concatenate((batch['features'], np.array([[[0] * num_features] * chunk_len] * num_padded_lines)))
            batch['target'] = np.concatenate((batch['target'], [[[0]] * self.predictions_in_chunk]* num_padded_lines))
            batch['weights'] = np.concatenate((batch['weights'], [[[0]] * self.predictions_in_chunk]* num_padded_lines))
            batch['padding'] = num_padded_lines

        ## ----- batch size 32 ------
        num_padded_lines = 32 - len(self.expected_chunked_3_2)
        batch_0 = {'features' : expected_features_np , 'target' : expected_target_np , 'weights' : expected_weights_np}
        pad_batch(batch_0 , num_padded_lines)

        self.expected_batches_32 = [batch_0]

        ## ----- batch size 3 ------
        batch_0 = {'features' : expected_features_np[0:3] , 'target' : expected_target_np[0:3] , 'weights' : expected_weights_np[0:3]}
        batch_1 = {'features' : expected_features_np[3:] , 'target' : expected_target_np[3:] , 'weights' : expected_weights_np[3:]}
        pad_batch(batch_1 , 2)

        self.expected_batches_3 = [batch_0 , batch_1]

        ## ----- batch size 4 ------
        batch_0 = {'features' : expected_features_np[0:4] , 'target' : expected_target_np[0:4] , 'weights' : expected_weights_np[0:4]}
        self.expected_batches_4 = [batch_0]





