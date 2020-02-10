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
import math
import numpy as np
from numbers import Number
from . import util as test_util
import pytest
from turicreate.toolkits._internal_utils import _mac_ver, _read_env_var_cpp
from turicreate.toolkits._main import ToolkitError as _ToolkitError
import uuid


def _load_data(
    self,
    num_examples=1000,
    num_features=3,
    max_num_sessions=4,
    randomize_num_sessions=True,
    num_labels=9,
    prediction_window=5,
    enforce_all_sessions=False,
):
    random.seed(42)

    self.num_examples = num_examples
    self.num_features = num_features
    self.num_sessions = (
        random.randint(1, max_num_sessions)
        if randomize_num_sessions
        else max_num_sessions
    )
    self.num_labels = num_labels
    self.prediction_window = prediction_window

    self.features = ["X1-r", "X2-r", "X3-r"]
    self.target = "activity_label"
    self.session_id = "session_id"

    if enforce_all_sessions:
        random_session_ids = _random_session_ids(self.num_examples, self.num_sessions)
    else:
        random_session_ids = sorted(
            [random.randint(0, self.num_sessions - 1) for i in range(self.num_examples)]
        )

    random_labels = [
        random.randint(0, self.num_labels - 1) for i in range(self.num_examples)
    ]

    self.data = tc.util.generate_random_sframe(
        column_codes="r" * self.num_features, num_rows=self.num_examples, random_seed=42
    )
    self.data[self.session_id] = random_session_ids
    self.data[self.target] = random_labels


"""
    Creates a random session_id column, that guarantees that the number
    of sessions is exactly the requested one.
"""


def _random_session_ids(num_examples, num_sessions):
    examples_per_session = num_examples // num_sessions
    if examples_per_session == 0:
        raise ValueError(
            "Can't divide {} lines into {} sessions.".format(num_examples, num_sessions)
        )

    min_lines_per_session = int(0.85 * examples_per_session)
    max_lines_per_session = int(1.15 * examples_per_session)

    lines_in_each_session = [
        random.randint(min_lines_per_session, max_lines_per_session)
        for i in range(num_sessions)
    ]
    lines_in_each_session = [
        (x * (num_examples)) // sum(lines_in_each_session)
        for x in lines_in_each_session
    ]
    lines_in_each_session[-1] += num_examples - sum(lines_in_each_session)

    session_ids = []
    for value, num_lines in enumerate(lines_in_each_session):
        session_ids.extend([value] * num_lines)

    return session_ids


class ActivityClassifierCreateStressTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        _load_data(self)

    def test_create_missing_value(self):
        sf_label = random.randint(0, self.num_labels - 1)
        sf_session_id = max(self.data[self.session_id])
        sf = self.data.append(
            tc.SFrame(
                {
                    self.features[0]: [None],
                    self.features[1]: [3.14],
                    self.features[2]: [5.23],
                    self.target: [sf_label],
                    self.session_id: [sf_session_id],
                }
            )
        )
        with self.assertRaises(_ToolkitError):
            tc.activity_classifier.create(
                sf,
                features=self.features,
                target=self.target,
                session_id=self.session_id,
                prediction_window=self.prediction_window,
                validation_set=None,
            )

    def test_create_missing_validation_set(self):
        sf_label = random.randint(0, self.num_labels - 1)
        sf_session_id = max(self.data[self.session_id])
        sf = self.data.append(
            tc.SFrame(
                {
                    self.features[0]: [None],
                    self.features[1]: [3.14],
                    self.features[2]: [5.23],
                    self.target: [sf_label],
                    self.session_id: [sf_session_id],
                }
            )
        )
        with self.assertRaises(_ToolkitError):
            tc.activity_classifier.create(
                self.data,
                features=self.features,
                target=self.target,
                session_id=self.session_id,
                prediction_window=self.prediction_window,
                validation_set=sf,
            )

    def test_create_invalid_batch_size(self):
        with self.assertRaises(_ToolkitError):
            tc.activity_classifier.create(
                self.data,
                features=self.features,
                target=self.target,
                session_id=self.session_id,
                prediction_window=self.prediction_window,
                validation_set=None,
                batch_size=-1,
            )

        with self.assertRaises(_ToolkitError):
            tc.activity_classifier.create(
                self.data,
                features=self.features,
                target=self.target,
                session_id=self.session_id,
                prediction_window=self.prediction_window,
                validation_set=None,
                batch_size="1",
            )

    def test_create_none_validation_set(self):
        model = tc.activity_classifier.create(
            self.data,
            features=self.features,
            target=self.target,
            session_id=self.session_id,
            prediction_window=self.prediction_window,
            validation_set=None,
        )
        predictions = model.predict(self.data)

    def test_create_no_validation_set(self):
        model = tc.activity_classifier.create(
            self.data,
            features=self.features,
            target=self.target,
            session_id=self.session_id,
            prediction_window=self.prediction_window,
        )
        predictions = model.predict(self.data)

    def test_create_with_verbose_False(self):
        args = [self.data, self.session_id, self.target]
        kwargs = {
            "features": self.features,
            "prediction_window": self.prediction_window,
        }
        test_util.assert_longer_verbose_logs(
            tc.activity_classifier.create, args, kwargs
        )

    def test_create_features_target_session(self):
        model = tc.activity_classifier.create(
            self.data,
            features=self.features,
            target=self.target,
            session_id=self.session_id,
        )
        predictions = model.predict(self.data)

    def test_create_target_session(self):
        model = tc.activity_classifier.create(
            self.data, target=self.target, session_id=self.session_id
        )
        predictions = model.predict(self.data)

    def test_invalid_model(self):
        """
        Verify that creating a model with wrong fields fails
        """
        with self.assertRaises(RuntimeError):
            model = tc.activity_classifier.create(
                self.data,
                features=self.features,
                target="wrong",
                session_id=self.session_id,
                prediction_window=self.prediction_window,
                validation_set=None,
            )

    def test_create_with_fixed_random_seed(self):
        model_1 = tc.activity_classifier.create(self.data, target=self.target, session_id=self.session_id, max_iterations=3, random_seed=86)
        pred_1 = model_1.predict(self.data, output_type="probability_vector")
        model_2 = tc.activity_classifier.create(self.data, target=self.target, session_id=self.session_id, max_iterations=3, random_seed=86)
        pred_2 = model_2.predict(self.data, output_type="probability_vector")
        assert len(pred_1) == len(pred_2)
        for i in range(len(pred_1)):
            self.assertEqual(pred_1[i], pred_2[i])


class ActivityClassifierAutoValdSetTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.fraction = 0.9
        self.seed = 42

    def _compute_expect_frac(self, num_sessions):
        if num_sessions > 200000:
            return 10000.0 / num_sessions
        elif num_sessions >= 200:
            return 0.95
        elif num_sessions >= 50:
            return 0.9
        return 1

    def _create_auto_validation_set(self, is_small=False):
        model = tc.activity_classifier.create(
            self.data,
            features=self.features,
            target=self.target,
            session_id=self.session_id,
            prediction_window=self.prediction_window,
            validation_set="auto",
        )
        predictions = model.predict(self.data)

        # Check the size of the auto validation set
        num_sessions = len(self.data[self.session_id].unique())
        valid_num_sessions = num_sessions - model.num_sessions
        valid_frac = float(valid_num_sessions / num_sessions)
        expected_frac = (
            0.0 if is_small else 1.0 - self._compute_expect_frac(num_sessions)
        )
        self.assertAlmostEqual(
            valid_frac,
            expected_frac,
            places=1,
            msg="Got {} validation sessions out of {}, which is {:.3f}, and not the expected {}".format(
                valid_num_sessions, num_sessions, valid_frac, expected_frac
            ),
        )

    def test_random_split_by_session(self):
        num_sessions = tc.activity_classifier.util._MIN_NUM_SESSIONS_FOR_SPLIT
        _load_data(
            self,
            num_examples=10000,
            max_num_sessions=num_sessions,
            randomize_num_sessions=False,
            enforce_all_sessions=True,
        )

        train, valid = tc.activity_classifier.util.random_split_by_session(
            self.data, self.session_id, self.fraction, self.seed
        )

        train_num_sessions = len(train[self.session_id].unique())
        train_frac = float(train_num_sessions / num_sessions)
        expected_frac = self.fraction

        self.assertAlmostEqual(
            train_frac,
            expected_frac,
            places=1,
            msg="Got {} train sessions out of {}, which is {:.3f}, and not the expected {}".format(
                train_num_sessions, num_sessions, train_frac, expected_frac
            ),
        )

        valid_num_sessions = len(valid[self.session_id].unique())
        valid_frac = float(valid_num_sessions / num_sessions)
        expected_valid_frac = 1.0 - self.fraction

        self.assertAlmostEqual(
            valid_frac,
            expected_valid_frac,
            places=1,
            msg="Got {} train sessions out of {}, which is {:.3f}, and not the expected {}".format(
                valid_num_sessions, num_sessions, valid_frac, expected_valid_frac
            ),
        )

        train_sessions_set = set(train[self.session_id].unique())
        valid_sessions_set = set(valid[self.session_id].unique())

        self.assertTrue(
            train_sessions_set.isdisjoint(valid_sessions_set),
            "After train-test split, the train and validation sets should not include the same sessions",
        )

    def test_create_auto_validation_set_small(self):
        min_num_session_for_split = 50
        num_sessions = min_num_session_for_split // 2
        _load_data(
            self,
            max_num_sessions=num_sessions,
            randomize_num_sessions=False,
            enforce_all_sessions=True,
        )

        self._create_auto_validation_set(is_small=True)

    def test_create_auto_validation_set_typical(self):
        num_sessions = tc.activity_classifier.util._MIN_NUM_SESSIONS_FOR_SPLIT * 4
        _load_data(
            self,
            num_examples=10000,
            max_num_sessions=num_sessions,
            randomize_num_sessions=False,
            enforce_all_sessions=True,
        )

        self._create_auto_validation_set()

    def test_create_auto_validation_set_string_session_id(self):
        num_sessions = tc.activity_classifier.util._MIN_NUM_SESSIONS_FOR_SPLIT * 4
        _load_data(
            self,
            num_examples=10000,
            max_num_sessions=num_sessions,
            randomize_num_sessions=False,
            enforce_all_sessions=True,
        )

        from six.moves import xrange as _xrange

        session_ids_dict = {}
        for i in _xrange(num_sessions):
            session_ids_dict[i] = uuid.uuid4().hex[:6].upper()

        self.data[self.session_id] = self.data[self.session_id].apply(
            lambda x: session_ids_dict[x]
        )

        self._create_auto_validation_set()


class ActivityClassifierTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        """
        The setup class method for the basic test case with all default values.
        """
        _load_data(self)

        # Create the model
        self.model = tc.activity_classifier.create(
            self.data,
            features=self.features,
            target=self.target,
            session_id=self.session_id,
            prediction_window=self.prediction_window,
            validation_set=None,
        )

        self.def_opts = {
            "verbose": True,
            "prediction_window": 100,
            "max_iterations": 10,
            "batch_size": 32,
        }

        # Answers
        self.opts = self.def_opts.copy()
        self.opts["prediction_window"] = self.prediction_window

        self.get_ans = {
            "features": lambda x: x == self.features,
            "training_time": lambda x: x > 0,
            "target": lambda x: x == self.target,
            "verbose": lambda x: x == True,
            "session_id": lambda x: x == self.session_id,
            "prediction_window": lambda x: x == self.prediction_window,
            "training_accuracy": lambda x: x >= 0 and x <= 1,
            "training_log_loss": lambda x: isinstance(x, Number),
            "max_iterations": lambda x: x == self.def_opts["max_iterations"],
            "num_sessions": lambda x: x == self.num_sessions,
            "num_features": lambda x: x == self.num_features,
            "num_examples": lambda x: x == self.num_examples,
            "num_classes": lambda x: x == self.num_labels,
            "batch_size": lambda x: x == self.def_opts["batch_size"],
            "classes": lambda x: sorted(x) == sorted(self.data[self.target].unique()),
        }
        self.exposed_fields_ans = list(self.get_ans.keys())
        self.fields_ans = self.exposed_fields_ans + [
            "training_report_by_class",
            "training_iterations",
            "random_seed",
            "training_precision",
            "training_confusion_matrix",
            "use_data_augmentation",
            "training_f1_score",
            "training_auc",
            "training_roc_curve",
            "training_recall",
        ]

    def _calc_expected_predictions_length(self, predict_input, top_k=1):

        input_sessions = predict_input.groupby(
            self.session_id, {"session_len": tc.aggregate.COUNT()}
        )
        prediction_window = self.model.prediction_window
        input_sessions["num_predictions_per_session"] = input_sessions[
            "session_len"
        ].apply(lambda x: math.ceil(float(x) / prediction_window))
        total_num_of_prediction = (
            sum(input_sessions["num_predictions_per_session"]) * top_k
        )

        return total_num_of_prediction

    def test_predict(self):
        """
        Check the predict() function.
        """
        model = self.model
        for output_type in ["probability_vector", "class"]:
            preds = model.predict(
                self.data, output_type=output_type, output_frequency="per_window"
            )
            expected_len = self._calc_expected_predictions_length(self.data)
            self.assertEqual(len(preds), expected_len)

    def test_export_coreml(self):
        """
        Check the export_coreml() function.
        """
        import coremltools

        # Save the model as a CoreML model file
        filename = tempfile.mkstemp("ActivityClassifier.mlmodel")[1]
        self.model.export_coreml(filename)

        # Load the model back from the CoreML model file
        coreml_model = coremltools.models.MLModel(filename)
        import platform

        self.assertDictEqual(
            {
                "com.github.apple.turicreate.version": tc.__version__,
                "com.github.apple.os.platform": platform.platform(),
                "target": self.target,
                "type": "activity_classifier",
                "prediction_window": str(self.prediction_window),
                "session_id": self.session_id,
                "features": ",".join(self.features),
                "max_iterations": "10",
                "version": "2",
            },
            dict(coreml_model.user_defined_metadata),
        )
        expected_result = "Activity classifier created by Turi Create (version %s)" % (
            tc.__version__
        )
        self.assertEquals(expected_result, coreml_model.short_description)

        self.check_prediction_match(self.model, coreml_model)

    def test_create_single_input_column(self):
        sf_label = random.randint(0, self.num_labels - 1)
        sf_session_id = max(self.data[self.session_id])
        input_data = tc.SFrame(
                {
                    self.features[0]: [3.14],
                    self.target: [sf_label],
                    self.session_id: [sf_session_id],
                }
        )
        model = tc.activity_classifier.create(
                input_data,
                features=["X1-r"],
                target=self.target,
                session_id=self.session_id,
                prediction_window=self.prediction_window,
        )
        filename = tempfile.mkstemp("ActivityClassifier.mlmodel")[1]
        model.export_coreml(filename)

        # Load the model back from the CoreML model file
        import coremltools
        coreml_model = coremltools.models.MLModel(filename)
        self.check_prediction_match(model, coreml_model)

    def check_prediction_match(self, model, coreml_model):
        # Create a small dataset, and compare the models' predict() output
        rs = np.random.RandomState(1234)
        dataset = tc.util.generate_random_sframe(column_codes="r" * 3, num_rows=10)
        dataset["session_id"] = 0
        dataset[self.target] = random_labels = [
            rs.randint(0, self.num_labels - 1,) for i in range(10)
        ]

        if _mac_ver() >= (10, 13):
            w = self.prediction_window
            labels = list(map(str, sorted(model.classes)))

            input_features = {}
            for f in self.features:
                input_features[f] = dataset[f].to_numpy()
            first_input_dict = {}
            second_input_dict = {}
            for key, value in input_features.items():
                first_input_dict[key] = value[:w].copy()
                second_input_dict[key] = value[w : 2 * w].copy()
            first_input_dict["stateIn"] = np.zeros((400))
            ret0 = coreml_model.predict(first_input_dict)

            second_input_dict["stateIn"] = ret0["stateOut"]
            ret1 = coreml_model.predict(second_input_dict)

            pred = model.predict(dataset, output_type="probability_vector")
            model_time0_values = pred[0]
            model_time1_values = pred[w]
            model_predictions = np.array([model_time0_values, model_time1_values])
            coreml_time0_values = [ret0[self.target + "Probability"][l] for l in labels]
            coreml_time1_values = [ret1[self.target + "Probability"][l] for l in labels]
            coreml_predictions = np.array([coreml_time0_values, coreml_time1_values])

            np.testing.assert_array_almost_equal(
                model_predictions, coreml_predictions, decimal=3
            )

    def test_classify(self):
        """
        Check the classify() function.
        """
        model = self.model
        preds = model.classify(self.data, output_frequency="per_window")
        expected_len = self._calc_expected_predictions_length(self.data)
        self.assertEqual(len(preds), expected_len)

    def test_classify_with_incomplete_data(self):
        data = self.data.copy()
        del data[self.features[0]]
        with self.assertRaises(_ToolkitError):
            pred = self.model.classify(data)

    def test_predict_topk(self):
        """
        Check the predict_topk function.
        """
        model = self.model
        for output_type in ["rank", "probability"]:
            preds = model.predict_topk(
                self.data, output_type=output_type, output_frequency="per_window"
            )
            expected_len = self._calc_expected_predictions_length(self.data, top_k=3)
            self.assertEqual(len(preds), expected_len)

            preds = model.predict_topk(
                self.data.head(100), k=5, output_frequency="per_window"
            )
            expected_len = self._calc_expected_predictions_length(
                self.data.head(100), top_k=5
            )
            self.assertEqual(len(preds), expected_len)

    def test_predict_topk_invalid_k(self):
        model = self.model
        with self.assertRaises(_ToolkitError):
            preds = model.predict_topk(self.data, k=-1)

        with self.assertRaises(_ToolkitError):
            preds = model.predict_topk(self.data, k=0)

        with self.assertRaises(TypeError):
            preds = model.predict_topk(self.data, k=[])

    def test_evaluate_with_incomplete_targets(self):
        """
        Check that evaluation does not require the test data to span all labels.
        """

        # Arbitrarily filter out all rows whose label matches the first row's.
        filtered_label = self.data[self.target][0]
        filtered_data = self.data[self.data[self.target] != filtered_label]

        # Run evaluation.
        evaluation = self.model.evaluate(filtered_data)

        # Verify that all metrics were computed and included in the result.
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
            self.assertTrue(
                self.get_ans[field](ans),
                """Get failed in field {}. Output was {}.""".format(field, ans),
            )

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
        with self.assertRaises(_ToolkitError):
            model.summary(model.summary("invalid"))

        with self.assertRaises(_ToolkitError):
            model.summary(model.summary(0))

        with self.assertRaises(_ToolkitError):
            model.summary(model.summary({}))

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
        test_methods_list = [
            func
            for func in dir(self)
            if callable(getattr(self, func)) and func.startswith("test")
        ]
        test_methods_list.remove("test_save_and_load")

        with test_util.TempDirectory() as filename:
            self.model.save(filename)
            self.model = None
            self.model = tc.load_model(filename)

            print("Repeating all test cases after model delete and reload")
            for test_method in test_methods_list:
                try:
                    getattr(self, test_method)()
                    print("Save and Load:", test_method, "has passed")
                except unittest.SkipTest:
                    pass
                except Exception as e:
                    self.assertTrue(
                        False,
                        "After model save and load, method "
                        + test_method
                        + " has failed with error: "
                        + str(e),
                    )


@unittest.skipIf(tc.util._num_available_gpus() == 0, "Requires GPU")
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
                model = tc.activity_classifier.create(
                    self.data, target=self.target, session_id=self.session_id
                )
                with test_util.TempDirectory() as filename:
                    model.save(filename)
                    model = tc.load_model(filename)

                filename = tempfile.mkstemp("ActivityClassifier.mlmodel")[1]
                model.export_coreml(filename)

        tc.config.set_num_gpus(old_num_gpus)
