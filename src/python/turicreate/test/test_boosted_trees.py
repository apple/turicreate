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
import math
import uuid
import random
import sys
import copy
from turicreate.toolkits._main import ToolkitError
import turicreate.toolkits.evaluation as evaluation
import shutil
import numpy as np
from array import array

import os as _os

dirname = _os.path.dirname(__file__)
mushroom_dataset = _os.path.join(dirname, "mushroom.csv")


_DEFAULT_OPTIONS_REGRESSION = {
    "step_size": 0.3,
    "max_depth": 6,
    "max_iterations": 10,
    "min_child_weight": 0.1,
    "min_loss_reduction": 0.0,
    "row_subsample": 1.0,
    "column_subsample": 1.0,
    "random_seed": None,
    "metric": "auto",
    "early_stopping_rounds": None,
    "model_checkpoint_interval": 5,
    "model_checkpoint_path": None,
    "resume_from_checkpoint": None,
}

_DEFAULT_OPTIONS_CLASSIFIER = copy.deepcopy(_DEFAULT_OPTIONS_REGRESSION)
_DEFAULT_OPTIONS_CLASSIFIER["class_weights"] = None


class BoostedTreesRegressionTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.data = tc.SFrame.read_csv(mushroom_dataset)
        self.data["label"] = (self.data["label"] == "p") + 20
        self.dtrain, self.dtest = self.data.random_split(0.8, seed=1)
        self.param = {
            "max_depth": 3,
            "step_size": 1,
            "min_loss_reduction": 1,
            "max_iterations": 10,
            "min_child_weight": 1,
        }
        self.target = "label"
        self.unpacked_features = self.data.column_names()
        self.unpacked_features.remove(self.target)
        self.features = self.unpacked_features[:]
        self.model = tc.boosted_trees_regression.create(
            self.dtrain, target=self.target, validation_set=self.dtest, **self.param
        )

        self.def_opts = copy.deepcopy(_DEFAULT_OPTIONS_REGRESSION)
        self.opts = self.def_opts.copy()
        self.opts.update(self.param)

        # Answers
        # ------------------------------------------------------------------------
        self.get_ans = {
            "column_subsample": lambda x: self.opts["column_subsample"],
            "unpacked_features": lambda x: x == self.unpacked_features,
            "features": lambda x: x == self.features,
            "max_depth": lambda x: x == self.opts["max_depth"],
            "min_child_weight": lambda x: x == self.opts["min_child_weight"],
            "min_loss_reduction": lambda x: x == self.opts["min_loss_reduction"],
            "num_examples": lambda x: x == self.dtrain.num_rows(),
            "num_unpacked_features": lambda x: x == 22,
            "num_features": lambda x: x == 22,
            "max_iterations": lambda x: x == self.opts["max_iterations"],
            "num_trees": lambda x: x == self.opts["max_iterations"],
            "num_validation_examples": lambda x: x == self.dtest.num_rows(),
            "row_subsample": lambda x: x == self.opts["row_subsample"],
            "step_size": lambda x: x == self.opts["step_size"],
            "target": lambda x: x == self.target,
            "training_rmse": lambda x: x > 0,
            "training_max_error": lambda x: x > 0,
            "training_time": lambda x: x >= 0,
            "trees_json": lambda x: isinstance(x, list),
            "validation_data": lambda x: isinstance(x, tc.SFrame)
            and len(x) == len(self.dtest),
            "validation_rmse": lambda x: x > 0,
            "validation_max_error": lambda x: x > 0,
            "random_seed": lambda x: x is None,
            "progress": lambda x: isinstance(x, tc.SFrame) or (x is None),
            "metric": lambda x: x == "auto",
            "early_stopping_rounds": lambda x: x is None,
            "model_checkpoint_interval": lambda x: x == 5,
            "model_checkpoint_path": lambda x: x is None,
            "resume_from_checkpoint": lambda x: x is None,
            "disable_posttrain_evaluation": lambda x: x == False,
        }

        self.metrics = ["rmse", "max_error"]
        self.fields_ans = self.get_ans.keys()

    def test_create(self):
        model = tc.boosted_trees_regression.create(
            self.dtrain, target="label", validation_set=self.dtest, **self.param
        )

        rmse = model.evaluate(self.dtest, "rmse")["rmse"]
        self.assertTrue(model is not None)
        self.assertTrue(rmse < 0.1)

        dtrain = self.dtrain
        dtrain["label"] = 10
        self.assertRaises(
            ToolkitError,
            lambda: tc.boosted_trees_regression.create(
                self.dtrain, target="label_wrong", **self.param
            ),
        )

    def test__list_fields(self):
        """
        Check the _list_fields function. Compare with the answer.
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
        for field in self.fields_ans:
            ans = model._get(field)
            self.assertTrue(
                self.get_ans[field](ans),
                """Get failed in field {}. Output was {}.""".format(field, ans),
            )

    def test_summary(self):
        """
        Check the summary function. Compare with the answer supplied as
        a lambda function for each field. Uses the same answers as test_get.
        """
        model = self.model
        print(model.summary())

    def test_repr(self):
        """
        Check the repr function.
        """
        model = self.model
        ans = str(model)
        self.assertTrue(type(ans) == str)

    def test_save_and_load(self):
        """
        Make sure saving and loading retains things.
        """
        filename = "save_file%s" % (str(uuid.uuid4()))
        self.model.save(filename)
        self.model = tc.load_model(filename)

        try:
            self.test_summary()
            print("Summary passed")
            self.test_repr()
            print("Repr passed")
            self.test_predict()
            print("Predict passed")
            self.test_evaluate()
            print("Evaluate passed")
            self.test_extract_features()
            print("Extract features passed")
            self.test_feature_importance()
            print("Feature importance passed")
            self.test__list_fields()
            print("List field passed")
            self.test_get()
            print("Get passed")
            shutil.rmtree(filename)
        except:
            self.assertTrue(False, "Failed during save & load diagnostics")

    def test_predict(self):
        y1 = self.model.predict(self.dtest)
        self.assertTrue(len(y1) == len(self.dtest))
        print(self.model.evaluate(self.dtest))
        print("check the result of evaluate and print history, they should match")

        y2 = self.model.predict(
            self.dtest[[c for c in self.dtest.column_names() if c != "label"]]
        )
        self.assertTrue(all((y1 - y2) * (y1 - y2) < 1e-10))

    def test_evaluate(self):
        """
        Make sure that evaluate works.
        """
        model = self.model
        t = self.dtrain[self.target]
        p = model.predict(self.dtrain)
        self.sm_metrics = {
            "max_error": evaluation.max_error(t, p),
            "rmse": evaluation.rmse(t, p),
        }

        def check_metric(ans, metric):
            self.assertTrue(ans is not None)
            self.assertTrue(metric in ans)
            self.assertAlmostEqual(
                ans[metric],
                self.sm_metrics[metric],
                places=4,
                msg="%s = (%s,%s)" % (metric, ans[metric], self.sm_metrics[metric]),
            )

        # Default
        ans = model.evaluate(self.dtrain)
        self.assertEqual(sorted(ans.keys()), sorted(self.metrics))
        for m in self.metrics:
            check_metric(ans, m)

        # Individual
        for m in self.metrics:
            ans = model.evaluate(self.dtrain, metric=m)
            check_metric(ans, m)

    def test_extract_features(self):
        y1 = self.model.extract_features(self.dtest)
        self.assertTrue(len(y1) == len(self.dtest))
        for feature in y1:
            self.assertTrue(len(feature) == self.model.max_iterations)

    def test_feature_importance(self):
        sf = self.model.get_feature_importance()
        self.assertEqual(sf.column_names(), ["name", "index", "count"])

    def test_trees_json(self):
        tree_0_vert_0 = eval(self.model.trees_json[0])["vertices"][0]
        self.assertEquals(
            set(tree_0_vert_0.keys()),
            set(
                [
                    "name",
                    "value_hexadecimal",
                    "yes_child",
                    "cover",
                    "missing_child",
                    "no_child",
                    "type",
                    "id",
                    "value",
                    "gain",
                ]
            ),
        )

    def test_list_and_dict_type(self):
        rmse_threshold = 0.2

        simple_data = self.data
        simple_train, simple_test = simple_data.random_split(0.8, seed=1)

        # make a more complicated dataset containing list and dictionary type columns
        complex_data = copy.copy(simple_data)
        complex_data["random_list_noise"] = tc.SArray(
            [
                [random.gauss(0, 1) for j in range(3)]
                for i in range(complex_data.num_rows())
            ]
        )
        complex_data["random_dict_noise"] = tc.SArray(
            [{"x0": random.gauss(0, 1)} for i in range(complex_data.num_rows())]
        )
        complex_train, complex_test = complex_data.random_split(0.8, seed=1)

        for (train, test) in [
            (simple_train, simple_test),
            (complex_train, complex_test),
        ]:
            self._test_regression_model(train, test, rmse_threshold)

    def _test_regression_model(self, train, test, rmse_threshold, target="label"):
        # create
        model = tc.boosted_trees_regression.create(
            train, target=target, validation_set=test, **self.param
        )
        # predict
        pred = model.predict(test)
        rmse = evaluation.rmse(pred, test[target])
        self.assertLess(rmse, rmse_threshold)

        # evaluate
        rmse_eval = model.evaluate(test, metric="rmse")["rmse"]
        self.assertTrue(rmse_eval < rmse_threshold)
        self.assertAlmostEqual(rmse_eval, rmse, delta=1e-2)

    def test_predict_new_category(self):
        # make new categorical feature
        new_test = self.dtest[:]
        # change 'r' cap-color  into a new color 'z'
        new_test["cap-color"] = new_test["cap-color"].apply(
            lambda x: "z" if x == "r" else x
        )
        y1 = self.model.predict(new_test)

        new_data = self.data[:]
        new_data["dict_color_feature"] = new_data["cap-color"].apply(
            lambda x: {"cap-color": ord(x)}
        )
        train, test = new_data.random_split(0.8, seed=1)

        ## add a new key to dictionary in predict time
        test["dict_color_feature"] = test["dict_color_feature"].apply(
            lambda x: dict(list(x.items()) + [("cap-color2", x["cap-color"] + 1)])
        )

        model = tc.boosted_trees_regression.create(train, target="label", **self.param)
        y = self.model.predict(test)


## ---------------------------------------------------------------------------
##
##             Boosted Trees Classifier Test
##
## ---------------------------------------------------------------------------


def test_suite_boosted_trees_classifier():
    """
    Create a test suite for each test case in the BoostedTreesClassifierTest.
    """
    testCases = [
        binary_classification_integer_target,
        binary_classification_string_target,
        binary_classification_string_target_misc_input,
        multiclass_classification_integer_target,
        multiclass_classification_string_target,
        multiclass_classification_string_target_misc_input,
    ]

    for t in testCases:
        testcase_members = {}
        testcase_members[t.__name__] = classmethod(t)
        testcase_class = type(
            "BoostedTreesClassifierTest_%s" % t.__name__,
            (BoostedTreesClassifierTest,),
            testcase_members,
        )
        testcase_class.__test__ = True
        getattr(testcase_class, t.__name__)()
        for method in dir(testcase_class):
            if method.startswith("test_"):
                testcase_instance = testcase_class(method)
                getattr(testcase_instance, method)()


def binary_classification_integer_target(cls):
    """
    Binary classification with an integer target.
    """
    # Get the data from the mushroom dataset.
    cls.data = tc.SFrame.read_csv(mushroom_dataset)
    cls.dtrain, cls.dtest = cls.data.random_split(0.8, seed=1)
    cls.dtrain["label"] = cls.dtrain["label"] == "p"
    cls.dtest["label"] = cls.dtest["label"] == "p"
    cls.param = {
        "max_depth": 3,
        "step_size": 1,
        "min_loss_reduction": 1,
        "max_iterations": 2,
        "min_child_weight": 1,
    }
    cls.target = "label"
    cls.unpacked_features = cls.data.column_names()
    cls.unpacked_features.remove(cls.target)
    cls.features = cls.unpacked_features[:]
    cls.model = tc.boosted_trees_classifier.create(
        cls.dtrain, target=cls.target, validation_set=cls.dtest, **cls.param
    )

    cls.def_opts = copy.deepcopy(_DEFAULT_OPTIONS_CLASSIFIER)
    cls.opts = cls.def_opts.copy()
    cls.opts.update(cls.param)
    cls.type = int

    # Answers
    # ------------------------------------------------------------------------
    if "classes" in cls.model._list_fields():
        num_examples_per_class = {
            c: (cls.dtrain[cls.target] == c).sum() for c in cls.model.classes
        }
    cls.get_ans = {
        "column_subsample": lambda x: cls.opts["column_subsample"],
        "unpacked_features": lambda x: x == cls.unpacked_features,
        "features": lambda x: x == cls.features,
        "max_depth": lambda x: x == cls.opts["max_depth"],
        "min_child_weight": lambda x: x == cls.opts["min_child_weight"],
        "min_loss_reduction": lambda x: x == cls.opts["min_loss_reduction"],
        "num_examples": lambda x: x == cls.dtrain.num_rows(),
        "num_examples_per_class": lambda x: x == num_examples_per_class,
        "num_classes": lambda x: x == 2,
        "classes": lambda x: x == [0, 1],
        "num_unpacked_features": lambda x: x == 22,
        "num_features": lambda x: x == 22,
        "max_iterations": lambda x: x == cls.opts["max_iterations"],
        "num_trees": lambda x: x == cls.opts["max_iterations"],
        "num_validation_examples": lambda x: x == cls.dtest.num_rows(),
        "row_subsample": lambda x: x == cls.opts["row_subsample"],
        "step_size": lambda x: x == cls.opts["step_size"],
        "target": lambda x: x == cls.target,
        "training_accuracy": lambda x: x > 0,
        "training_log_loss": lambda x: x > 0,
        "training_time": lambda x: x >= 0,
        "class_weights": lambda x: x == {0: 1.0, 1: 1.0},
        "trees_json": lambda x: isinstance(x, list),
        "validation_accuracy": lambda x: x > 0,
        "validation_log_loss": lambda x: x > 0,
        "random_seed": lambda x: x is None,
        "progress": lambda x: isinstance(x, tc.SFrame) or (x is None),
        "metric": lambda x: x == "auto",
        "early_stopping_rounds": lambda x: x is None,
        "model_checkpoint_interval": lambda x: x == 5,
        "model_checkpoint_path": lambda x: x is None,
        "resume_from_checkpoint": lambda x: x is None,
        "training_auc": lambda x: x > 0,
        "training_confusion_matrix": lambda x: len(x) > 0,
        "training_f1_score": lambda x: x > 0,
        "training_precision": lambda x: x > 0,
        "training_recall": lambda x: x > 0,
        "training_report_by_class": lambda x: len(x) > 0,
        "training_roc_curve": lambda x: len(x) > 0,
        "validation_data": lambda x: isinstance(x, tc.SFrame)
        and len(x) == len(cls.dtest),
        "validation_auc": lambda x: x > 0,
        "validation_confusion_matrix": lambda x: len(x) > 0,
        "validation_f1_score": lambda x: x > 0,
        "validation_precision": lambda x: x > 0,
        "validation_recall": lambda x: x > 0,
        "validation_report_by_class": lambda x: len(x) > 0,
        "validation_roc_curve": lambda x: len(x) > 0,
        "disable_posttrain_evaluation": lambda x: x == False,
    }
    cls.fields_ans = cls.get_ans.keys()


def binary_classification_string_target(cls):

    binary_classification_integer_target(cls)
    cls.type = str
    cls.dtrain["label"] = cls.dtrain["label"].astype(str)
    cls.dtest["label"] = cls.dtest["label"].astype(str)
    cls.dtrain["label"] = cls.dtrain["label"] + "-cat"
    cls.dtest["label"] = cls.dtest["label"] + "-cat"
    cls.model = tc.boosted_trees_classifier.create(
        cls.dtrain, target=cls.target, validation_set=cls.dtest, **cls.param
    )

    num_examples_per_class = {
        c: (cls.dtrain[cls.target] == c).sum() for c in cls.model.classes
    }
    cls.get_ans["num_examples_per_class"] = lambda x: x == num_examples_per_class
    cls.get_ans["class_weights"] = lambda x: x == {"0-cat": 1.0, "1-cat": 1.0}
    cls.get_ans["classes"] = lambda x: x == ["0-cat", "1-cat"]


def binary_classification_string_target_misc_input(cls):
    binary_classification_string_target(cls)

    # Add noise columns of categorical,
    noise_X = tc.util.generate_random_sframe(cls.data.num_rows(), "vmdA")

    for c in noise_X.column_names():
        cls.data[c] = noise_X[c]


def multiclass_classification_integer_target(cls):

    binary_classification_integer_target(cls)

    def create_multiclass_label(row):
        if row["label"] == 0:
            return 0
        elif row["cap-surface"] == "y":
            return 1
        else:
            return 2

    cls.dtrain["label"] = cls.dtrain.apply(create_multiclass_label)
    cls.dtest["label"] = cls.dtest.apply(create_multiclass_label)
    cls.model = tc.boosted_trees_classifier.create(
        cls.dtrain, target=cls.target, validation_set=cls.dtest, **cls.param
    )

    num_examples_per_class = {
        c: (cls.dtrain[cls.target] == c).sum() for c in cls.model.classes
    }
    cls.get_ans["num_examples_per_class"] = lambda x: x == num_examples_per_class
    cls.get_ans["num_classes"] = lambda x: x == 3
    cls.get_ans["num_trees"] = lambda x: x == 6
    cls.get_ans["classes"] = lambda x: set(x) == set([0, 1, 2])
    cls.get_ans["class_weights"] = lambda x: x == {0: 1.0, 1: 1.0, 2: 1.0}


def multiclass_classification_string_target(cls):

    multiclass_classification_integer_target(cls)
    cls.type = str
    cls.dtrain["label"] = cls.dtrain["label"].astype(str)
    cls.dtest["label"] = cls.dtest["label"].astype(str)
    cls.model = tc.boosted_trees_classifier.create(
        cls.dtrain, target=cls.target, validation_set=cls.dtest, **cls.param
    )
    num_examples_per_class = {
        c: (cls.dtrain[cls.target] == c).sum() for c in cls.model.classes
    }
    cls.get_ans["num_examples_per_class"] = lambda x: x == num_examples_per_class
    cls.get_ans["classes"] = lambda x: set(x) == set(map(str, [0, 1, 2]))
    cls.get_ans["class_weights"] = lambda x: x == {"0": 1.0, "1": 1.0, "2": 1.0}


def multiclass_classification_string_target_misc_input(cls):
    multiclass_classification_string_target(cls)

    # Add noise columns of categorical,
    noise_X = tc.util.generate_random_sframe(cls.data.num_rows(), "vmdA")

    for c in noise_X.column_names():
        cls.data[c] = noise_X[c]


class BoostedTreesClassifierTest(unittest.TestCase):
    __test__ = False

    def test_create(self):
        model = tc.boosted_trees_classifier.create(
            self.dtrain, target="label", validation_set=self.dtest, **self.param
        )

        self.assertTrue(model is not None)
        self.assertGreater(model.evaluate(self.dtest, "accuracy")["accuracy"], 0.9)

        dtrain = self.dtrain[:]
        dtrain["label"] = 10
        self.assertRaises(
            ToolkitError,
            lambda: tc.boosted_trees_classifier.create(
                self.dtrain, target="label_wrong", **self.param
            ),
        )

    def test__list_fields(self):
        """
        Check the _list_fields function. Compare with the answer.
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
        for field in self.fields_ans:
            ans = model._get(field)
            result = self.get_ans[field](ans)
            if isinstance(result, tc.SArray):
                result = result.all()
            self.assertTrue(
                result, """Get failed in field {}. Output was {}.""".format(field, ans)
            )

    def test_summary(self):
        """
        Check the summary function. Compare with the answer supplied as
        a lambda function for each field. Uses the same answers as test_get.
        """
        model = self.model
        print(model.summary())

    def test_repr(self):
        """
        Check the repr function.
        """
        model = self.model
        ans = str(model)
        self.assertTrue(type(ans) == str, "Repr failed")

    def test_save_and_load(self):
        """
        Make sure saving and loading retains things.
        """
        filename = "save_file%s" % (str(uuid.uuid4()))
        self.model.save(filename)
        self.model = tc.load_model(filename)

        try:
            self.test_summary()
            print("Summary passed")
            self.test_repr()
            print("Repr passed")
            self.test_predict()
            print("Predict passed")
            self.test_evaluate()
            print("Evaluate passed")
            self.test_extract_features()
            print("Extract features passed")
            self.test_feature_importance()
            print("Feature importance passed")
            self.test__list_fields()
            print("List field passed")
            self.test_get()
            print("Get passed")
            shutil.rmtree(filename)
        except:
            self.assertTrue(False, "Failed during save & load diagnostics")

    def test_predict_topk(self):

        ks = [self.model.num_classes - 1, self.model.num_classes]

        for k in ks:

            y1 = self.model.predict_topk(self.dtest, k=k, output_type="rank")
            self.assertEqual(y1["class"].dtype, self.type)
            self.assertEqual(y1["id"].dtype, int)
            self.assertEqual(y1.num_rows(), self.dtest.num_rows() * k)

            y2 = self.model.predict_topk(self.dtest, k=k, output_type="probability")
            self.assertEqual(y2["id"].dtype, int)
            self.assertEqual(y2.num_rows(), self.dtest.num_rows() * k)

            y3 = self.model.predict_topk(self.dtest, k=k, output_type="margin")
            self.assertEqual(y3["id"].dtype, int)
            self.assertEqual(y3.num_rows(), self.dtest.num_rows() * k)
            self.assertTrue(all(y3[y3["class"] == 0]["margin"] == 0.0))

            test_sf = tc.SFrame()
            test_sf["rank"] = y1["class"]
            test_sf["prob"] = y2["class"]
            test_sf["margin"] = y3["class"]
            test_sf["error"] = test_sf.apply(
                lambda x: x["rank"] != x["prob"] or x["rank"] != x["margin"]
            )

            self.assertEqual(test_sf["error"].sum(), 0)

    def test_predict(self):

        # Default, output_type = class
        y1 = self.model.predict(self.dtest)
        self.assertEqual(len(y1), self.dtest.num_rows())
        self.assertEqual(y1.dtype, self.type)

        # Default, output_type = class
        y1 = self.model.predict(self.dtest, output_type="class")
        self.assertEqual(len(y1), self.dtest.num_rows())
        self.assertEqual(y1.dtype, self.type)

        # output_type = probability vector
        y1 = self.model.predict(self.dtest, output_type="probability_vector")
        self.assertEqual(len(y1), self.dtest.num_rows())
        self.assertEqual(y1.dtype, array)
        self.assertTrue(all(y1.apply(lambda x: abs(sum(x) - 1.0)) < 1e-5))

        k = self.model.num_classes
        if k == 2:
            class_one = sorted(self.model.classes)[1]
            y_class = self.model.predict(self.dtest, "class") == class_one

            y1 = self.model.predict(self.dtest, "margin")
            self.assertEqual(len(y1), self.dtest.num_rows())
            self.assertTrue(all(y_class == (y1 > 0.0)))

            y1 = self.model.predict(self.dtest, "probability")
            self.assertEqual(len(y1), self.dtest.num_rows())
            self.assertTrue(all(y_class == (y1 > 0.5)))

    def test_classify(self):
        y1 = self.model.classify(self.dtest)
        self.assertEqual(len(y1), len(self.dtest))
        self.assertEqual(y1["class"].dtype, self.type)
        self.assertEqual(set(y1.column_names()), set(["class", "probability"]))

    def test_evaluate(self):
        t = self.dtrain[self.target]
        c = self.model.predict(self.dtrain, "class")
        p = self.model.predict(self.dtrain, "probability_vector")
        ans_metrics = [
            "accuracy",
            "auc",
            "confusion_matrix",
            "f1_score",
            "log_loss",
            "precision",
            "recall",
            "roc_curve",
        ]

        self.sm_metrics = {
            "accuracy": evaluation.accuracy(t, c),
            "auc": evaluation.auc(t, p),
            "confusion_matrix": evaluation.confusion_matrix(t, c),
            "f1_score": evaluation.f1_score(t, c),
            "log_loss": evaluation.log_loss(t, p),
            "precision": evaluation.precision(t, c),
            "recall": evaluation.recall(t, c),
            "roc_curve": evaluation.roc_curve(t, p),
        }
        model = self.model

        def check_cf_matrix(ans):
            self.assertTrue(ans is not None)
            self.assertTrue("confusion_matrix" in ans)
            cf = ans["confusion_matrix"].sort(["target_label", "predicted_label"])
            ans_cf = self.sm_metrics["confusion_matrix"].sort(
                ["target_label", "predicted_label"]
            )
            self.assertEqual(list(cf["count"]), list(ans_cf["count"]))

        def check_roc_curve(ans):
            self.assertTrue(ans is not None)
            self.assertTrue("roc_curve" in ans)
            roc = ans["roc_curve"]
            self.assertEqual(type(roc), tc.SFrame)

        def check_metric(ans, metric):
            if metric == "confusion_matrix":
                check_cf_matrix(ans)
            elif metric == "roc_curve":
                check_roc_curve(ans)
            else:
                self.assertTrue(ans is not None)
                self.assertTrue(metric in ans)
                self.assertAlmostEqual(
                    ans[metric],
                    self.sm_metrics[metric],
                    places=4,
                    msg="%s = (%s,%s)" % (metric, ans[metric], self.sm_metrics[metric]),
                )

        # Default
        ans = model.evaluate(self.dtrain)
        self.assertEqual(sorted(ans.keys()), sorted(ans_metrics))
        for m in ans_metrics:
            check_metric(ans, m)

        # Individual
        for m in ans_metrics:
            ans = model.evaluate(self.dtrain, metric=m)
            check_metric(ans, m)

        # Test evaluate with new class
        test_data = self.dtrain.copy().head()
        test_data[self.target] = test_data[self.target].apply(lambda x: str(x) + "-new")
        for m in ans_metrics:
            ans = model.evaluate(test_data, metric=m)

    def test_extract_features(self):
        y1 = self.model.extract_features(self.dtest)
        self.assertTrue(len(y1) == len(self.dtest))
        for feature in y1:
            if self.model.num_classes == 2:
                self.assertTrue(len(feature) == self.model.max_iterations)
            else:
                self.assertTrue(
                    len(feature) == self.model.max_iterations * self.model.num_classes
                )

    def test_feature_importance(self):
        sf = self.model.get_feature_importance()
        self.assertEqual(sf.column_names(), ["name", "index", "count"])

    def test_list_and_dict_type(self):
        accuracy_threshold = 0.8

        simple_data = self.data
        simple_train, simple_test = simple_data.random_split(0.8, seed=1)

        # make a more complicated dataset containing list and dictionary type columns
        complex_data = copy.copy(simple_data)
        complex_data["random_list_noise"] = tc.SArray(
            [
                [random.gauss(0, 1) for j in range(3)]
                for i in range(complex_data.num_rows())
            ]
        )
        complex_data["random_dict_noise"] = tc.SArray(
            [{"x0": random.gauss(0, 1)} for i in range(complex_data.num_rows())]
        )
        complex_train, complex_test = complex_data.random_split(0.8, seed=1)

        for (train, test) in [
            (simple_train, simple_test),
            (complex_train, complex_test),
        ]:
            self._test_classifier_model(train, test, accuracy_threshold)

    def _test_classifier_model(self, train, test, accuracy_threshold, target="label"):
        # create
        model = tc.boosted_trees_classifier.create(
            train, target=target, validation_set=test, **self.param
        )
        # predict
        pred = model.predict(test, output_type="class")
        accuracy = model.evaluate(test, metric="accuracy")
        self.assertGreater(accuracy["accuracy"], accuracy_threshold)

    def test_predict_new_category(self):
        # make new categorical feature
        new_test = copy.copy(self.dtest)
        # change 'r' cap-color  into a new color 'z'
        new_test["cap-color"] = new_test["cap-color"].apply(
            lambda x: "z" if x == "r" else x
        )
        y1 = self.model.predict(new_test)

        new_data = copy.copy(self.data)
        new_data["dict_color_feature"] = new_data["cap-color"].apply(
            lambda x: {"cap-color": ord(x)}
        )
        train, test = new_data.random_split(0.8, seed=1)

        # add a new key to dictionary in predict time
        test["dict_color_feature"] = test["dict_color_feature"].apply(
            lambda x: dict(
                list(x.items()) + list({"cap-color2": x["cap-color"] + 1}.items())
            )
        )

        model = tc.boosted_trees_classifier.create(train, target="label", **self.param)
        y = self.model.predict(test)

    def test_metric_none(self):
        # Make sure that when passing in the reported_evaluation_metric as none, the model still works.
        #

        simple_data = self.data
        simple_train, simple_test = simple_data.random_split(0.8, seed=1)

        model = tc.boosted_trees_classifier.create(
            simple_train, target="label", disable_posttrain_evaluation=True
        )

        # These fields should not be present.
        self.assertTrue("training_confusion_matrix" not in model._list_fields())
        self.assertTrue("training_precision" not in model._list_fields())
        self.assertTrue("training_recall" not in model._list_fields())
        self.assertTrue("training_report_by_class" not in model._list_fields())

        # None of these should error out.
        model.evaluate(simple_test)
        model.predict(simple_test)


class TestStringTarget(unittest.TestCase):
    def test_cat(self):
        import numpy as np

        # Arrange
        np.random.seed(8)
        n, d = 1000, 100
        sf = tc.SFrame()
        for i in range(d):
            sf.add_column(tc.SArray(np.random.rand(n)), inplace=True)
            target = np.random.randint(2, size=n)
            sf["target"] = target

        sf["target"] = sf["target"].astype(str)
        sf["target"] = "cat-" + sf["target"]
        model = tc.boosted_trees_classifier.create(sf, "target")

        # Act
        evaluation = model.evaluate(sf)

        # Assert
        self.assertEqual(
            ["cat-0", "cat-1"],
            sorted(list(evaluation["confusion_matrix"]["target_label"].unique())),
        )
