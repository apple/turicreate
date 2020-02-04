# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from .util import TempDirectory

from copy import copy
import math
from os import mkdir
import unittest
import pytest

import coremltools
from coremltools.proto import FeatureTypes_pb2
import numpy as np
from scipy.io import wavfile
import sys as _sys

import turicreate as tc
from turicreate.toolkits._internal_utils import _raise_error_if_not_sarray
from turicreate.toolkits._main import ToolkitError
from turicreate.toolkits._internal_utils import _mac_ver


class ReadAudioTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        random = np.random.RandomState(1234)
        self.noise1 = random.normal(loc=500, scale=100, size=16000).astype("int16")
        self.sample_rate1 = 16000
        self.noise2 = random.normal(loc=500, scale=100, size=48000).astype("int16")
        self.sample_rate2 = 48000

    def test_simple_case(self, random_order=False):
        with TempDirectory() as temp_dir:
            file1, file2 = self._write_audio_files_in_dir(temp_dir)

            sf = tc.load_audio(temp_dir, recursive=False, random_order=random_order)

            self._assert_audio_sframe_correct(sf, file1, file2)

    def test_recursive_dir(self):
        with TempDirectory() as temp_dir:
            file1 = temp_dir + "/1.wav"
            mkdir(temp_dir + "/foo")
            file2 = temp_dir + "/foo/2.wav"
            wavfile.write(file1, self.sample_rate1, self.noise1)
            wavfile.write(file2, self.sample_rate2, self.noise2)

            sf = tc.load_audio(temp_dir)

            self._assert_audio_sframe_correct(sf, file1, file2)

    def test_no_path(self):
        with TempDirectory() as temp_dir:
            file1, file2 = self._write_audio_files_in_dir(temp_dir)

            sf = tc.load_audio(temp_dir, with_path=False)

            self.assertEqual(len(sf), 2)
            self.assertEqual(sorted(sf.column_names()), ["audio"])

    def test_ignore_failure(self):
        with TempDirectory() as temp_dir:
            file1, file2 = self._write_audio_files_in_dir(temp_dir)
            with open(temp_dir + "/junk.wav", "w") as f:
                f.write("junk, junk, junk. Not audio data!")

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
            self.assertEqual(sorted(sf.column_names()), ["audio", "path"])

            # Check the audio file
            audio1 = sf.filter_by([file1], "path")["audio"][0]
            self.assertEqual(audio1["sample_rate"], self.sample_rate1)
            self.assertTrue(all(audio1["data"] == self.noise1))

    def _assert_audio_sframe_correct(self, sf, file1, file2):
        self.assertEqual(len(sf), 2)
        self.assertEqual(sorted(sf.column_names()), ["audio", "path"])

        # Check the first audio file
        audio1 = sf.filter_by([file1], "path")["audio"][0]
        self.assertEqual(audio1["sample_rate"], self.sample_rate1)
        self.assertTrue(all(audio1["data"] == self.noise1))

        # Check the second audio file
        audio2 = sf.filter_by([file2], "path")["audio"][0]
        self.assertEqual(audio2["sample_rate"], self.sample_rate2)
        self.assertTrue(all(audio2["data"] == self.noise2))

    def _write_audio_files_in_dir(self, dir_path):
        file1 = dir_path + "/1.wav"
        file2 = dir_path + "/2.wav"
        wavfile.write(file1, self.sample_rate1, self.noise1)
        wavfile.write(file2, self.sample_rate2, self.noise2)
        return file1, file2


def _generate_binary_test_data():
    random = np.random.RandomState(1234)

    def generate_white_noise(length, sample_rate):
        loc = random.randint(300, 600)
        scale = random.randint(80, 130)
        size = int(length * sample_rate)
        data = random.normal(loc=loc, scale=scale, size=size).astype("int16")
        return {"sample_rate": sample_rate, "data": data}

    def generate_sine_wave(length, sample_rate):
        data = []
        volume = random.randint(500, 1500)
        freq = random.randint(300, 800)
        for x in range(int(length * sample_rate)):
            data.append(
                volume * math.sin(2 * math.pi * freq * (x / float(sample_rate)))
            )
        return {"sample_rate": sample_rate, "data": np.asarray(data, dtype="int16")}

    white_noise = [
        generate_white_noise(3, 16000),
        generate_white_noise(5.1, 48000),
        generate_white_noise(1, 16500),
    ]

    sine_waves = [
        generate_sine_wave(3, 16000),
        generate_sine_wave(5.1, 48000),
        generate_sine_wave(1, 12000),
    ]

    data = tc.SFrame(
        {
            "audio": white_noise + sine_waves,
            "labels": ["white noise"] * len(white_noise)
            + ["sine wave"] * len(sine_waves),
        }
    )
    return data


binary_test_data = _generate_binary_test_data()


class ClassifierTestTwoClassesStringLabels(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.data = copy(binary_test_data)
        self.is_binary_classification = True
        self.model = tc.sound_classifier.create(
            self.data, "labels", feature="audio", max_iterations=100
        )

    def test_create_invalid_max_iterations(self):
        with self.assertRaises(ToolkitError):
            model = tc.sound_classifier.create(
                self.data, "labels", feature="audio", max_iterations=0
            )

        with self.assertRaises(TypeError):
            model = tc.sound_classifier.create(
                self.data, "labels", feature="audio", max_iterations="1"
            )

    def test_create_with_invalid_custom_layers(self):
        with self.assertRaises(ToolkitError):
            model = tc.sound_classifier.create(
                self.data, "labels", feature="audio", custom_layer_sizes=[]
            )

        with self.assertRaises(ToolkitError):
            model = tc.sound_classifier.create(
                self.data, "labels", feature="audio", custom_layer_sizes={}
            )

        with self.assertRaises(ToolkitError):
            model = tc.sound_classifier.create(
                self.data, "labels", feature="audio", custom_layer_sizes=["1"]
            )

        with self.assertRaises(ToolkitError):
            model = tc.sound_classifier.create(
                self.data, "labels", feature="audio", custom_layer_sizes=[-1]
            )

        with self.assertRaises(ToolkitError):
            model = tc.sound_classifier.create(
                self.data, "labels", feature="audio", custom_layer_sizes=[0, 0]
            )

    def test_create_with_invalid_batch_size(self):
        with self.assertRaises(ValueError):
            model = tc.sound_classifier.create(
                self.data, "labels", feature="audio", batch_size=-1
            )

        with self.assertRaises(TypeError):
            model = tc.sound_classifier.create(
                self.data, "labels", feature="audio", batch_size=[]
            )

    def test_predict(self):
        # default ('class') output_type
        predictions = self.model.predict(self.data["audio"])
        _raise_error_if_not_sarray(predictions)
        self.assertEqual(len(predictions), len(self.data))
        for a, b in zip(predictions, self.data["labels"]):
            self.assertEqual(a, b)

        # 'probability' output_type
        if self.is_binary_classification:
            predictions = self.model.predict(
                self.data["audio"], output_type="probability"
            )
            _raise_error_if_not_sarray(predictions)
            self.assertEqual(len(predictions), len(self.data))
            for probabilities, correct_label in zip(predictions, self.data["labels"]):
                # correct value has highest probability?
                correct_index = self.model.classes.index(correct_label)
                self.assertEqual(np.argmax(probabilities), correct_index)
                # all probabilities sum close to 1?
                self.assertTrue(abs(np.sum(probabilities) - 1) < 0.00001)
        else:
            # 'probability' output type only supported for binary classification
            with self.assertRaises(ToolkitError):
                self.model.predict(self.data["audio"], output_type="probability")

        # 'probability_vector' output_type
        predictions = self.model.predict(
            self.data["audio"], output_type="probability_vector"
        )
        _raise_error_if_not_sarray(predictions)
        self.assertEqual(len(predictions), len(self.data))
        for prob_vector, correct_label in zip(predictions, self.data["labels"]):
            # correct value has highest probability?
            correct_index = self.model.classes.index(correct_label)
            self.assertEqual(np.argmax(prob_vector), correct_index)
            # all probabilities sum close to 1?
            self.assertTrue(abs(np.sum(prob_vector) - 1) < 0.00001)

        # predict with single (dict) example
        single_prediction = self.model.predict(self.data["audio"][0])
        _raise_error_if_not_sarray(single_prediction)
        self.assertEqual(len(single_prediction), 1)
        self.assertEqual(single_prediction[0], self.data["labels"][0])

        # predict with SFrame
        data = self.data.copy()
        del data["labels"]
        predictions = self.model.predict(data)
        _raise_error_if_not_sarray(predictions)
        self.assertEqual(len(predictions), len(data))
        for a, b in zip(predictions, self.data["labels"]):
            self.assertEqual(a, b)

    def test_save_and_load(self):
        with TempDirectory() as filename:
            self.model.save(filename)
            new_model = tc.load_model(filename)

        self.assertEqual(self.model.feature, new_model.feature)

        old_model_probs = self.model.predict(
            self.data["audio"], output_type="probability_vector"
        )
        new_model_probs = new_model.predict(
            self.data["audio"], output_type="probability_vector"
        )
        for a, b in zip(old_model_probs, new_model_probs):
            np.testing.assert_array_almost_equal(a, b, decimal=6)

    @unittest.skipIf(
        _mac_ver() < (10, 14), "Custom models only supported on macOS 10.14+"
    )
    def test_export_coreml_with_prediction(self):
        import resampy

        with TempDirectory() as temp_dir:
            file_name = temp_dir + "/model.mlmodel"
            self.model.export_coreml(file_name)
            core_ml_model = coremltools.models.MLModel(file_name)

        # Check predictions
        for cur_audio in self.data["audio"]:
            resampled_data = resampy.resample(
                cur_audio["data"], cur_audio["sample_rate"], 16000
            )
            first_audio_frame = resampled_data[:15600]

            tc_x = {"data": first_audio_frame, "sample_rate": 16000}
            tc_prob_vector = self.model.predict(tc_x, output_type="probability_vector")[
                0
            ]

            coreml_x = np.float32(
                first_audio_frame / 32768.0
            )  # Convert to [-1.0, +1.0]
            coreml_y = core_ml_model.predict({"audio": coreml_x})

            core_ml_prob_output_name = self.model.target + "Probability"
            for i, cur_class in enumerate(self.model.classes):
                self.assertAlmostEquals(
                    tc_prob_vector[i],
                    coreml_y[core_ml_prob_output_name][cur_class],
                    delta=0.001,
                )
        # Check metadata
        metadata = core_ml_model.get_spec().description.metadata
        self.assertTrue("sampleRate" in metadata.userDefined)
        self.assertEqual(metadata.userDefined["sampleRate"], "16000")

    def test_export_core_ml_no_prediction(self):
        import platform

        with TempDirectory() as temp_dir:
            file_name = temp_dir + "/model.mlmodel"
            self.model.export_coreml(file_name)
            core_ml_model = coremltools.models.MLModel(file_name)

        # Check metadata
        metadata = core_ml_model.get_spec().description.metadata
        self.assertTrue("sampleRate" in metadata.userDefined)
        self.assertEqual(metadata.userDefined["sampleRate"], "16000")
        self.assertDictEqual(
            {
                "com.github.apple.turicreate.version": tc.__version__,
                "com.github.apple.os.platform": platform.platform(),
                "type": "SoundClassifier",
                "coremltoolsVersion": coremltools.__version__,
                "sampleRate": "16000",
                "version": "1",
            },
            dict(core_ml_model.user_defined_metadata),
        )
        expected_result = "Sound classifier created by Turi Create (version %s)" % (
            tc.__version__
        )
        self.assertEquals(expected_result, core_ml_model.short_description)

    def test_evaluate(self):
        evaluation = self.model.evaluate(self.data)

        # Verify that all metrics are included in the result.
        for metric in [
            "accuracy",
            "auc",
            "precision",
            "recall",
            "f1_score",
            "log_loss",
            "confusion_matrix",
            "roc_curve",
        ]:
            self.assertIn(metric, evaluation)

    def test_classify(self):
        classification = self.model.classify(self.data)
        for a, b in zip(classification["class"], self.data["labels"]):
            self.assertEqual(a, b)
        for p in classification["probability"]:
            if self.is_binary_classification:
                self.assertTrue(p > 0.5)
            else:
                self.assertTrue(p > 0.33)

    def test_predict_topk(self):
        topk_predictions = self.model.predict_topk(self.data, k=2)
        self.assertEqual(len(topk_predictions), len(self.data) * 2)
        self.assertEqual(3, len(topk_predictions.column_names()))
        for column in ["id", "class", "probability"]:
            self.assertIn(column, topk_predictions.column_names())

        topk_predictions = self.model.predict_topk(self.data, k=1, output_type="rank")
        self.assertEqual(len(topk_predictions), len(self.data) * 1)
        self.assertEqual(3, len(topk_predictions.column_names()))
        for column in ["id", "class", "rank"]:
            self.assertIn(column, topk_predictions.column_names())
        unique_ranks = topk_predictions["rank"].unique()
        self.assertTrue(len(unique_ranks) == 1)
        self.assertTrue(unique_ranks[0] == 0)

    def test_predict_topk_invalid_k(self):
        with self.assertRaises(ToolkitError):
            pred = self.model.predict_topk(self.data, k=-1)

        with self.assertRaises(ToolkitError):
            pred = self.model.predict_topk(self.data, k=0)

        with self.assertRaises(TypeError):
            pred = self.model.predict_topk(self.data, k={})

    def test_validation_set(self):
        self.assertTrue(self.model.validation_accuracy is None)

    def test_summary(self):
        """
        Check the summary function.
        """
        model = self.model
        model.summary()

    def test_summary_str(self):
        model = self.model
        self.assertTrue(isinstance(model.summary("str"), str))

    def test_summary_dict(self):
        model = self.model
        self.assertTrue(isinstance(model.summary("dict"), dict))

    def test_summary_invalid_input(self):
        model = self.model
        with self.assertRaises(ToolkitError):
            model.summary(model.summary("invalid"))

        with self.assertRaises(ToolkitError):
            model.summary(model.summary(0))

        with self.assertRaises(ToolkitError):
            model.summary(model.summary({}))


class ClassifierTestTwoClassesIntLabels(ClassifierTestTwoClassesStringLabels):
    @classmethod
    def setUpClass(self):
        self.data = copy(binary_test_data)
        self.data["labels"] = self.data["labels"].apply(
            lambda x: 0 if x == "white noise" else 1
        )
        self.is_binary_classification = True
        layer_sizes = [100]
        self.model = tc.sound_classifier.create(
            self.data,
            "labels",
            feature="audio",
            custom_layer_sizes=layer_sizes,
            validation_set=None,
        )
        assert self.model.custom_layer_sizes == layer_sizes

    # Remove the following two tests after #2949 is fixed!

    @pytest.mark.xfail(
        reason="Non-deterministic test failure tracked in https://github.com/apple/turicreate/issues/2949"
    )
    def test_classify(self):
        pass

    @pytest.mark.xfail(
        reason="Non-deterministic test failure tracked in https://github.com/apple/turicreate/issues/2949"
    )
    def test_predict(self):
        pass


class ClassifierTestThreeClassesStringLabels(ClassifierTestTwoClassesStringLabels):
    @classmethod
    def setUpClass(self):
        def generate_constant_noise(length, sample_rate):
            data = np.ones((int(length * sample_rate))).astype("int16")
            return {"sample_rate": sample_rate, "data": data}

        constant_noise = [
            generate_constant_noise(2.5, 17000),
            generate_constant_noise(5, 17000),
            generate_constant_noise(1, 17000),
        ]
        constant_noise = tc.SFrame(
            {
                "audio": constant_noise,
                "labels": ["constant noise"] * len(constant_noise),
            }
        )
        self.data = copy(binary_test_data).append(constant_noise)

        self.is_binary_classification = False
        layer_sizes = [75, 100, 20]
        self.model = tc.sound_classifier.create(
            self.data,
            "labels",
            feature="audio",
            custom_layer_sizes=layer_sizes,
            validation_set=self.data,
            max_iterations=100,
        )
        assert self.model.custom_layer_sizes == layer_sizes

    def test_validation_set(self):
        self.assertTrue(self.model.validation_accuracy is not None)


class ClassifierTestWithShortClip(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.data = copy(binary_test_data)

        # Add a half second clip
        short_clip = binary_test_data[0]
        half_second_length = int(short_clip["audio"]["sample_rate"] / 2.0)
        short_clip["audio"]["data"] = short_clip["audio"]["data"][:half_second_length]
        short_clip = tc.SFrame(
            {"audio": [short_clip["audio"]], "labels": [short_clip["labels"]]}
        )
        self.data = self.data.append(short_clip)

    def test_get_deep_features(self):
        deep_features = tc.sound_classifier.get_deep_features(self.data["audio"])
        self.assertEqual(len(deep_features), len(self.data))
        self.assertEqual(deep_features[-1], [])

    def test_model(self):
        model = tc.sound_classifier.create(
            self.data, "labels", feature="audio", validation_set=self.data
        )

        # A prediction for a clip which is too short should be None
        predictions = model.predict(self.data)
        self.assertEqual(len(predictions), len(self.data))
        self.assertEqual(predictions[-1], None)
        for l in predictions[:-1]:
            self.assertNotEqual(l, None)

        predictions = model.predict(self.data, output_type="probability_vector")
        self.assertEqual(predictions[-1], None)
        for l in predictions[:-1]:
            self.assertNotEqual(l, None)

        evaluate_results = model.evaluate(self.data)
        self.assertIsNotNone(evaluate_results)

        classify_results = model.classify(self.data)
        self.assertEqual(classify_results[-1], {"class": None, "probability": None})
        for i in classify_results[:-1]:
            self.assertNotEqual(i["class"], None)
            self.assertNotEqual(i["probability"], None)

        topk_results = model.predict_topk(self.data)
        self.assertEqual(topk_results[-1]["class"], None)
        self.assertEqual(topk_results[-1]["probability"], None)
        for r in topk_results[:-1]:
            self.assertNotEqual(r["class"], None)
            self.assertNotEqual(r["probability"], None)


@unittest.skipIf(_mac_ver() < (10, 14), "Custom models only supported on macOS 10.14+")
class CoreMlCustomModelPreprocessingTest(unittest.TestCase):
    sample_rate = 16000
    frame_length = int(0.975 * sample_rate)

    def test_case(self):
        from turicreate.toolkits.sound_classifier import vggish_input

        model = coremltools.proto.Model_pb2.Model()
        model.customModel.className = "TCSoundClassifierPreprocessing"
        model.specificationVersion = 3

        # Input - float array with shape (frame_length)
        x = model.description.input.add()
        x.name = "x"
        x.type.multiArrayType.dataType = FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value(
            "FLOAT32"
        )
        x.type.multiArrayType.shape.append(self.frame_length)

        # Output - double array with shape (1, 96, 64)
        y = model.description.output.add()
        y.name = "y"
        y.type.multiArrayType.dataType = FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value(
            "DOUBLE"
        )
        y.type.multiArrayType.shape.append(1)
        y.type.multiArrayType.shape.append(96)
        y.type.multiArrayType.shape.append(64)

        with TempDirectory() as temp_dir:
            model = coremltools.models.MLModel(model)
            model_path = temp_dir + "/test.mlmodel"
            model.save(model_path)
            model = coremltools.models.MLModel(model_path)

        input_data = np.arange(self.frame_length) * 0.00001
        y1 = vggish_input.waveform_to_examples(input_data, self.sample_rate)[0]
        y2 = model.predict({"x": np.float32(input_data)})["y"]

        self.assertEqual(y2.shape, (1, 96, 64))
        self.assertTrue(np.isclose(y1, y2, atol=1e-04).all())


class ReuseDeepFeatures(unittest.TestCase):
    def test_simple_case(self):
        data = copy(binary_test_data)
        deep_features = tc.sound_classifier.get_deep_features(data["audio"])

        # Verify deep features in correct format
        self.assertTrue(isinstance(deep_features, tc.SArray))
        self.assertEqual(len(data), len(deep_features))
        self.assertEqual(deep_features.dtype, list)
        self.assertEqual(len(deep_features[0]), 3)
        self.assertTrue(isinstance(deep_features[0][0], np.ndarray))
        self.assertEqual(deep_features[0][0].dtype, np.float64)
        self.assertEqual(len(deep_features[0][0]), 12288)

        # Test helper methods
        self.assertTrue(tc.sound_classifier._is_audio_data_sarray(data["audio"]))
        self.assertTrue(tc.sound_classifier._is_deep_feature_sarray(deep_features))

        original_audio_data = data["audio"]
        del data["audio"]

        # Create a model using the deep features
        data["features"] = deep_features
        model = tc.sound_classifier.create(data, "labels", feature="features")

        # Test predict
        predictions_from_audio = model.predict(
            original_audio_data, output_type="probability_vector"
        )
        predictions_from_deep_features = model.predict(
            deep_features, output_type="probability_vector"
        )
        for a, b in zip(predictions_from_audio, predictions_from_deep_features):
            np.testing.assert_array_almost_equal(a, b, decimal=6)

        # Test classify
        predictions_from_audio = model.classify(original_audio_data)
        predictions_from_deep_features = model.classify(deep_features)
        for a, b in zip(predictions_from_audio, predictions_from_deep_features):
            self.assertEqual(a["class"], b["class"])
            np.testing.assert_array_almost_equal(
                a["probability"], b["probability"], decimal=6
            )

        # Test predict_topk
        predictions_from_audio = model.predict_topk(original_audio_data, k=2)
        predictions_from_deep_features = model.predict_topk(deep_features, k=2)
        for a, b in zip(predictions_from_audio, predictions_from_deep_features):
            self.assertEqual(a["id"], b["id"])
            self.assertEqual(a["class"], b["class"])
            np.testing.assert_array_almost_equal(
                a["probability"], b["probability"], decimal=6
            )

        # Test evaluate
        predictions_from_audio = model.evaluate(
            tc.SFrame({"features": original_audio_data, "labels": data["labels"]})
        )
        predictions_from_deep_features = model.evaluate(
            tc.SFrame({"features": deep_features, "labels": data["labels"]})
        )
        self.assertEqual(
            predictions_from_audio["f1_score"],
            predictions_from_deep_features["f1_score"],
        )
