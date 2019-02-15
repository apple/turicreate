# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from .util import TempDirectory

import math
from os import mkdir
from sys import platform
import unittest

import coremltools
from coremltools.proto import FeatureTypes_pb2
import numpy as np
from scipy.io import wavfile

import turicreate as tc
from turicreate.toolkits._internal_utils import _raise_error_if_not_sarray
from turicreate.toolkits._main import ToolkitError
from turicreate.toolkits.sound_classifier import mel_features


class ReadAudioTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        random = np.random.RandomState(1234)
        self.noise1 = random.normal(loc=500, scale=100, size=16000).astype('int16')
        self.sample_rate1 = 16000
        self.noise2 = random.normal(loc=500, scale=100, size=48000).astype('int16')
        self.sample_rate2 = 48000

    def test_simple_case(self, random_order=False):
        with TempDirectory() as temp_dir:
            file1, file2 = self._write_audio_files_in_dir(temp_dir)

            sf = tc.load_audio(temp_dir, recursive=False, random_order=random_order)

            self._assert_audio_sframe_correct(sf, file1, file2)

    def test_recursive_dir(self):
        with TempDirectory() as temp_dir:
            file1 = temp_dir + '/1.wav'
            mkdir(temp_dir + '/foo')
            file2 = temp_dir + '/foo/2.wav'
            wavfile.write(file1, self.sample_rate1, self.noise1)
            wavfile.write(file2, self.sample_rate2, self.noise2)

            sf = tc.load_audio(temp_dir)

            self._assert_audio_sframe_correct(sf, file1, file2)

    def test_no_path(self):
        with TempDirectory() as temp_dir:
            file1, file2 = self._write_audio_files_in_dir(temp_dir)

            sf = tc.load_audio(temp_dir, with_path=False)

            self.assertEqual(len(sf), 2)
            self.assertEqual(sorted(sf.column_names()), ['audio'])

    def test_ignore_failure(self):
        with TempDirectory() as temp_dir:
            file1, file2 = self._write_audio_files_in_dir(temp_dir)
            with open(temp_dir + '/junk.wav', 'wb') as f:
                f.write('junk, junk, junk. Not audio data!')

            with self.assertRaises(ToolkitError):
                tc.load_audio(temp_dir, ignore_failure=False)
            sf = tc.load_audio(temp_dir)

            self._assert_audio_sframe_correct(sf, file1, file2)

    def test_random_oder(self):
        self.test_simple_case(random_order=True)

    def test_single_file(self):
        with TempDirectory() as temp_dir:
            file1, _ = self._write_audio_files_in_dir(temp_dir)

            sf = tc.load_audio(file1)

            self.assertEqual(len(sf), 1)
            self.assertEqual(sorted(sf.column_names()), ['audio', 'path'])

            # Check the audio file
            audio1 = sf.filter_by([file1], 'path')['audio'][0]
            self.assertEqual(audio1['sample rate'], self.sample_rate1)
            self.assertTrue(all(audio1['data'] == self.noise1))

    def _assert_audio_sframe_correct(self, sf, file1, file2):
        self.assertEqual(len(sf), 2)
        self.assertEqual(sorted(sf.column_names()), ['audio', 'path'])

        # Check the first audio file
        audio1 = sf.filter_by([file1], 'path')['audio'][0]
        self.assertEqual(audio1['sample rate'], self.sample_rate1)
        self.assertTrue(all(audio1['data'] == self.noise1))

        # Check the second audio file
        audio2 = sf.filter_by([file2], 'path')['audio'][0]
        self.assertEqual(audio2['sample rate'], self.sample_rate2)
        self.assertTrue(all(audio2['data'] == self.noise2))

    def _write_audio_files_in_dir(self, dir_path):
        file1 = dir_path + '/1.wav'
        file2 = dir_path + '/2.wav'
        wavfile.write(file1, self.sample_rate1, self.noise1)
        wavfile.write(file2, self.sample_rate2, self.noise2)
        return file1, file2


def _generate_binary_test_data():
    random = np.random.RandomState(1234)

    def generate_white_noise(length, sample_rate):
        loc = random.randint(300, 600)
        scale = random.randint(80, 130)
        size = int(length * sample_rate)
        data = random.normal(loc=loc, scale=scale, size=size).astype('int16')
        return {'sample rate': sample_rate, 'data': data}

    def generate_sine_wave(length, sample_rate):
        data = []
        volume = random.randint(500, 1500)
        freq = random.randint(300, 800)
        for x in range(int(length * sample_rate)):
            data.append(volume * math.sin(2 * math.pi * freq * (x / float(sample_rate))))
        return {'sample rate': sample_rate, 'data': np.asarray(data, dtype='int16')}

    white_noise = [generate_white_noise(3, 16000), generate_white_noise(5.1, 48000),
                   generate_white_noise(1, 16500)]

    sine_waves = [generate_sine_wave(3, 16000), generate_sine_wave(5.1, 48000),
                  generate_sine_wave(1, 12000)]

    data = tc.SFrame({'audio': white_noise + sine_waves,
                      'labels': ['white noise'] * len(white_noise) + ['sine wave'] * len(sine_waves)})
    return data


class ClassifierTestTwoClassesStringLabels(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        self.data = _generate_binary_test_data()
        self.is_binary_classification = True
        self.model = tc.sound_classifier.create(self.data, 'labels', feature='audio')

    def test_predict(self):
        # default ('class') output_type
        predictions = self.model.predict(self.data['audio'])
        _raise_error_if_not_sarray(predictions)
        self.assertEqual(len(predictions), len(self.data))
        for a, b in zip(predictions, self.data['labels']):
            self.assertEqual(a, b)

        # 'probability' output_type
        if self.is_binary_classification:
            predictions = self.model.predict(self.data['audio'], output_type='probability')
            _raise_error_if_not_sarray(predictions)
            self.assertEqual(len(predictions), len(self.data))
            for probabilities, correct_label in zip(predictions, self.data['labels']):
                # correct value has highest probability?
                correct_index = self.model.classes.index(correct_label)
                self.assertEqual(np.argmax(probabilities), correct_index)
                # all probabilities sum close to 1?
                self.assertTrue(abs(np.sum(probabilities) - 1) < 0.00001)
        else:
            # 'probability' output type only supported for binary classification
            with self.assertRaises(ToolkitError):
                self.model.predict(self.data['audio'], output_type='probability')

        # 'probability_vector' output_type
        predictions = self.model.predict(self.data['audio'], output_type='probability_vector')
        _raise_error_if_not_sarray(predictions)
        self.assertEqual(len(predictions), len(self.data))
        for prob_vector, correct_label in zip(predictions, self.data['labels']):
            # correct value has highest probability?
            correct_index = self.model.classes.index(correct_label)
            self.assertEqual(np.argmax(prob_vector), correct_index)
            # all probabilities sum close to 1?
            self.assertTrue(abs(np.sum(prob_vector) - 1) < 0.00001)

        # predict with single (dict) example
        single_prediction = self.model.predict(self.data['audio'][0])
        _raise_error_if_not_sarray(single_prediction)
        self.assertEqual(len(single_prediction), 1)
        self.assertEqual(single_prediction[0], self.data['labels'][0])

        # predict with SFrame
        data = self.data.copy()
        del data['labels']
        predictions = self.model.predict(data)
        _raise_error_if_not_sarray(predictions)
        self.assertEqual(len(predictions), len(data))
        for a, b in zip(predictions, self.data['labels']):
            self.assertEqual(a, b)

    def test_save_and_load(self):
        with TempDirectory() as filename:
            self.model.save(filename)
            new_model = tc.load_model(filename)

        self.assertEqual(self.model.feature, new_model.feature)

        old_model_probs = self.model.predict(self.data['audio'], output_type='probability_vector')
        new_model_probs = new_model.predict(self.data['audio'], output_type='probability_vector')
        for a, b in zip(old_model_probs, new_model_probs):
            self.assertItemsEqual(a, b)

    @unittest.skipIf(platform != 'darwin', 'Can not test predictions.')
    def test_mac_export_coreml(self):
        with TempDirectory() as temp_dir:
            file_name = temp_dir + '/model.mlmodel'
            self.model.export_coreml(file_name)
            core_ml_model = coremltools.models.MLModel(file_name)

        for cur_audio in self.data['audio']:
            cur_sample_rate = cur_audio['sample rate']
            first_sec_audio = cur_audio['data'][:cur_sample_rate]
            x = [{'data': first_sec_audio, 'sample rate': cur_sample_rate}]
            preprocessed_audio, _ = self.model._feature_extractor.preprocess_data(x, [None])

            tc_prob_vector = self.model.predict(tc.SArray(x), output_type='probability_vector')[0]

            coreml_y = core_ml_model.predict({'preprocessed_audio': preprocessed_audio[0]})
            prob_column_name = self.model.target + 'Probability'
            core_ml_prob_vector = [coreml_y[prob_column_name][cur_label] for cur_label in self.model.classes]

            self.assertEqual(len(core_ml_prob_vector), len(tc_prob_vector))
            for a, b in zip(core_ml_prob_vector, tc_prob_vector):
                self.assertAlmostEquals(a, b, delta=0.001)

    @unittest.skipIf(platform == 'darwin', 'Already testing in more comprehensive way.')
    def test_linux_export_core_ml(self):
        # TODO: just test that calling `export_coreml(...)` produces a .mlmodel file
        pass

    def test_evaluate(self):
        evaluation = self.model.evaluate(self.data)

        # Verify that all metrics are included in the result.
        for metric in ['accuracy', 'auc', 'precision', 'recall', 'f1_score',
                       'log_loss', 'confusion_matrix', 'roc_curve']:
            self.assertIn(metric, evaluation)

    def test_classify(self):
        classification = self.model.classify(self.data)
        for a, b in zip(classification['class'], self.data['labels']):
            self.assertEqual(a, b)
        for p in classification['probability']:
            if self.is_binary_classification:
                self.assertTrue(p > .5)
            else:
                self.assertTrue(p > .33)

    def test_predict_topk(self):
        topk_predictions = self.model.predict_topk(self.data, k=2)
        self.assertEqual(len(topk_predictions), len(self.data) * 2)
        self.assertEqual(3, len(topk_predictions.column_names()))
        for column in ['id', 'class', 'probability']:
            self.assertIn(column, topk_predictions.column_names())

        topk_predictions = self.model.predict_topk(self.data, k=1, output_type='rank')
        self.assertEqual(len(topk_predictions), len(self.data) * 1)
        self.assertEqual(3, len(topk_predictions.column_names()))
        for column in ['id', 'class', 'rank']:
            self.assertIn(column, topk_predictions.column_names())


class ClassifierTestTwoClassesIntLabels(ClassifierTestTwoClassesStringLabels):
    @classmethod
    def setUpClass(self):
        self.data = _generate_binary_test_data()
        self.data['labels'] = self.data['labels'].apply(lambda x: 0 if x == 'white noise' else 1)
        self.is_binary_classification = True

        self.model = tc.sound_classifier.create(self.data, 'labels', feature='audio')


class ClassifierTestThreeClassesStringLabels(ClassifierTestTwoClassesStringLabels):
    @classmethod
    def setUpClass(self):
        def generate_constant_noise(length, sample_rate):
            data = np.ones((int(length * sample_rate))).astype('int16')
            return {'sample rate': sample_rate, 'data': data}

        constant_noise = [generate_constant_noise(2.5, 17000), generate_constant_noise(5, 17000),
                          generate_constant_noise(1, 17000)]
        constant_noise = tc.SFrame({'audio': constant_noise,
                                    'labels': ['constant noise'] * len(constant_noise)})
        self.data = _generate_binary_test_data().append(constant_noise)

        self.is_binary_classification = False
        self.model = tc.sound_classifier.create(self.data, 'labels', feature='audio')
