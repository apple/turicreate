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
import random
import tempfile
import platform
import math
import numpy as np
from numbers import Number
from . import util as test_util
import pytest
from turicreate.toolkits._internal_utils import _mac_ver


def _load_data(self):
    random.seed(42)

    self.num_examples = 1000
    self.num_features = 3
    self.num_sessions = random.randint(1, 4)
    self.num_labels = 9
    self.prediction_window = 5

    self.features = ['X1-r', 'X2-r', 'X3-r']
    self.target = 'activity_label'
    self.session_id = 'session_id'

    random_session_ids = sorted([random.randint(0, self.num_sessions - 1) for i in range(self.num_examples)])
    random_labels = [random.randint(0, self.num_labels - 1) for i in range(self.num_examples)]

    self.data = tc.util.generate_random_sframe(column_codes='r' * self.num_features, num_rows=self.num_examples)
    self.data['session_id'] = random_session_ids
    self.data[self.target] = random_labels


class ActivityClassifierCreateStressTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        _load_data(self)

    def test_create_none_validation_set(self):
        model = tc.activity_classifier.create(self.data,
                            features=self.features ,
                            target=self.target,
                            session_id=self.session_id,
                            prediction_window=self.prediction_window,
                            validation_set=None)
        predictions = model.predict(self.data)


    def test_create_no_validation_set(self):
        model = tc.activity_classifier.create(self.data,
                            features=self.features ,
                            target=self.target,
                            session_id=self.session_id,
                            prediction_window=self.prediction_window)
        predictions = model.predict(self.data)


    def test_create_features_target_session(self):
        model = tc.activity_classifier.create(self.data,
                            features=self.features ,
                            target=self.target,
                            session_id=self.session_id)
        predictions = model.predict(self.data)


    def test_create_target_session(self):
        model = tc.activity_classifier.create(self.data,
                            target=self.target,
                            session_id=self.session_id)
        predictions = model.predict(self.data)

    @pytest.mark.xfail(raises = RuntimeError)
    def test_invalid_model(self):
        """
        Verify that creating a model with wrong fields fails
        """
        model = tc.activity_classifier.create(self.data,
                                              features = self.features,
                                              target ='wrong' ,
                                              session_id=self.session_id,
                                              prediction_window=self.prediction_window,
                                              validation_set=None)

class ActivityClassifierTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        """
        The setup class method for the basic test case with all default values.
        """
        _load_data(self)

        # Create the model
        self.model = tc.activity_classifier.create(self.data,
                                                   features=self.features ,
                                                   target=self.target,
                                                   session_id=self.session_id,
                                                   prediction_window=self.prediction_window,
                                                   validation_set=None)

        self.def_opts = {
            'verbose': True,
            'prediction_window': 100,
            'max_iterations': 10,
            'batch_size' : 32
        }

        # Answers
        self.opts = self.def_opts.copy()
        self.opts['prediction_window'] = self.prediction_window

        self.get_ans = {
            'features': lambda x: x == self.features,
            'training_time': lambda x: x > 0,
            'target': lambda x: x == self.target,
            'verbose': lambda x: x == True,
            'session_id': lambda x: x == self.session_id,
            'prediction_window': lambda x: x == self.prediction_window,
            'training_accuracy': lambda x: x >= 0 and x <= 1 ,
            'training_log_loss': lambda x: isinstance(x , Number),
            'max_iterations': lambda x: x == self.def_opts['max_iterations'],
            'num_sessions': lambda x: x == self.num_sessions,
            'num_features': lambda x: x == self.num_features,
            'num_examples': lambda x: x == self.num_examples,
            'num_classes': lambda x: x == self.num_labels,
            'batch_size' : lambda x: x == self.def_opts['batch_size'],
            'classes': lambda x: sorted(x) == sorted(self.data[self.target].unique())
        }
        self.exposed_fields_ans = self.get_ans.keys()
        self.fields_ans = self.exposed_fields_ans + ['_recalibrated_batch_size',
                '_loss_model', '_pred_model', '_id_target_map',
                '_predictions_in_chunk', '_target_id_map']



    def _calc_expected_predictions_length(self , predict_input , top_k = 1):

        input_sessions = predict_input.groupby(self.session_id , { 'session_len' : tc.aggregate.COUNT()})
        prediction_window = self.model.prediction_window
        input_sessions['num_predictions_per_session'] = input_sessions['session_len'].apply(
            lambda x: math.ceil(float(x) / prediction_window) )
        total_num_of_prediction = sum(input_sessions['num_predictions_per_session']) * top_k

        return total_num_of_prediction

    def test_predict(self):
        """
        Check the predict() function.
        """
        model = self.model
        for output_type in ['probability_vector', 'class']:
            preds = model.predict(
                self.data, output_type=output_type, output_frequency='per_window')
            expected_len = self._calc_expected_predictions_length(self.data)
            self.assertEqual(len(preds), expected_len)

    def test_export_coreml(self):
        """
        Check the export_coreml() function.
        """
        import coremltools
        # Save the model as a CoreML model file
        filename = tempfile.mkstemp('ActivityClassifier.mlmodel')[1]
        self.model.export_coreml(filename)

        # Load the model back from the CoreML model file
        coreml_model = coremltools.models.MLModel(filename)

        rs = np.random.RandomState(1234)

        # Create a small dataset, and compare the models' predict() output
        dataset = tc.util.generate_random_sframe(column_codes='r' * 3, num_rows=10)
        dataset['session_id'] = 0
        dataset[self.target] = random_labels = [rs.randint(0, self.num_labels - 1, ) for i in range(10)]

        if _mac_ver() >= (10, 13):
            w = self.prediction_window
            labels = map(str, sorted(self.model._target_id_map.keys()))

            data_list = [dataset[f].to_numpy()[:, np.newaxis] for f in self.features]
            np_data = np.concatenate(data_list, 1)[np.newaxis]

            pred = self.model.predict(dataset, output_type='probability_vector')
            model_time0_values = pred[0]
            model_time1_values = pred[w]
            model_predictions = np.array([model_time0_values, model_time1_values])

            ret0 = coreml_model.predict({'features' : np_data[:, :w].copy()})

            ret1 = coreml_model.predict({'features' : np_data[:, w:2*w].copy(),
                                         'hiddenIn': ret0['hiddenOut'],
                                         'cellIn': ret0['cellOut']})

            coreml_time0_values = [ret0[self.target + 'Probability'][l] for l in labels]
            coreml_time1_values = [ret1[self.target + 'Probability'][l] for l in labels]
            coreml_predictions = np.array([coreml_time0_values, coreml_time1_values])

            np.testing.assert_array_almost_equal(model_predictions, coreml_predictions, decimal=3)

    def test_classify(self):
        """
        Check the classify() function.
        """
        model = self.model
        preds = model.classify(self.data, output_frequency='per_window')
        expected_len = self._calc_expected_predictions_length(self.data)
        self.assertEqual(len(preds), expected_len)

    def test_predict_topk(self):
        """
        Check the predict_topk function.
        """
        model = self.model
        for output_type in ['rank', 'probability']:
            preds = model.predict_topk(
                self.data, output_type=output_type, output_frequency='per_window')
            expected_len = self._calc_expected_predictions_length(self.data, top_k=3)
            self.assertEqual(len(preds), expected_len)

            preds = model.predict_topk(
                self.data.head(100), k=5, output_frequency='per_window')
            expected_len = self._calc_expected_predictions_length(self.data.head(100), top_k=5)
            self.assertEqual(len(preds), expected_len)

    def test__list_fields(self):
        """
        Check the list fields function.
        """
        model = self.model
        fields = model._list_fields()
        self.assertEqual(set(fields), set(self.fields_ans))

    def test_get(self):
        """
        Check the get function. Compare with the answer supplied as a lambda
        function for each field.
        """
        model = self.model
        for field in self.exposed_fields_ans:
            ans = model._get(field)
            self.assertTrue(self.get_ans[field](ans),
                    '''Get failed in field {}. Output was {}.'''.format(field, ans))

    def test_summary(self):
        """
        Check the summary function.
        """
        model = self.model
        model.summary()

    def test_repr(self):
        """
        Check the repr function.
        """
        # Repr after fit
        model = self.model
        self.assertEqual(type(str(model)), str)
        self.assertEqual(type(model.__repr__()), str)

    def test_save_and_load(self):
        """
        Make sure saving and loading retains everything.
        """
        test_methods_list = [func for func in dir(self) if callable(getattr(self, func)) and func.startswith("test")]
        test_methods_list.remove('test_save_and_load')

        with test_util.TempDirectory() as filename:
            self.model.save(filename)
            self.model = None
            self.model = tc.load_model(filename)

            print ("Repeating all test cases after model delete and reload")
            for test_method in test_methods_list:
                try:
                    getattr(self, test_method)()
                    print("Save and Load:", test_method, "has passed")
                except unittest.SkipTest:
                    pass
                except Exception as e:
                    self.assertTrue(False, "After model save and load, method " + test_method +
                                    " has failed with error: " + e.message)


@unittest.skipIf(tc.util._num_available_gpus() == 0, 'Requires GPU')
@pytest.mark.gpu
class ActivityClassifierGPUTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        _load_data(self)

    def test_gpu_save_load_export(self):
        old_num_gpus = tc.config.get_num_gpus()
        gpu_options = set([old_num_gpus, 0, 1])
        for in_gpus in gpu_options:
            for out_gpus in gpu_options:
                tc.config.set_num_gpus(in_gpus)
                model = tc.activity_classifier.create(self.data,
                                    target=self.target,
                                    session_id=self.session_id)
                with test_util.TempDirectory() as filename:
                    model.save(filename)
                    tc.config.set_num_gpus(out_gpus)
                    model = tc.load_model(filename)

                with test_util.TempDirectory() as filename:
                    model.export_coreml(filename)

        tc.config.set_num_gpus(old_num_gpus)
