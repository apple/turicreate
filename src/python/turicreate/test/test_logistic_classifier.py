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
import scipy.stats as ss
import uuid
import numpy as np
import statsmodels.api as sm
from sklearn.metrics import *
import statsmodels.formula.api as smf
import array
from turicreate.toolkits._main import ToolkitError
from turicreate.toolkits.classifier.logistic_classifier import _DEFAULT_SOLVER_OPTIONS
import shutil
import os
import copy
import pandas as pd


#
# Various test cases for the _LogisticRegressionClassifierModelTest
#
def binary_classification_integer_target(cls):
    """
    The setup class method for a binary classification problem with the
    target being binary.
    """

    ## Simulate test data
    np.random.seed(8)
    n, d = 100, 10
    cls.sf = tc.SFrame()
    for i in range(d):
        cls.sf.add_column(tc.SArray(np.random.rand(n)), inplace=True)
    target = np.random.randint(2, size=n)
    target[0] = 0
    target[1] = 1

    ## Compute the correct answers with statsmodels
    sm_model = sm.GLM(
        target, sm.add_constant(cls.sf.to_dataframe()), family=sm.families.Binomial()
    ).fit()
    cls.loss = -sm_model.llf
    cls.coef = list(sm_model.params)
    cls.stderr = list(sm_model.bse)
    cls.yhat_margin = tc.SArray(
        list(np.log(sm_model.fittedvalues) - np.log(1 - sm_model.fittedvalues))
    )
    cls.yhat_prob = tc.SArray(list(sm_model.fittedvalues))
    cls.yhat_max_prob = tc.SArray(list(sm_model.fittedvalues)).apply(
        lambda x: max(x, 1.0 - x)
    )
    cls.yhat_class = tc.SArray(list((sm_model.fittedvalues >= 0.5).astype(int)))
    cls.type = int
    cls.test_stderr = True

    # Predict top_k
    cls.yhat_prob_vec = []
    cls.topk_yhat_prob = []
    cls.topk_yhat_rank = []
    cls.topk_yhat_margin = []
    for margin, prob, cat in zip(cls.yhat_margin, cls.yhat_prob, cls.yhat_class):
        cls.yhat_prob_vec += [1 - prob, prob]
        cls.topk_yhat_prob += [1 - prob, prob]
        cls.topk_yhat_margin += [0, margin]
        if prob <= 0.5:
            cls.topk_yhat_rank += [0, 1]
        else:
            cls.topk_yhat_rank += [1, 0]

    # Compute the answers you get from Stats Models.
    cls.sm_cf_matrix = table = np.histogram2d(target, cls.yhat_class, bins=2)[0]

    ## Create the model
    cls.sf["target"] = target
    cls.def_kwargs = copy.deepcopy(_DEFAULT_SOLVER_OPTIONS)
    cls.def_opts = dict(
        list(cls.def_kwargs.items())
        + list(
            {
                "solver": "auto",
                "feature_rescaling": True,
                "class_weights": None,
                "l1_penalty": 0,
                "l2_penalty": 1e-2,
            }.items()
        )
    )

    cls.solver = "auto"
    cls.features = ["X{}".format(i) for i in range(1, d + 1)]
    cls.unpacked_features = ["X{}".format(i) for i in range(1, d + 1)]
    cls.target = "target"

    cls.model = tc.logistic_classifier.create(
        cls.sf,
        target="target",
        features=None,
        l2_penalty=0.0,
        feature_rescaling=True,
        validation_set=None,
        solver=cls.solver,
    )

    # Metrics!
    cls.metrics = [
        "accuracy",
        "auc",
        "confusion_matrix",
        "f1_score",
        "log_loss",
        "precision",
        "recall",
        "roc_curve",
    ]
    cls.sm_metrics = {
        "accuracy": accuracy_score(target, list(cls.yhat_class)),
        "auc": roc_auc_score(target, list(cls.yhat_prob)),
        "confusion_matrix": cls.sm_cf_matrix.flatten(),
        "f1_score": f1_score(target, list(cls.yhat_class)),
        "log_loss": log_loss(target, list(cls.yhat_prob)),
        "precision": precision_score(target, list(cls.yhat_class)),
        "recall": recall_score(target, list(cls.yhat_class)),
        "roc_curve": tc.toolkits.evaluation.roc_curve(cls.sf["target"], cls.yhat_prob),
    }

    ## Answers
    cls.opts = cls.def_opts.copy()
    cls.opts["l2_penalty"] = 0.0
    cls.opts["solver"] = "newton"

    cls.get_ans = {
        "coefficients": lambda x: isinstance(x, tc.SFrame),
        "convergence_threshold": lambda x: x == cls.opts["convergence_threshold"],
        "unpacked_features": lambda x: x == cls.unpacked_features,
        "feature_rescaling": lambda x: x == cls.opts["feature_rescaling"],
        "features": lambda x: x == cls.features,
        "l1_penalty": lambda x: x == cls.opts["l1_penalty"],
        "l2_penalty": lambda x: x == cls.opts["l2_penalty"],
        "lbfgs_memory_level": lambda x: x == cls.opts["lbfgs_memory_level"],
        "max_iterations": lambda x: x == cls.opts["max_iterations"],
        "num_classes": lambda x: x == 2,
        "classes": lambda x: list(x) == [0, 1],
        "num_coefficients": lambda x: x == 11,
        "num_examples": lambda x: x == 100,
        "class_weights": lambda x: x == {0: 1, 1: 1},
        "num_examples_per_class": lambda x: {
            0: cls.sf.num_rows() - cls.sf["target"].sum(),
            1: cls.sf["target"].sum(),
        },
        "num_unpacked_features": lambda x: x == 10,
        "num_features": lambda x: x == 10,
        "progress": lambda x: isinstance(x, tc.SFrame) or (x is None),
        "solver": lambda x: x == cls.opts["solver"],
        "step_size": lambda x: lambda x: x == cls.opts["step_size"],
        "target": lambda x: x == cls.target,
        "training_accuracy": lambda x: x >= 0 and x <= 1,
        "training_iterations": lambda x: x > 0,
        "training_loss": lambda x: abs(x - cls.loss) < 1e-5,
        "training_solver_status": lambda x: x == "SUCCESS: Optimal solution found.",
        "training_time": lambda x: x >= 0,
        "simple_mode": lambda x: not x,
        "training_auc": lambda x: x > 0,
        "training_confusion_matrix": lambda x: len(x) > 0,
        "training_f1_score": lambda x: x > 0,
        "training_log_loss": lambda x: x > 0,
        "training_precision": lambda x: x > 0,
        "training_recall": lambda x: x > 0,
        "training_report_by_class": lambda x: len(x) > 0,
        "training_roc_curve": lambda x: len(x) > 0,
        "validation_data": lambda x: isinstance(x, tc.SFrame) and len(x) == 0,
        "disable_posttrain_evaluation": lambda x: x == False,
    }
    cls.fields_ans = cls.get_ans.keys()


def multiclass_integer_target(cls):
    """
    The setup class method for multi-class classification problem with the
    target being integer.
    """

    binary_classification_integer_target(cls)
    np.random.seed(8)
    n, d = 100, 10
    cls.sf = tc.SFrame()
    for i in range(d):
        cls.sf.add_column(tc.SArray(np.random.rand(n)), inplace=True)
    target = np.random.randint(3, size=n)
    target[0] = 0
    target[1] = 1
    target[2] = 2

    sm_model = sm.MNLogit(target, sm.add_constant(cls.sf.to_dataframe())).fit()
    coef = np.empty([0])
    for i in range(sm_model.params.ndim):
        coef = np.append(coef, sm_model.params[i].values)
    cls.coef = list(coef)
    stderr = np.empty([0])
    for i in range(sm_model.params.ndim):
        stderr = np.append(stderr, sm_model.bse[i].values)
    cls.stderr = list(stderr)

    # Predict
    raw_predictions = sm_model.predict()
    cls.yhat_class = raw_predictions.argmax(-1)
    cls.yhat_max_prob = raw_predictions.max(-1)
    cls.sm_accuracy = np.diag(sm_model.pred_table()).sum() / sm_model.nobs
    cls.sm_cf_matrix = sm_model.pred_table().flatten()

    cls.sm_metrics = {
        "accuracy": accuracy_score(target, list(cls.yhat_class)),
        "auc": tc.toolkits.evaluation.auc(
            tc.SArray(target), tc.SArray(raw_predictions)
        ),
        "confusion_matrix": cls.sm_cf_matrix.flatten(),
        "f1_score": f1_score(target, list(cls.yhat_class), average="macro"),
        "log_loss": log_loss(target, list(raw_predictions)),
        "precision": precision_score(target, list(cls.yhat_class), average="macro"),
        "recall": recall_score(target, list(cls.yhat_class), average="macro"),
        "roc_curve": tc.toolkits.evaluation.roc_curve(
            tc.SArray(target), tc.SArray(raw_predictions)
        ),
    }

    # Predict topk
    preds_sf = tc.SFrame(pd.DataFrame(raw_predictions))
    cls.topk_yhat_prob = (
        preds_sf.pack_columns(preds_sf.column_names(), dtype=dict)
        .add_row_number()
        .stack("X1", ["class", "prediction"])
        .sort(["id", "class"])["prediction"]
    )
    cls.yhat_prob_vec = (
        preds_sf.pack_columns(preds_sf.column_names(), dtype=dict)
        .add_row_number()
        .stack("X1", ["class", "prediction"])
        .sort(["id", "class"])["prediction"]
    )

    # Function to rank all items in a list
    rank = lambda x: list(len(x) - ss.rankdata(x))
    rank_sa = preds_sf.pack_columns(preds_sf.column_names())["X1"].apply(rank)
    topk_yhat_rank = tc.SFrame({"X1": rank_sa}).add_row_number()
    topk_yhat_rank["X1"] = topk_yhat_rank["X1"].apply(
        lambda x: {i: v for i, v in enumerate(x)}
    )
    topk_yhat_rank = topk_yhat_rank.stack("X1").sort(["id", "X2"])["X3"].astype(int)
    cls.topk_yhat_rank = topk_yhat_rank

    # Compute the margins
    df = sm.add_constant(cls.sf.to_dataframe())
    sf_margin = sm.add_constant(np.dot(df.values, sm_model.params))
    sf_margin[:, 0] = 0
    sf_margin = tc.SFrame(pd.DataFrame(sf_margin))
    cls.topk_yhat_margin = (
        sf_margin.pack_columns(sf_margin.column_names(), dtype=dict)
        .add_row_number()
        .stack("X1", ["class", "prediction"])
        .sort(["id", "class"])["prediction"]
    )

    ## Create the model
    cls.sf["target"] = target
    cls.loss = -sm_model.llf

    cls.model = tc.logistic_classifier.create(
        cls.sf,
        target="target",
        features=None,
        l2_penalty=0.0,
        feature_rescaling=True,
        validation_set=None,
        solver=cls.solver,
    )

    cls.get_ans["num_classes"] = lambda x: x == 3
    cls.get_ans["classes"] = lambda x: x == [0, 1, 2]
    cls.get_ans["num_coefficients"] = lambda x: x == 22
    cls.get_ans["class_weights"] = lambda x: x == {0: 1, 1: 1, 2: 1}
    cls.get_ans["num_examples_per_class"] = lambda x: {
        0: (cls.sf["target"] == 0).sum(),
        1: (cls.sf["target"] == 1).sum(),
        2: (cls.sf["target"] == 2).sum(),
    }

    cls.fields_ans = cls.get_ans.keys()


def binary_classification_string_target(cls):
    """
    The setup class method for a binary classification problem with the
    target being binary.
    """

    binary_classification_integer_target(cls)

    cls.sf["target"] = cls.sf["target"].astype(str)
    cls.model = tc.logistic_classifier.create(
        cls.sf,
        target="target",
        features=None,
        l2_penalty=0.0,
        feature_rescaling=True,
        validation_set=None,
        solver=cls.solver,
    )

    cls.get_ans["classes"] = lambda x: x == ["0", "1"]
    cls.get_ans["class_weights"] = lambda x: x == {"0": 1, "1": 1}
    cls.get_ans["num_examples_per_class"] = lambda x: x == {
        "0": (cls.sf["target"] == "0").sum(),
        "1": (cls.sf["target"] == "1").sum(),
    }
    cls.type = str


def multiclass_string_target(cls):
    """
    The setup class method for a binary classification problem with the
    target being binary.
    """

    multiclass_integer_target(cls)

    cls.sf["target"] = cls.sf["target"].astype(str)
    cls.model = tc.logistic_classifier.create(
        cls.sf,
        target="target",
        features=None,
        l2_penalty=0.0,
        feature_rescaling=True,
        validation_set=None,
        solver=cls.solver,
    )

    cls.get_ans["classes"] = lambda x: x == ["0", "1", "2"]
    cls.get_ans["class_weights"] = lambda x: x == {"0": 1, "1": 1, "2": 1}
    cls.get_ans["num_examples_per_class"] = lambda x: x == {
        "0": (cls.sf["target"] == "0").sum(),
        "1": (cls.sf["target"] == "1").sum(),
        "2": (cls.sf["target"] == "2").sum(),
    }
    cls.type = str


def test_suite():
    """
    Create a test suite for each test case in LogisticRegressionClassifierModelTest
    """
    testCases = [
        binary_classification_integer_target,
        binary_classification_string_target,
        multiclass_integer_target,
    ]
    # multiclass_string_target]

    for t in testCases:
        testcase_members = {}
        testcase_members[t.__name__] = classmethod(t)
        testcase_class = type(
            "LogisticRegressionClassifierModelTest_%s" % t.__name__,
            (LogisticRegressionClassifierModelTest,),
            testcase_members,
        )
        getattr(testcase_class, t.__name__)()
        testcase_class.__test__ = True
        for method in dir(testcase_class):
            if method.startswith("test_"):
                testcase_instance = testcase_class(method)
                method_instance = getattr(testcase_instance, method)
                # needs callable() since some class- or instance-level
                # properties are named test_ and have non-method values.
                if callable(method_instance):
                    method_instance()


class LogisticRegressionClassifierModelTest(unittest.TestCase):
    __test__ = False
    """
    Unit test class for a LogisticRegressionModel that has already been created.
    """

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
        for field in self.fields_ans:
            ans = model._get(field)
            self.assertTrue(self.get_ans[field](ans))

    def test_coefficients(self):
        """
        Check that the coefficient values are very close to the correct values.
        """
        model = self.model
        coefs = model.coefficients
        coef_list = list(coefs["value"])
        self.assertTrue(np.allclose(coef_list, self.coef, rtol=1e-03, atol=1e-03))
        if self.test_stderr:
            stderr_list = list(coefs["stderr"])
            self.assertTrue(
                np.allclose(stderr_list, self.stderr, rtol=1e-03, atol=1e-03)
            )
        else:
            self.assertTrue("stderr" in coefs.column_names())
            self.assertEqual(list(coefs["stderr"]), [None for v in coef_list])

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
        model = self.model
        ans = str(model)
        self.assertEqual(type(ans), str)

    def test_predict_topk(self):
        """
        Check the prediction function against pre-computed answers. Check that
        all predictions are at most 1e-5 away from the true answers.
        """
        model = self.model
        tol = 1e-3
        k = model.num_classes

        ans = model.predict_topk(self.sf, output_type="margin", k=k)
        ans = ans.sort(["id", "class"])["margin"]
        self.assertTrue(
            np.allclose(ans, self.topk_yhat_margin, tol, tol),
            "{%s} - {%s}" % (ans, self.topk_yhat_margin),
        )

        ans = model.predict_topk(self.sf, output_type="probability", k=k)
        ans = ans.sort(["id", "class"])["probability"]
        self.assertTrue(
            np.allclose(ans, self.topk_yhat_prob, tol, tol),
            "{%s} - {%s}" % (ans, self.topk_yhat_prob),
        )

        ans = model.predict_topk(self.sf, output_type="rank", k=k)
        self.assertEqual(ans["class"].dtype, self.type)
        ans = ans.sort(["id", "class"])["rank"]
        self.assertEqual(list(ans), list(self.topk_yhat_rank))

        ans = model.predict_topk(self.sf, k=k)
        ans = ans.sort(["id", "class"])["probability"]
        self.assertTrue(
            np.allclose(ans, self.topk_yhat_prob, tol, tol),
            "{%s} - {%s}" % (ans, self.topk_yhat_prob),
        )

    def test_predict(self):
        """
        Check the prediction function against pre-computed answers. Check that
        all predictions are at most 1e-5 away from the true answers.
        """
        model = self.model
        tol = 1e-3

        if model.num_classes == 2:
            ans = model.predict(self.sf, output_type="margin")
            self.assertTrue(np.allclose(ans, self.yhat_margin, tol, tol))
            ans = model.predict(self.sf, output_type="probability")
            self.assertTrue(np.allclose(ans, self.yhat_prob, tol, tol))
        else:
            try:
                ans = model.predict(self.sf, output_type="margin")
            except ToolkitError:
                pass
            try:
                ans = model.predict(self.sf, output_type="probability")
            except ToolkitError:
                pass

        # Prob vector.
        ans = model.predict(self.sf, output_type="probability_vector")
        import itertools

        merged_ans = list(itertools.chain(*ans))
        self.assertTrue(np.allclose(merged_ans, self.yhat_prob_vec, tol, tol))

        # class
        ans = model.predict(self.sf, output_type="class")
        self.assertEqual(ans.dtype, self.type)
        self.assertTrue((ans == tc.SArray(list(map(self.type, self.yhat_class)))).all())

        # Default is class
        ans = model.predict(self.sf)
        self.assertEqual(ans.dtype, self.type)
        self.assertTrue((ans == tc.SArray(list(map(self.type, self.yhat_class)))).all())

    def test_classify(self):
        """
        Check the classify function against pre-computed answers. Check that
        all predictions are at most 1e-5 away from the true answers.
        """
        model = self.model
        ans = model.classify(self.sf)
        tol = 1e-3
        self.assertEqual(ans["class"].dtype, self.type)
        self.assertTrue(
            (ans["class"] == tc.SArray(list(map(self.type, self.yhat_class)))).all()
        )
        self.assertTrue(np.allclose(ans["probability"], self.yhat_max_prob, tol, tol))

    def test_evaluate(self):
        """
        Make sure that evaluate works.
        """
        model = self.model

        def check_cf_matrix(ans):
            self.assertTrue(ans is not None)
            self.assertTrue("confusion_matrix" in ans)
            cf = ans["confusion_matrix"].sort(["target_label", "predicted_label"])
            self.assertTrue(
                np.allclose(cf["count"], self.sm_metrics["confusion_matrix"])
            )

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
        ans = model.evaluate(self.sf)
        self.assertEqual(sorted(ans.keys()), sorted(self.metrics))
        for m in self.metrics:
            check_metric(ans, m)

        # Individual
        for m in self.metrics:
            ans = model.evaluate(self.sf, metric=m)
            check_metric(ans, m)

    def test_save_and_load(self):
        """
        Make sure saving and loading retains everything.
        """
        filename = "save_file{}".format(uuid.uuid4())
        self.model.save(filename)
        self.model = tc.load_model(filename)

        try:
            self.test_get()
            print("Get passed")
            self.test_summary()
            print("Summary passed")
            self.test_repr()
            print("Repr passed")
            self.test_predict()
            print("Classify passed")
            self.test_classify()
            print("Predict passed")
            self.test_evaluate()
            print("Evaluate passed")
            self.test__list_fields()
            print("List fields passed")
            shutil.rmtree(filename)
        except:
            shutil.rmtree(filename)
            self.assertTrue(False)


class LogisticRegressionCreateTest(unittest.TestCase):
    """
    Unit test class for testing a Logistic Regression model that is not trained.
    """

    @classmethod
    def setUpClass(self):
        """
        Setup required for all tests that don't require an trained model.
        """

        np.random.seed(8)
        n, d = 100, 10
        self.sf = tc.SFrame()

        for i in range(d):
            self.sf.add_column(tc.SArray(np.random.rand(n)), inplace=True)

        target = np.random.randint(2, size=n)
        target[0] = 0
        target[1] = 1

        # the correct model
        sm_model = sm.GLM(
            target,
            sm.add_constant(self.sf.to_dataframe()),
            family=sm.families.Binomial(),
        ).fit()
        self.loss = -sm_model.llf
        self.coef = list(sm_model.params)
        self.stderr = list(sm_model.bse)

        # turicreate model parameters
        self.def_kwargs = copy.deepcopy(_DEFAULT_SOLVER_OPTIONS)
        self.def_kwargs["max_iterations"] = 100
        self.def_kwargs["convergence_threshold"] = 1e-5
        self.sf["target"] = target
        self.solver = "newton"
        self.features = ["X{}".format(i) for i in range(1, d + 1)]
        self.target = "target"

    def _test_create(self, sf, target, features, solver, kwargs, rescaling):
        """
        Test logistic regression create.
        """

        model = tc.logistic_classifier.create(
            sf,
            target,
            features,
            solver=solver,
            l2_penalty=0.0,
            verbose=True,
            validation_set=None,
            **kwargs
        )

        test_case = "solver = {}, kwargs = {}".format(solver, kwargs)
        self.assertTrue(model is not None)
        self.assertTrue(
            abs(model.training_loss - self.loss) < 0.01 * abs(self.loss),
            "Loss failed: {}".format(test_case),
        )
        coefs = model.coefficients
        coefs_list = list(coefs["value"])
        self.assertTrue(np.allclose(coefs_list, self.coef, rtol=2e-01, atol=2e-01))
        if solver == "newton":
            stderr_list = list(model.coefficients["stderr"])
            self.assertTrue(
                np.allclose(stderr_list, self.stderr, rtol=2e-01, atol=2e-01)
            )
        else:
            self.assertTrue("stderr" in coefs.column_names())
            self.assertEqual(list(coefs["stderr"]), [None for v in coefs_list])

    def test_create_default_features(self):
        """
        Test logistic regression create.
        """

        for solver in ["newton", "fista", "lbfgs"]:
            args = (self.sf, self.target, None, solver, self.def_kwargs, True)
            self._test_create(*args)
            args = (self.sf, self.target, None, solver, self.def_kwargs, False)
            self._test_create(*args)

    def test_create(self):
        """
        Test logistic regression create.
        """
        for solver in ["newton", "fista", "lbfgs"]:
            args = (self.sf, self.target, self.features, solver, self.def_kwargs, True)
            self._test_create(*args)
            args = (self.sf, self.target, self.features, solver, self.def_kwargs, False)
            self._test_create(*args)

    def test_class_weights(self):
        """
        Test svm create.
        """

        # Should train correctly
        model = tc.logistic_classifier.create(
            self.sf, self.target, self.features, class_weights="auto"
        )
        model = tc.logistic_classifier.create(
            self.sf, self.target, self.features, class_weights={0: 1, 1: 2}
        )
        # Should fail
        try:
            model = tc.logistic_classifier.create(
                self.sf, self.target, self.features, class_weights=1.0
            )
        except ToolkitError:
            pass
        try:
            model = tc.logistic_classifier.create(
                self.sf, self.target, self.features, class_weights={2: 10}
            )
        except ToolkitError:
            pass
        try:
            model = tc.logistic_classifier.create(
                self.sf, self.target, self.features, class_weights=[1, 1]
            )
        except ToolkitError:
            pass

    def test_lbfgs(self):
        solver = "lbfgs"
        kwargs = self.def_kwargs.copy()
        for m in [3, 5, 9, 21]:
            kwargs["lbfgs_memory_level"] = m
            args = (self.sf, self.target, self.features, solver, kwargs, True)
            self._test_create(*args)
            args = (self.sf, self.target, self.features, solver, kwargs, False)
            self._test_create(*args)

    def test_init_residual_of_zero(self):
        X = tc.SFrame({"col1": [2.0, 1.0, 2.0, 1.0], "target": [1, 1, 2, 2]})

        # Try all three solvers
        tc.logistic_classifier.create(X, target="target", solver="newton")
        tc.logistic_classifier.create(X, target="target", solver="lbfgs")
        tc.logistic_classifier.create(X, target="target", solver="fista")


class ListCategoricalLogisticRegressionTest(unittest.TestCase):
    """
    Unit test class for testing logistic regression with a categorical feature.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up (run once).
        """

        ## Create fake data with a categorical variable
        np.random.seed(15)
        n, d = 100, 3
        self.sf = tc.SFrame()

        # float columns
        for i in range(d):
            self.sf.add_column(tc.SArray(np.random.rand(n)), inplace=True)

        # categorical column
        species = np.array(["cat", "dog", "foosa"])
        idx = np.random.randint(3, size=n)
        # Stats models maps categorical in alphabetical order of categories.
        # We do it in the order of appearance.
        idx[0] = 0
        idx[1] = 1
        idx[2] = 2
        self.sf["species"] = list(species[idx])

        # target column
        target = np.random.randint(2, size=n)
        target[0] = 0
        target[1] = 1
        self.sf["target"] = target

        ## Get the right answer with statsmodels
        df = self.sf.to_dataframe()
        formula = "target ~ species + " + " + ".join(
            ["X{}".format(i + 1) for i in range(d)]
        )
        sm_model = smf.glm(formula, data=df, family=sm.families.Binomial()).fit()
        self.loss = -sm_model.llf
        self.coef = list(sm_model.params)
        self.yhat = np.array([1 if x >= 0.5 else 0 for x in sm_model.fittedvalues])

        ## Set the turicreate model params
        self.target = "target"
        self.features = ["species", "X1", "X2", "X3"]
        self.def_kwargs = copy.deepcopy(_DEFAULT_SOLVER_OPTIONS)
        self.sf["species"] = self.sf["species"].apply(lambda x: [x])

    def _test_coefficients(self, model):
        """
        Check that the coefficient values are very close to the correct values.
        """
        coefs = model.coefficients
        coef_list = list(coefs["value"])
        self.assertTrue(
            np.allclose(coef_list, self.coef, rtol=1e-02, atol=1e-02),
            "value values are incorrect. {} vs {}".format(self.coef, coef_list),
        )

    def _test_create(self, sf, target, features, solver, kwargs, rescaling):
        """
        Test logistic regression create function for a particular set of inputs.
        """

        test_label = "solver: {}\tkwargs: {}".format(solver, kwargs)
        model = tc.logistic_classifier.create(
            sf,
            target,
            features,
            l2_penalty=0.0,
            solver=solver,
            feature_rescaling=rescaling,
            validation_set=None,
            **kwargs
        )

        self.assertTrue(model is not None)
        loss_diff = abs(model.training_loss - self.loss)
        self.assertTrue(
            loss_diff < self.def_kwargs["convergence_threshold"],
            "Loss failed: {}".format(test_label),
        )
        self._test_coefficients(model)

    def test_create(self):
        """
        Driver for testing create function under various inputs.
        """

        for solver in ["newton"]:
            self._test_create(
                self.sf, self.target, self.features, solver, self.def_kwargs, True
            )
            self._test_create(
                self.sf, self.target, self.features, solver, self.def_kwargs, False
            )


class CategoricalLogisticRegressionTest(unittest.TestCase):
    """
    Unit test class for testing logistic regression with a categorical feature.
    """

    @classmethod
    def setUpClass(self):
        """
        Set up (run once).
        """

        ## Create fake data with a categorical variable
        np.random.seed(15)
        n, d = 100, 3
        self.sf = tc.SFrame()

        # float columns
        for i in range(d):
            self.sf.add_column(tc.SArray(np.random.rand(n)), inplace=True)

        # categorical column
        species = np.array(["cat", "dog", "foosa"])
        idx = np.random.randint(3, size=n)
        # Stats models maps categorical in alphabetical order of categories.
        # We do it in the order of appearance.
        idx[0] = 0
        idx[1] = 1
        idx[2] = 2
        self.sf["species"] = list(species[idx])

        # target column
        target = np.random.randint(2, size=n)
        target[0] = 0
        target[1] = 1
        self.sf["target"] = target

        ## Get the right answer with statsmodels
        df = self.sf.to_dataframe()
        formula = "target ~ species + " + " + ".join(
            ["X{}".format(i + 1) for i in range(d)]
        )
        sm_model = smf.glm(formula, data=df, family=sm.families.Binomial()).fit()
        self.loss = -sm_model.llf
        self.coef = list(sm_model.params)
        self.yhat = np.array([1 if x >= 0.5 else 0 for x in sm_model.fittedvalues])

        ## Set the turicreate model params
        self.target = "target"
        self.features = ["species", "X1", "X2", "X3"]
        self.def_kwargs = copy.deepcopy(_DEFAULT_SOLVER_OPTIONS)

    def _test_coefficients(self, model):
        """
        Check that the coefficient values are very close to the correct values.
        """
        coefs = model.coefficients
        coef_list = list(coefs["value"])
        self.assertTrue(
            np.allclose(coef_list, self.coef, rtol=1e-02, atol=1e-02),
            "value values are incorrect. {} vs {}".format(self.coef, coef_list),
        )

    def _test_create(self, sf, target, features, solver, kwargs, rescaling):
        """
        Test logistic regression create function for a particular set of inputs.
        """

        test_label = "solver: {}\tkwargs: {}".format(solver, kwargs)
        model = tc.logistic_classifier.create(
            sf,
            target,
            features,
            l2_penalty=0.0,
            solver=solver,
            feature_rescaling=rescaling,
            validation_set=None,
            **kwargs
        )

        self.assertTrue(model is not None)
        loss_diff = abs(model.training_loss - self.loss)
        self.assertTrue(
            loss_diff < self.def_kwargs["convergence_threshold"],
            "Loss failed: {}".format(test_label),
        )
        self._test_coefficients(model)

    def test_create(self):
        """
        Driver for testing create function under various inputs.
        """

        for solver in ["newton"]:
            self._test_create(
                self.sf, self.target, self.features, solver, self.def_kwargs, True
            )
            self._test_create(
                self.sf, self.target, self.features, solver, self.def_kwargs, False
            )

    def test_predict_extra_cols(self):

        sf = self.sf[:]
        model = tc.logistic_classifier.create(
            sf, self.target, self.features, feature_rescaling=False
        )
        pred = model.predict(sf)
        sf["species"] = sf["species"].apply(lambda x: "rat" if x == "foosa" else x)
        pred = model.predict(sf)

    def test_evaluate_extra_cols(self):

        sf = self.sf[:]
        model = tc.logistic_classifier.create(
            sf, self.target, self.features, feature_rescaling=False
        )
        eval1 = model.evaluate(sf)
        sf["species"] = sf["species"].apply(lambda x: "rat" if x == "foosa" else x)
        eval2 = model.evaluate(sf)

    """
       Test detection of columns that are almost the same.
    """

    def test_zero_variance_detection(self):
        sf = self.sf[:]
        sf["error-column"] = "1"
        model = tc.logistic_classifier.create(sf, self.target)

        sf["error-column"] = [[1] for i in sf]
        model = tc.logistic_classifier.create(sf, self.target)

        sf["error-column"] = [{1: 1} for i in sf]
        model = tc.logistic_classifier.create(sf, self.target)

    """
       Test detection of columns have nan values
    """

    def test_nan_detection(self):
        sf = self.sf[:]
        try:
            sf["error-column"] = np.nan
            model = tc.logistic_classifier.create(sf, self.target)
        except ToolkitError:
            pass
        try:
            sf["error-column"] = [[np.nan] for i in sf]
            model = tc.logistic_classifier.create(sf, self.target)
        except ToolkitError:
            pass
        try:
            sf["error-column"] = [{1: np.nan} for i in sf]
            model = tc.logistic_classifier.create(sf, self.target)
        except ToolkitError:
            pass


class VectorLogisticRegressionTest(unittest.TestCase):
    """
    Unit test class for testing a Logistic Regression create function.
  """

    @classmethod
    def setUpClass(self):
        """
        Set up (Run only once)
    """
        np.random.seed(15)
        n, d = 100, 3
        self.sf = tc.SFrame()
        for i in range(d):
            self.sf.add_column(tc.SArray(np.random.rand(n)), inplace=True)
        target = np.random.randint(2, size=n)
        target[0] = 0
        target[1] = 1
        self.sf["target"] = target

        ## Get the right answer with statsmodels
        df = self.sf.to_dataframe()
        formula = "target ~ " + " + ".join(["X{}".format(i + 1) for i in range(d)])
        sm_model = smf.glm(formula, data=df, family=sm.families.Binomial()).fit()

        self.loss = -sm_model.llf  # sum of squared residuals
        self.coef = list(sm_model.params)
        self.stderr = list(sm_model.bse)
        self.yhat = list(sm_model.fittedvalues)

        ## Set the turicreate model params
        self.target = "target"
        self.sf["vec"] = self.sf.apply(
            lambda row: [row["X{}".format(i + 1)] for i in range(d)]
        )
        self.sf["vec"] = self.sf["vec"].apply(lambda x: x, array.array)

        self.features = ["vec"]
        self.unpacked_features = ["vec[%s]" % (i) for i in range(d)]
        self.def_kwargs = copy.deepcopy(_DEFAULT_SOLVER_OPTIONS)

    def _test_coefficients(self, model):
        """
      Check that the coefficient values are very close to the correct values.
      """
        coefs = model.coefficients
        coef_list = list(coefs["value"])
        stderr_list = list(coefs["stderr"])
        self.assertTrue(np.allclose(coef_list, self.coef, rtol=1e-02, atol=1e-02))
        self.assertTrue(np.allclose(stderr_list, self.stderr, rtol=1e-02, atol=1e-02))

    def _test_create(self, sf, target, features, solver, kwargs, rescaling):

        model = tc.logistic_classifier.create(
            sf,
            target,
            features,
            solver=solver,
            l2_penalty=0.0,
            feature_rescaling=rescaling,
            validation_set=None,
            **kwargs
        )

        test_case = "solver = {solver}, kwargs = {kwargs}".format(
            solver=solver, kwargs=kwargs
        )
        self.assertTrue(model is not None)
        self.assertTrue(
            abs(model.training_loss - self.loss) < 0.01 * abs(self.loss),
            "Loss failed: %s. Expected %s" % (test_case, self.loss),
        )
        self._test_coefficients(model)

    def test_create(self):

        for solver in ["newton"]:
            args = (self.sf, self.target, self.features, solver, self.def_kwargs, True)
            self._test_create(*args)
            args = (self.sf, self.target, self.features, solver, self.def_kwargs, False)
            self._test_create(*args)

    def test_features(self):

        model = tc.logistic_classifier.create(
            self.sf, self.target, self.features, feature_rescaling=False
        )
        self.assertEqual(model.num_features, len(self.features))
        self.assertEqual(model.features, self.features)
        self.assertEqual(model.num_unpacked_features, len(self.unpacked_features))
        self.assertEqual(model.unpacked_features, self.unpacked_features)


class DictLogisticRegressionTest(unittest.TestCase):
    """
    Unit test class for testing a Logistic Regression create function.
  """

    @classmethod
    def setUpClass(self):
        """
        Set up (Run only once)
    """

        np.random.seed(15)
        n, d = 100, 3
        self.sf = tc.SFrame()

        # float columns
        for i in range(d):
            self.sf.add_column(tc.SArray(np.random.rand(n)), inplace=True)

        # target column
        target = np.random.randint(2, size=n)
        target[0] = 0
        target[1] = 1
        self.sf["target"] = target

        ## Get the right answer with statsmodels
        df = self.sf.to_dataframe()
        formula = "target ~ " + " + ".join(["X{}".format(i + 1) for i in range(d)])
        sm_model = smf.glm(formula, data=df, family=sm.families.Binomial()).fit()

        self.loss = -sm_model.llf  # sum of squared residuals
        self.coef = list(sm_model.params)
        self.stderr = list(sm_model.bse)
        self.yhat = list(sm_model.fittedvalues)

        ## Set the turicreate model params
        self.target = "target"
        self.sf["dict"] = self.sf.apply(
            lambda row: {i: row["X{}".format(i + 1)] for i in range(d)}
        )
        self.features = ["dict"]
        self.unpacked_features = ["dict[%s]" % i for i in range(d)]
        self.def_kwargs = copy.deepcopy(_DEFAULT_SOLVER_OPTIONS)

    def _test_coefficients(self, model):
        coefs = model.coefficients
        coef_list = list(coefs["value"])
        stderr_list = list(coefs["stderr"])
        self.assertTrue(np.allclose(coef_list, self.coef, rtol=1e-02, atol=1e-02))
        self.assertTrue(np.allclose(stderr_list, self.stderr, rtol=1e-02, atol=1e-02))

    def test_features(self):

        model = tc.logistic_classifier.create(
            self.sf, self.target, self.features, feature_rescaling=False
        )
        self.assertEqual(model.num_features, len(self.features))
        self.assertEqual(model.features, self.features)
        self.assertEqual(model.num_unpacked_features, len(self.unpacked_features))
        self.assertEqual(model.unpacked_features, self.unpacked_features)

    def _test_create(self, sf, target, features, solver, opts, rescaling):

        model = tc.logistic_classifier.create(
            sf,
            target,
            features,
            solver=solver,
            l2_penalty=0.0,
            feature_rescaling=rescaling,
            validation_set=None,
            **opts
        )
        test_case = "solver = {solver}, opts = {opts}".format(solver=solver, opts=opts)
        self.assertTrue(model is not None)
        self.assertTrue(
            abs(model.training_loss - self.loss) < 0.01 * abs(self.loss),
            "Loss failed: %s. Expected %s" % (test_case, self.loss),
        )
        self._test_coefficients(model)

    def test_create(self):

        for solver in ["newton"]:
            args = (self.sf, self.target, self.features, solver, self.def_kwargs, True)
            self._test_create(*args)
            args = (self.sf, self.target, self.features, solver, self.def_kwargs, False)
            self._test_create(*args)

    def test_predict_extra_cols(self):

        model = tc.logistic_classifier.create(
            self.sf, self.target, self.features, feature_rescaling=False
        )
        pred = model.predict(self.sf)
        self.sf["dict"] = self.sf["dict"].apply(
            lambda x: dict(
                list(x.items()) + list({"extra_col": 0, "extra_col_2": 1}.items())
            )
        )
        pred2 = model.predict(self.sf)
        self.assertEqual(sum(pred - pred2), 0)
        self.sf["dict"] = self.sf["dict"].apply(
            lambda x: {
                k: v for k, v in x.items() if k not in ["extra_col", "extra_col_2"]
            }
        )

    def test_evaluate_extra_cols(self):

        model = tc.logistic_classifier.create(
            self.sf, self.target, self.features, feature_rescaling=False
        )
        eval1 = model.evaluate(self.sf)
        self.sf["dict"] = self.sf["dict"].apply(
            lambda x: dict(
                list(x.items()) + list({"extra_col": 0, "extra_col_2": 1}.items())
            )
        )
        eval2 = model.evaluate(self.sf)
        self.sf["dict"] = self.sf["dict"].apply(
            lambda x: {
                k: v for k, v in x.items() if k not in ["extra_col", "extra_col_2"]
            }
        )
        self.assertEqual(eval1["accuracy"], eval2["accuracy"])


class RegularizedLogisticRegressionTest(unittest.TestCase):
    """
    Unit test class for testing l2 regularization with logistic regression.
    """

    @classmethod
    def tearDownClass(self):
        os.remove(self.dataset)

    @classmethod
    def setUpClass(self):
        """
        Set up (run only once).
        """

        ## Fake data, generated in R
        feature_data = """0.723040834941846,1.0648961025071,-0.479191624484056,0.433073682915559
            -1.29705301514688,-0.0898754334392415,-0.244320454255808,-0.578687648218724
            -1.99524976461205,-0.125152158307165,-0.086446106920042,-0.233340479601935
            0.402456295304511,-0.550374347857019,1.35685637262204,0.544712458718116
            0.71292040284874,0.357692368398735,-0.328532299531591,1.1438885901337
            1.43568182594776,0.261766509482266,0.464001357444711,0.629315227017958
            1.14311885150198,0.655492113418554,-0.531778301235656,-1.07301671754922
            -0.727190240020983,-0.804833424641632,0.992937586282921,0.257164452843208
            0.496305648762312,-0.33026548145215,0.294213277292863,-0.148326616831932
            0.0715154210755435,3.10643735093654,-2.42302328502955,1.85362623599767
            0.439938175612892,0.360512495357457,0.911999213655342,0.580679106520842
            -0.53740664558619,-1.03479285736856,0.648106809698278,-2.08045579753764
            0.771390467805027,-0.365226832473157,0.212507592560423,0.605043405257671
            0.409068040844746,-1.11958892697221,0.251410438902745,1.49533148907976
            1.33452015182787,-0.221443420464248,0.684166879062637,0.405404283792913
            -0.814559334455869,0.622951200349794,-0.15579855510818,0.816581609925525
            0.0724112589836755,0.127117783915735,-1.69170802887266,1.15603901862971
            -1.34795259750591,0.32965875325895,-0.484014974747081,-0.0974675586113715
            1.34889605007481,0.0824357126688539,-1.09888981753882,0.0435212855912346
            -1.23451928446894,-2.02571782398023,1.69698953241631,0.311259332559498
            -1.08107490578277,2.01992049688993,0.686919510862818,0.171595543174128
            -0.370026916702062,0.649201106003441,1.08537694050599,-0.166308007519768
            0.604953265143402,-0.295134454954877,-1.37278291289961,-0.221943233504827
            -0.221502010455765,0.143243164432162,0.0865212922471775,-0.707486928716484
            0.130880062924443,-0.96220764081838,-0.812496924563519,0.034135944769292
            0.254267982814166,-1.32685703222374,-0.981633975802561,0.732836284618879
            1.265142842748,0.282934557141652,-0.0861758569290395,0.322481599076151
            -1.24752678321833,-0.784053211249692,0.223597102115654,1.43121199522732
            -0.110303717333287,0.435388364385118,1.08048691639869,0.428415119045148
            -0.307325193240204,0.0796089178713656,2.01143399535698,0.172849471698307
            0.901456547236732,0.117943142883823,1.1193628338247,0.169245377540655
            0.379266104548251,-0.525970142188443,0.451782795356222,-0.274921665462158
            0.186422788462521,-0.218091616952572,-0.555765737009514,-0.494331871587989
            -1.31074080509732,-0.6895670948282,0.381059426380244,-0.277553356042858
            1.53697865751165,-0.120967473867285,0.520731585380014,1.45857635613988
            0.033524683387432,-0.577793104407188,0.937785791241064,1.00920129732343
            -1.05879611910704,1.42056740074338,0.195885586149368,0.490520532394734
            -1.08881429525958,0.123824483210179,0.0956250180140426,1.76003460194415
            -1.29997849510857,1.75776513941181,1.7510694133533,-0.511502266420589
            -0.144255243364542,-0.40115575943564,-1.52523430152192,0.155554928487472
            1.59937651120117,0.529078766289062,-1.30470387055666,-0.314066282796635
            -0.518993199133024,-0.411140980260914,-0.946889104493446,0.347779326836616
            -0.397555059381936,0.325296711868571,0.00213770796198489,0.872267722389688
            0.593734428308537,0.549413784172661,-1.3475551964432,0.0749821701668647
            -0.820781157880031,0.891993295538893,-0.888995181665049,0.677545323189558
            -0.866657379164596,-0.0214602907130982,-0.0925579457431698,-0.792198092875795
            0.295852684868691,-1.72458284526813,-0.419566365605465,1.09781961286834
            -1.2198041946688,-0.950987998336847,0.533481290621865,0.506462738807015
            0.640722007441465,-0.0601424819428958,0.295459649680193,-0.0921462013443656
            -1.52605388273713,-0.543730079706389,0.131346071414916,0.738058577166992
            -0.45391090597556,0.48779651453639,1.120873932577,-0.713448586902679
            -0.628840370454681,0.0762760110821232,0.076486773914837,-1.72087444202454
            0.434105791577903,-0.174097386670335,-1.62210952809933,-0.572260040573192
            0.210959007638248,0.931253473020117,-1.55529523010342,1.12184635210952
            0.377172308826005,-0.193156019531129,-2.34672964583264,-1.69496327650004
            -0.768326979481981,0.090534779377819,1.40926469130814,0.185723353422034
            2.84451052966456,0.991081175802105,-0.671889834506259,-1.05648705124797
            1.07199047825086,-0.630788000094623,-0.295719980365804,0.00578414600098129
            0.567847756422627,-0.135356530281259,-1.8661182649549,-0.583604332794753
            -1.35727242162732,-0.75390249183435,-1.08556720847384,-0.440561116761817
            0.740254472870971,0.655286750465761,-0.204647020480224,0.071964407037729
            -0.360793786777967,0.159246915001863,-0.393548461938151,0.0816487134403937
            1.43222559303184,-1.71983895824847,1.09712139841872,-1.64139757629683
            -0.376935788510295,-0.806761492812103,0.384026357646247,-0.699593771978961
            0.0278784881937148,-0.724537272791918,-2.05555879783548,-1.72478240684875
            0.190044222323182,-1.56774354586837,0.916178290128054,-0.432491837161175
            -0.567234854615799,-1.28223438325058,1.63035694199303,0.548442154128947
            1.29513548361495,1.08475144022745,0.336767371349133,0.481833338619483
            -0.728389258087971,0.685279676658112,1.61450559899377,2.02071395906146
            0.793861332271289,-1.41000480231271,0.763251493485584,-0.155228958547631
            0.65302739002322,-0.485904056761054,-0.583954557844738,0.834766477570608
            -0.183270740543986,0.311902928274404,0.0985357652998081,-0.81070766679897
            -1.44372055216941,0.301956562391904,1.96606377671346,-0.108357109077201
            -0.151925447051968,-1.85524173745675,-0.553363598862492,0.249461227551983
            2.07720319452313,1.02644753848201,-0.489562460589595,-1.82071693364734
            -2.21384059315342,-0.572816646522423,1.14425496174664,1.43856739487412
            -3.76586767924992,-0.477052670757007,-0.773715096691185,-0.571345428242588
            -0.610862692763602,0.611778144535133,-0.838580512312718,0.955847632621329
            -0.45005942173923,-1.60840383057648,-0.411960530932751,-0.89201414449718
            0.437027691622469,-1.11112698008814,2.28488401670962,0.723732175834912
            1.51935637945586,0.279916735352714,-1.015744513005,-0.227850579603768
            1.51123609336915,0.509995861560829,-0.791506781683076,-0.714527596319846
            -0.247467045020083,-1.31808368333257,-1.57860422363563,-0.685303156093254
            0.330937542151292,-1.22321366752869,-1.85081236761593,-0.608695439762601
            -0.713937873344641,-1.61830265968122,-0.203553709845488,0.342748028693319
            -0.445096094774576,-0.170065676755747,1.21065773010851,0.370327297599183
            -0.396579992922086,-0.812285750659216,0.488101347014334,-0.930597115408531
            0.30407652171237,0.959922293378184,0.673393514196353,0.707876382192161
            -0.0153541194478992,0.770807367233966,0.567821014496558,1.00137512571836
            -1.01209407494884,-1.0046115801755,0.409919592485247,-0.967128108733911
            1.60319946103286,-0.317788211419659,-0.383481230436869,-0.175392291758827
            -0.76622750872221,1.62115080964559,0.634655587591233,0.613197236224363
            0.323159292013831,2.33414672599566,-0.248659447604274,-0.453463148852298
            0.17884811122143,1.08207426502955,0.60316676152225,-0.844963697357711
            -0.527650913007582,1.12339358028069,-0.140245975668798,0.672647943308228
            -0.706376609975316,0.361247970083208,-0.108594748820389,-1.54245677044285
            -0.313676473532486,0.244242322538692,-0.172553981996335,0.31935807851552
            -0.620909598452922,0.655163343467281,2.00816338389406,-0.422875475337577
            -0.339769903386523,0.189204653082022,-2.34980611959092,0.783263944917566
            1.19717835010489,0.479479297178576,-0.682999419503163,1.55590456330123"""
        target_data = [
            0,
            1,
            0,
            0,
            0,
            1,
            1,
            1,
            0,
            0,
            0,
            1,
            0,
            1,
            1,
            0,
            0,
            0,
            1,
            0,
            0,
            0,
            1,
            0,
            1,
            1,
            1,
            1,
            0,
            0,
            0,
            0,
            0,
            1,
            0,
            1,
            0,
            0,
            0,
            1,
            1,
            0,
            0,
            1,
            0,
            0,
            0,
            1,
            0,
            1,
            1,
            1,
            1,
            1,
            1,
            0,
            0,
            1,
            0,
            0,
            0,
            1,
            0,
            0,
            1,
            0,
            1,
            0,
            0,
            1,
            0,
            0,
            1,
            1,
            0,
            1,
            0,
            0,
            1,
            1,
            0,
            0,
            1,
            0,
            0,
            1,
            1,
            0,
            0,
            1,
            1,
            0,
            1,
            0,
            0,
            0,
            0,
            0,
            0,
            1,
        ]

        ## Write data to file so it can be loaded into an SFrame
        f_data = "data_file_{}.csv".format(uuid.uuid4())
        self.dataset = f_data
        with open(f_data, "w") as f:
            f.write(feature_data)

        ## Load the data into an SFrame
        self.sf = tc.SFrame.read_csv(f_data, header=False, column_type_hints=float)
        self.sf["target"] = target_data

        ## Default options
        self.def_kwargs = copy.deepcopy(_DEFAULT_SOLVER_OPTIONS)
        self.def_kwargs["max_iterations"] = 100
        self.def_kwargs["convergence_threshold"] = 1e-5
        self.l2_penalty = 5.0
        self.l1_penalty = 3.0

        ## Constant parameters
        self.target = "target"
        self.features = ["X{}".format(i) for i in range(1, 4 + 1)]
        self.solver = "auto"

        ## Correct answers, from glmnet in R
        ## require(glmnet)
        ## fit = glmnet(x, y, family='binomial', alpha=0, lambda=0.1, standardize=False)
        ## Note: l2_penalty is 0.1 in R but 5 here because a) the penalty in
        #  glmnet is lambda/2, and b) the loss in glmnet is the log-likelihood/n + penalty.
        self.l2_coef = np.array(
            [-0.3554688, 0.06594038, -0.48338736, -0.11910414, -0.09901472]
        )

        ## fit = glmnet(x, y, family='binomial', alpha=1.0, lambda=0.03, standardize=False)
        ## Note: l1 penalty is 0.03 in R but 3 here because the loss in glmnet
        #  is log-lik/n + penalty.
        self.l1_coef = np.array([-0.3728739, 0.0, -0.58645032, -0.07656562, 0.0])

    def _test_l2_create(self, sf, target, features, solver, opts, l2_penalty):
        """
        Test l2-regularized logistic regression create under particular
        parameter settings.
        """

        test_case = "solver = {}, opts = {}".format(solver, opts)
        model = tc.logistic_classifier.create(
            sf,
            target,
            features,
            l2_penalty=l2_penalty,
            l1_penalty=0.0,
            solver=solver,
            feature_rescaling=False,
            validation_set=None,
            **opts
        )
        coefs = list(model.coefficients["value"])
        self.assertTrue(model is not None)
        self.assertTrue(np.allclose(coefs, self.l2_coef, rtol=1e-02, atol=1e-02))

    def _test_l1_create(self, sf, target, features, solver, opts, l1_penalty):
        """
        Test l1-regularized logistic regression create under particular
        parameter settings.
        """

        test_case = "solver = {}, opts = {}".format(solver, opts)
        model = tc.logistic_classifier.create(
            sf,
            target,
            features,
            l2_penalty=0.0,
            l1_penalty=l1_penalty,
            solver=solver,
            feature_rescaling=False,
            validation_set=None,
            **opts
        )
        coefs = list(model.coefficients["value"])
        self.assertTrue(model is not None)
        self.assertTrue(np.allclose(coefs, self.l1_coef, rtol=1e-02, atol=1e-02))

    def test_create(self):
        """
        Test logistic regression create for a variety of solvers with l2
        regularization.
        """

        for solver in ["newton", "lbfgs", "fista"]:
            self._test_l2_create(
                self.sf,
                self.target,
                self.features,
                solver,
                self.def_kwargs,
                self.l2_penalty,
            )
        for solver in ["fista"]:
            self._test_l1_create(
                self.sf,
                self.target,
                self.features,
                solver,
                self.def_kwargs,
                self.l1_penalty,
            )


class ImproperProblemsTest(unittest.TestCase):
    """
    Unit test class for problems with the setup, e.g. dataset.
  """

    @classmethod
    def setUpClass(self):
        """
        Set up (Run only once)
    """

        self.target = "y"
        self.sf = tc.SFrame()
        self.sf["y"] = tc.SArray([0, 1, 0], int)
        self.sf["int"] = tc.SArray([1, 2, 3], int)
        self.sf["float"] = tc.SArray([1, 2, 3], float)
        self.sf["dict"] = tc.SArray([{"1": 3, "2": 2}, {"2": 1}, {}], dict)
        self.sf["array"] = tc.SArray([[1, 2], [3, 4], [5, 6]], array.array)
        self.sf["str"] = tc.SArray(["1", "2", "3"], str)
        print(self.sf)

    """
     Test predict missing value
  """

    def test_single_label_error(self):
        sf = self.sf.__copy__()
        sf["y"] = tc.SArray.from_const(0, 3)
        with self.assertRaises(ToolkitError):
            m = tc.logistic_classifier.create(sf, "y")


class ValidationSetLogisticClassifierTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        ## Simulate test data
        np.random.seed(10)
        n, d = 100, 10
        self.sf = tc.SFrame()
        for i in range(d):
            self.sf.add_column(tc.SArray(np.random.randn(n)), inplace=True)
        target = np.random.randint(2, size=n)
        target[0] = 0
        target[1] = 1

        ## Create the model
        self.sf["target"] = target
        self.target = "target"

    def test_valid_set(self):

        model = tc.logistic_classifier.create(
            self.sf, target="target", validation_set="auto"
        )
        self.assertTrue(model is not None)
        self.assertTrue(isinstance(model.progress, tc.SFrame))

        model = tc.logistic_classifier.create(
            self.sf, target="target", validation_set=self.sf
        )
        self.assertTrue(model is not None)
        self.assertTrue(isinstance(model.progress, tc.SFrame))

        valid_set = self.sf.head(5)
        valid_set[self.target] = 0
        model = tc.logistic_classifier.create(
            self.sf, target="target", validation_set=valid_set
        )

        self.assertTrue(model is not None)
        self.assertTrue(isinstance(model.progress, tc.SFrame))

        model = tc.logistic_classifier.create(
            self.sf, target="target", validation_set=None
        )
        self.assertTrue(model is not None)
        self.assertTrue(isinstance(model.progress, tc.SFrame))

        # Test that an error is thrown when you use a uncorrectly formatted
        # validation set.
        with self.assertRaises(RuntimeError):
            validation_set = self.sf.__copy__()
            validation_set["X1"] = validation_set["X1"].astype(str)
            model = tc.logistic_classifier.create(
                self.sf, target="target", validation_set=validation_set
            )


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
        model = tc.logistic_classifier.create(sf, "target")

        # Act
        evaluation = model.evaluate(sf)

        # Assert
        self.assertEqual(
            ["cat-0", "cat-1"],
            sorted(list(evaluation["confusion_matrix"]["target_label"].unique())),
        )
