# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import turicreate
from sklearn.metrics import (
    fbeta_score,
    recall_score,
    precision_score,
    accuracy_score,
    f1_score,
    roc_auc_score,
)
from turicreate.toolkits._main import ToolkitError
import math
from numpy import inf
import numpy as np

DELTA = 0.00001


def _round_scores(p):
    """
    Since turicreate uses fixed threshold (100001) and scikit-learn
    uses all threshold for things like AUC, we might get slightly different
    results if some scores fall between the cracks. To make sure this does
    not happen during our unit tests, we snap numbers to the centers between
    thresholds.
    """
    return (np.round(p, decimals=5) + 0.5 / 100001).clip(max=1)


def _generate_classes_and_scores(num_classes, n, seed=42, hard_predictions=False):
    rs = np.random.RandomState(seed)
    t = rs.randint(num_classes, size=n)
    if hard_predictions:
        p = rs.randint(num_classes, size=n)
    else:
        p = rs.uniform(size=(n, num_classes))
        p /= p.sum(-1, keepdims=True)
        p = _round_scores(p)
    return t, p


class MetricsTest(unittest.TestCase):
    def test_rmse(self):

        y = turicreate.SArray([1, 2, 1, 2])
        yhat = turicreate.SArray([3, -1, 1, 0])
        rmse = turicreate.toolkits.evaluation.rmse(y, yhat)
        true_rmse = (float(2 * 2 + 3 * 3 + 0 + 2 * 2) / 4) ** 0.5
        self.assertAlmostEqual(rmse, true_rmse)

    def test_log_loss(self):

        # Binary classification
        y = turicreate.SArray([1, 1, 0, 1, 1])
        yhat = turicreate.SArray([0.5, 0.2, 0.8, 0.3, 0.9])

        logp = yhat.apply(lambda x: math.log(x))
        log1mp = yhat.apply(lambda x: math.log(1 - x))
        expected = -(y * logp + (1 - y) * log1mp).mean()

        observed = turicreate.toolkits.evaluation.log_loss(y, yhat)
        self.assertAlmostEqual(expected, observed)

        # Binary classification, string
        y = turicreate.SArray([1, 1, 0, 1, 1])
        yhat = turicreate.SArray([0.5, 0.2, 0.8, 0.3, 0.9])

        logp = yhat.apply(lambda x: math.log(x))
        log1mp = yhat.apply(lambda x: math.log(1 - x))
        expected = -(y * logp + (1 - y) * log1mp).mean()

        observed = turicreate.toolkits.evaluation.log_loss(y.astype(str), yhat)
        self.assertAlmostEqual(expected, observed)

        # Binary classification vs sklearn example
        y = turicreate.SArray([1, 0, 0, 1])
        yhat = turicreate.SArray([[0.1, 0.9], [0.9, 0.1], [0.8, 0.2], [0.35, 0.65]])

        expected = (
            -(math.log(0.9) + math.log(0.9) + math.log(0.8) + math.log(0.65)) / 4.0
        )

        observed = turicreate.toolkits.evaluation.log_loss(y, yhat)
        self.assertAlmostEqual(expected, observed)

        # Binary classification with missing data
        y = turicreate.SArray([1, None, None, 1])
        yhat = turicreate.SArray([[0.1, 0.9], [0.9, 0.1], [0.8, 0.2], [0.35, 0.65]])

        expected = (
            -(math.log(0.9) + math.log(0.9) + math.log(0.8) + math.log(0.65)) / 4.0
        )

        observed = turicreate.toolkits.evaluation.log_loss(y, yhat)
        self.assertAlmostEqual(expected, observed)

        # Multiclass
        y = turicreate.SArray([0, 1, 2, 0])
        yhat = turicreate.SArray(
            [[0.5, 0.1, 0.4], [0.5, 0.1, 0.4], [0.1, 0.5, 0.4], [0.2, 0.3, 0.5]]
        )

        true_probs = [yhat[0][0], yhat[1][1], yhat[2][2], yhat[3][0]]
        expected = -sum([math.log(x) for x in true_probs]) / 4.0

        observed = turicreate.toolkits.evaluation.log_loss(y, yhat)
        self.assertAlmostEqual(expected, observed)

        # Multiclass with strings
        y = turicreate.SArray(["a", "b", "c", "a"])
        yhat = turicreate.SArray(
            [[0.5, 0.1, 0.4], [0.5, 0.1, 0.4], [0.1, 0.5, 0.4], [0.2, 0.3, 0.5]]
        )

        true_probs = [yhat[0][0], yhat[1][1], yhat[2][2], yhat[3][0]]
        expected = -sum([math.log(x) for x in true_probs]) / 4.0

        observed = turicreate.toolkits.evaluation.log_loss(y, yhat)
        self.assertAlmostEqual(expected, observed)

        # Multiclass with strings and explicit index map
        y = turicreate.SArray(["a", "c", "d", "a"])
        yhat = turicreate.SArray(
            [
                [0.5, 0.0, 0.1, 0.4],
                [0.5, 0.0, 0.1, 0.4],
                [0.1, 0.0, 0.5, 0.4],
                [0.2, 0.0, 0.3, 0.5],
            ]
        )
        index_map = {"a": 0, "b": 1, "c": 2, "d": 3}

        true_probs = [yhat[0][0], yhat[1][2], yhat[2][3], yhat[3][0]]
        expected = -sum([math.log(x) for x in true_probs]) / 4.0

        observed = turicreate.toolkits.evaluation.log_loss(y, yhat, index_map=index_map)
        self.assertAlmostEqual(expected, observed)

    def test_logloss_clipping(self):

        y = turicreate.SArray([0, 1, 2, 0])
        yhat = turicreate.SArray(
            [[0.9, 0.0, 0.1], [0.8, 0.1, 0.1], [0.1, 0.1, 0.8], [0.1, 0.1, 0.8]]
        )
        log_loss = turicreate.toolkits.evaluation.log_loss(y, yhat)
        self.assertTrue(log_loss != inf)

        y = turicreate.SArray([0, 1, 2, 0])
        yhat = turicreate.SArray(
            [[1.0, 0.0, 0.0], [0.8, 0.1, 0.1], [0.1, 0.1, 0.8], [0.1, 0.1, 0.8]]
        )

        log_loss = turicreate.toolkits.evaluation.log_loss(y, yhat)
        self.assertTrue(log_loss != inf)

        y = turicreate.SArray([0, 1, 0, 0])
        yhat = turicreate.SArray([0.0, 0.9, 0.1, 0.1])
        log_loss = turicreate.toolkits.evaluation.log_loss(y, yhat)
        self.assertTrue(log_loss != inf)

        y = turicreate.SArray([0, 1, 0, 0])
        yhat = turicreate.SArray([0.1, 1.0, 0.1, 0.1])
        log_loss = turicreate.toolkits.evaluation.log_loss(y, yhat)
        self.assertTrue(log_loss != inf)

    def test_probabilities_with_index_map(self):

        y = turicreate.SArray([0, 2, 3, 0])
        yhat = turicreate.SArray(
            [
                [0.9, 0.0, 0.0, 0.1],
                [0.8, 0.0, 0.1, 0.1],
                [0.1, 0.0, 0.1, 0.8],
                [0.1, 0.0, 0.1, 0.8],
            ]
        )

        # The evaluation toolkit must know that 1 is a possible class label, and
        # corresponds to index 1 in each probability vector.
        index_map = {i: i for i in range(4)}

        log_loss = turicreate.toolkits.evaluation.log_loss(y, yhat, index_map=index_map)
        auc = turicreate.toolkits.evaluation.auc(y, yhat, index_map=index_map)
        roc_curve = turicreate.toolkits.evaluation.roc_curve(
            y, yhat, index_map=index_map
        )

    def test_integer_probabilities(self):

        y = turicreate.SArray([0, 1, 2, 0])
        yhat = turicreate.SArray([[1, 0, 1], [1, 0, 1], [0, 1, 1], [0, 0, 1]])

        log_loss = turicreate.toolkits.evaluation.log_loss(y, yhat)
        auc = turicreate.toolkits.evaluation.auc(y, yhat)
        roc_curve = turicreate.toolkits.evaluation.roc_curve(y, yhat)

        y = turicreate.SArray([0, 1, 0, 0])
        yhat = turicreate.SArray([0, 1, 0, 0])

        turicreate.toolkits.evaluation.log_loss(y, yhat)
        turicreate.toolkits.evaluation.auc(y, yhat)
        turicreate.toolkits.evaluation.roc_curve(y, yhat)

    def test_none_probabilities(self):

        # Test the case when probabilities are integer (sigh!)
        y = turicreate.SArray([0, 1, 2, 0])
        yhat = turicreate.SArray(
            [[1, 0, None], [1, 0, None], [0, 1, None], [0, 0, None]]
        )

        with self.assertRaises(TypeError):
            log_loss = turicreate.toolkits.evaluation.log_loss(y, yhat)
        with self.assertRaises(TypeError):
            auc = turicreate.toolkits.evaluation.auc(y, yhat)
        with self.assertRaises(TypeError):
            roc_curve = turicreate.toolkits.evaluation.roc_curve(y, yhat)

        # Test the case when probabilities are integer (sigh!)
        y = turicreate.SArray([0, 1, 2, 0])
        yhat = turicreate.SArray(
            [[0.9, 0.0, 0.0], [0.9, 0.1, 0.0], None, [0.0, 0.1, 0.9]]
        )

        with self.assertRaises(ToolkitError):
            turicreate.toolkits.evaluation.log_loss(y, yhat)
        with self.assertRaises(ToolkitError):
            turicreate.toolkits.evaluation.auc(y, yhat)
        with self.assertRaises(ToolkitError):
            turicreate.toolkits.evaluation.roc_curve(y, yhat)

        # Test the case when probabilities are integer (sigh!)
        y = turicreate.SArray([0, 1, 0, 0])
        yhat = turicreate.SArray([0, 1, 0, None])

        with self.assertRaises(ToolkitError):
            turicreate.toolkits.evaluation.log_loss(y, yhat)
        with self.assertRaises(ToolkitError):
            turicreate.toolkits.evaluation.auc(y, yhat)
        with self.assertRaises(ToolkitError):
            turicreate.toolkits.evaluation.roc_curve(y, yhat)

        # Test the case when probabilities are integer (sigh!)
        y = turicreate.SArray([0, 1, 0, 0])
        yhat = turicreate.SArray([0.1, 0.1, 0.9, None])

        with self.assertRaises(ToolkitError):
            turicreate.toolkits.evaluation.log_loss(y, yhat)
        with self.assertRaises(ToolkitError):
            turicreate.toolkits.evaluation.auc(y, yhat)
        with self.assertRaises(ToolkitError):
            turicreate.toolkits.evaluation.roc_curve(y, yhat)

    def test_none_prec_recall_scores_binary(self):
        # Arrange
        y = turicreate.SArray([0, 1])
        yhat = turicreate.SArray([0, 0])

        # Act
        pr = turicreate.toolkits.evaluation.precision(y, yhat)
        rec = turicreate.toolkits.evaluation.recall(y, yhat)
        f1 = turicreate.toolkits.evaluation.f1_score(y, yhat)
        fbeta = turicreate.toolkits.evaluation.fbeta_score(y, yhat, beta=2.0)

        # Assert
        self.assertEqual(pr, None)
        self.assertEqual(rec, 0.0)
        self.assertEqual(f1, 0.0)
        self.assertEqual(fbeta, 0.0)

        # Arrange
        y = turicreate.SArray([0, 0])
        yhat = turicreate.SArray([0, 1])

        # Act
        pr = turicreate.toolkits.evaluation.precision(y, yhat)
        rec = turicreate.toolkits.evaluation.recall(y, yhat)
        f1 = turicreate.toolkits.evaluation.f1_score(y, yhat)
        fbeta = turicreate.toolkits.evaluation.fbeta_score(y, yhat, beta=2.0)

        # Assert
        self.assertEqual(pr, 0.0)
        self.assertEqual(rec, None)
        self.assertEqual(f1, 0.0)
        self.assertEqual(fbeta, 0.0)

        # Arrange
        y = turicreate.SArray(["0", "1"])
        yhat = turicreate.SArray(["0", "0"])

        # Act
        pr = turicreate.toolkits.evaluation.precision(y, yhat)
        rec = turicreate.toolkits.evaluation.recall(y, yhat)
        f1 = turicreate.toolkits.evaluation.f1_score(y, yhat)
        fbeta = turicreate.toolkits.evaluation.fbeta_score(y, yhat, beta=2.0)

        # Assert
        self.assertEqual(pr, None)
        self.assertEqual(rec, 0.0)
        self.assertEqual(f1, 0.0)
        self.assertEqual(fbeta, 0.0)

        # Arrange
        y = turicreate.SArray(["0", "0"])
        yhat = turicreate.SArray(["0", "1"])

        # Act
        pr = turicreate.toolkits.evaluation.precision(y, yhat)
        rec = turicreate.toolkits.evaluation.recall(y, yhat)
        f1 = turicreate.toolkits.evaluation.f1_score(y, yhat)
        fbeta = turicreate.toolkits.evaluation.fbeta_score(y, yhat, beta=2.0)

        # Assert
        self.assertEqual(pr, 0.0)
        self.assertEqual(rec, None)
        self.assertEqual(f1, 0.0)
        self.assertEqual(fbeta, 0.0)

    def test_none_prec_recall_scores(self):
        # Arrange
        y = turicreate.SArray([0, 1, 2])
        yhat = turicreate.SArray([0, 0, 2])

        # Act
        avg_cases = ["micro", "macro", None]
        pr = {}
        rec = {}
        f1 = {}
        fbeta = {}
        for avg in avg_cases:
            pr[avg] = turicreate.toolkits.evaluation.precision(y, yhat, avg)
            rec[avg] = turicreate.toolkits.evaluation.recall(y, yhat, avg)
            f1[avg] = turicreate.toolkits.evaluation.f1_score(y, yhat, avg)
            fbeta[avg] = turicreate.toolkits.evaluation.fbeta_score(y, yhat, 2.0, avg)

        # Assert
        self.assertAlmostEqual(pr["micro"], 2.0 / 3)
        self.assertAlmostEqual(rec["micro"], 2.0 / 3)
        self.assertAlmostEqual(f1["micro"], 2.0 / 3)
        self.assertAlmostEqual(fbeta["micro"], 2.0 / 3)

        self.assertAlmostEqual(pr["macro"], 0.75)
        self.assertAlmostEqual(rec["macro"], 2.0 / 3)
        self.assertAlmostEqual(f1["macro"], 0.5555555555555555)
        self.assertAlmostEqual(fbeta["macro"], 0.6111111111111112)

        self.assertEqual(pr[None][1], None)
        self.assertEqual(rec[None][1], 0.0)
        self.assertEqual(f1[None][1], 0.0)
        self.assertEqual(fbeta[None][1], 0.0)

        # Arrange
        y = turicreate.SArray(["0", "1", "2"])
        yhat = turicreate.SArray(["0", "0", "2"])

        # Act
        avg_cases = ["micro", "macro", None]
        pr = {}
        rec = {}
        f1 = {}
        fbeta = {}
        for avg in avg_cases:
            pr[avg] = turicreate.toolkits.evaluation.precision(y, yhat, avg)
            rec[avg] = turicreate.toolkits.evaluation.recall(y, yhat, avg)
            f1[avg] = turicreate.toolkits.evaluation.f1_score(y, yhat, avg)
            fbeta[avg] = turicreate.toolkits.evaluation.fbeta_score(y, yhat, 2.0, avg)

        # Assert
        self.assertAlmostEqual(pr["micro"], 2.0 / 3)
        self.assertAlmostEqual(rec["micro"], 2.0 / 3)
        self.assertAlmostEqual(f1["micro"], 2.0 / 3)
        self.assertAlmostEqual(fbeta["micro"], 2.0 / 3)

        self.assertAlmostEqual(pr["macro"], 0.75)
        self.assertAlmostEqual(rec["macro"], 2.0 / 3)
        self.assertAlmostEqual(f1["macro"], 0.5555555555555555)
        self.assertAlmostEqual(fbeta["macro"], 0.6111111111111112)

        self.assertEqual(pr[None]["1"], None)
        self.assertEqual(rec[None]["1"], 0.0)
        self.assertEqual(f1[None]["1"], 0.0)
        self.assertEqual(fbeta[None]["1"], 0.0)

        # Arrange
        y = turicreate.SArray([0, 0, 2])
        yhat = turicreate.SArray([0, 1, 2])

        # Act
        avg_cases = ["micro", "macro", None]
        pr = {}
        rec = {}
        f1 = {}
        fbeta = {}
        for avg in avg_cases:
            pr[avg] = turicreate.toolkits.evaluation.precision(y, yhat, avg)
            rec[avg] = turicreate.toolkits.evaluation.recall(y, yhat, avg)
            f1[avg] = turicreate.toolkits.evaluation.f1_score(y, yhat, avg)
            fbeta[avg] = turicreate.toolkits.evaluation.fbeta_score(y, yhat, 2.0, avg)

        # Assert
        self.assertAlmostEqual(pr["micro"], 2.0 / 3)
        self.assertAlmostEqual(rec["micro"], 2.0 / 3)
        self.assertAlmostEqual(f1["micro"], 2.0 / 3)
        self.assertAlmostEqual(fbeta["micro"], 2.0 / 3)

        self.assertAlmostEqual(pr["macro"], 2.0 / 3)
        self.assertAlmostEqual(rec["macro"], 0.75)
        self.assertAlmostEqual(f1["macro"], 0.5555555555555555)
        self.assertAlmostEqual(fbeta["macro"], 0.5185185185185185)

        self.assertEqual(pr[None][1], 0.0)
        self.assertEqual(rec[None][1], None)
        self.assertEqual(f1[None][1], 0.0)
        self.assertEqual(fbeta[None][1], 0.0)

        # Arrange
        y = turicreate.SArray(["0", "0", "2"])
        yhat = turicreate.SArray(["0", "1", "2"])

        # Act
        avg_cases = ["micro", "macro", None]
        pr = {}
        rec = {}
        f1 = {}
        fbeta = {}
        for avg in avg_cases:
            pr[avg] = turicreate.toolkits.evaluation.precision(y, yhat, avg)
            rec[avg] = turicreate.toolkits.evaluation.recall(y, yhat, avg)
            f1[avg] = turicreate.toolkits.evaluation.f1_score(y, yhat, avg)
            fbeta[avg] = turicreate.toolkits.evaluation.fbeta_score(y, yhat, 2.0, avg)

        # Assert
        self.assertAlmostEqual(pr["micro"], 2.0 / 3)
        self.assertAlmostEqual(rec["micro"], 2.0 / 3)
        self.assertAlmostEqual(f1["micro"], 2.0 / 3)
        self.assertAlmostEqual(fbeta["micro"], 2.0 / 3)

        self.assertAlmostEqual(pr["macro"], 2.0 / 3)
        self.assertAlmostEqual(rec["macro"], 0.75)
        self.assertAlmostEqual(f1["macro"], 0.5555555555555555)
        self.assertAlmostEqual(fbeta["macro"], 0.5185185185185185)

        self.assertEqual(pr[None]["1"], 0.0)
        self.assertEqual(rec[None]["1"], None)
        self.assertEqual(f1[None]["1"], 0.0)
        self.assertEqual(fbeta[None]["1"], 0.0)

    def test_confusion_matrix(self):
        y = turicreate.SArray([1, 1, 0, 1, 1, 0, 1])
        yhat = turicreate.SArray([0, 1, 0, 0, 1, 1, 0])

        res = turicreate.toolkits.evaluation.confusion_matrix(y, yhat)
        res = res.sort(["predicted_label", "target_label"])["count"]
        self.assertTrue((res == turicreate.SArray([1, 3, 1, 2])).all())

    def test_roc_curve(self):
        # Example from p.864
        # https://ccrma.stanford.edu/workshops/mir2009/references/ROCintro.pdf
        y = turicreate.SArray(
            [1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0]
        )
        yhat = turicreate.SArray(
            [
                0.9,
                0.8,
                0.7,
                0.6,
                0.55,
                0.54,
                0.53,
                0.52,
                0.51,
                0.505,
                0.4,
                0.39,
                0.38,
                0.37,
                0.36,
                0.35,
                0.34,
                0.33,
                0.30,
                0.1,
            ]
        )

        true_fpr = turicreate.SArray(
            [
                0,
                0,
                0,
                0.1,
                0.1,
                0.1,
                0.1,
                0.2,
                0.3,
                0.3,
                0.4,
                0.4,
                0.5,
                0.5,
                0.6,
                0.7,
                0.8,
                0.8,
                0.9,
                0.9,
                1.0,
            ]
        )
        true_tpr = turicreate.SArray(
            [
                0,
                0.1,
                0.2,
                0.2,
                0.3,
                0.4,
                0.5,
                0.5,
                0.5,
                0.6,
                0.6,
                0.7,
                0.7,
                0.8,
                0.8,
                0.8,
                0.8,
                0.9,
                0.9,
                1.0,
                1.0,
            ]
        )

        res = turicreate.toolkits.evaluation.roc_curve(y, yhat)
        points = res[["fpr", "tpr"]].unique().sort(["fpr", "tpr"])
        self.assertTrue(all(points["fpr"] == true_fpr))
        self.assertTrue(all(points["tpr"] == true_tpr))

    def test_roc_curve_str(self):
        y = turicreate.SArray(["a", "b", "a", "b"])
        yhat = turicreate.SArray([0.1, 0.2, 0.3, 0.4])

        res = turicreate.toolkits.evaluation.roc_curve(y, yhat)
        points = res[["fpr", "tpr"]].unique().sort(["fpr", "tpr"])
        self.assertTrue(all(res["tpr"] >= 0) and all(res["tpr"] <= 1))
        self.assertTrue(all(res["fpr"] >= 0) and all(res["fpr"] <= 1))

    def test_grouped_precision_recall(self):
        data = turicreate.SFrame()
        data["user_id"] = ["a", "b", "b", "c", "c", "c"]
        data["item_id"] = ["x", "x", "y", "v", "w", "z"]
        data["rating"] = [0, 1, 2, 3, 4, 5]
        m = turicreate.recommender.item_similarity_recommender.create(data)
        recs = m.recommend()

        test_data = turicreate.SFrame()
        test_data["user_id"] = ["a", "b"]
        test_data["item_id"] = ["v", "z"]
        test_data["rating"] = [7, 8]

        pr = turicreate.recommender.util.precision_recall_by_user(
            test_data, recs, cutoffs=[3]
        )

        self.assertEqual(type(pr), turicreate.SFrame)
        self.assertEqual(
            pr.column_names(), ["user_id", "cutoff", "precision", "recall", "count"]
        )
        self.assertEqual(list(pr["user_id"]), list(turicreate.SArray(["a", "b", "c"])))
        pr = turicreate.recommender.util.precision_recall_by_user(
            test_data, recs, cutoffs=[5, 10, 15]
        )
        self.assertEqual(pr.num_rows(), 9)

    def test_fbeta_binary_score(self):
        # Arrange
        t, p = _generate_classes_and_scores(2, n=100, hard_predictions=True)
        targets = turicreate.SArray(t)
        predictions = turicreate.SArray(p)
        str_targets = targets.astype(str)
        str_predictions = predictions.astype(str)

        # Act
        skl_beta = fbeta_score(
            list(targets), list(predictions), beta=2.0, average="binary"
        )
        beta = turicreate.toolkits.evaluation.fbeta_score(
            targets, predictions, beta=2.0
        )
        str_beta = turicreate.toolkits.evaluation.fbeta_score(
            str_targets, str_predictions, beta=2.0
        )

        # Assert
        self.assertAlmostEqual(skl_beta, beta)
        self.assertAlmostEqual(skl_beta, str_beta)

        # Act
        skl_beta = fbeta_score(list(targets), list(predictions), beta=0.5)
        beta = turicreate.toolkits.evaluation.fbeta_score(
            targets, predictions, beta=0.5
        )
        str_beta = turicreate.toolkits.evaluation.fbeta_score(
            str_targets, str_predictions, beta=0.5
        )

        # Assert
        self.assertAlmostEqual(skl_beta, beta)
        self.assertAlmostEqual(skl_beta, str_beta)

    def test_fbeta_multi_class_score(self):
        # Arrange
        t, p = _generate_classes_and_scores(3, n=100, seed=42, hard_predictions=True)
        targets = turicreate.SArray(t)
        predictions = turicreate.SArray(p)
        str_targets = targets.astype(str)
        str_predictions = predictions.astype(str)

        # Act [ beta = 2]
        skl_beta = fbeta_score(
            list(targets), list(predictions), beta=2.0, average="macro"
        )
        beta = turicreate.toolkits.evaluation.fbeta_score(
            targets, predictions, beta=2.0
        )
        str_beta = turicreate.toolkits.evaluation.fbeta_score(
            str_targets, str_predictions, beta=2.0
        )

        # Assert
        self.assertAlmostEqual(skl_beta, beta)
        self.assertAlmostEqual(skl_beta, str_beta)

        # Act [ beta = 0.5]
        skl_beta = fbeta_score(
            list(targets), list(predictions), beta=0.5, average="micro"
        )
        beta = turicreate.toolkits.evaluation.fbeta_score(
            targets, predictions, beta=0.5, average="micro"
        )
        str_beta = turicreate.toolkits.evaluation.fbeta_score(
            str_targets, str_predictions, beta=0.5, average="micro"
        )

        # Assert
        self.assertAlmostEqual(skl_beta, beta)
        self.assertAlmostEqual(skl_beta, str_beta)

        # Act [Average = 'macro']
        skl_beta = fbeta_score(
            list(targets), list(predictions), beta=2.0, average="macro"
        )
        beta = turicreate.toolkits.evaluation.fbeta_score(
            targets, predictions, beta=2.0, average="macro"
        )
        str_beta = turicreate.toolkits.evaluation.fbeta_score(
            str_targets, str_predictions, beta=2.0, average="macro"
        )

        ## Assert
        self.assertAlmostEqual(skl_beta, beta)
        self.assertAlmostEqual(skl_beta, str_beta)

        ## Act [Average = 'micro']
        skl_beta = fbeta_score(
            list(targets), list(predictions), beta=2.0, average="micro"
        )
        beta = turicreate.toolkits.evaluation.fbeta_score(
            targets, predictions, beta=2.0, average="micro"
        )
        str_beta = turicreate.toolkits.evaluation.fbeta_score(
            str_targets, str_predictions, beta=2.0, average="micro"
        )

        # Assert
        self.assertAlmostEqual(skl_beta, beta)
        self.assertAlmostEqual(skl_beta, str_beta)

        # Act [Average = None]
        skl_beta = fbeta_score(list(targets), list(predictions), beta=2.0, average=None)
        beta = turicreate.toolkits.evaluation.fbeta_score(
            targets, predictions, beta=2.0, average=None
        )
        str_beta = turicreate.toolkits.evaluation.fbeta_score(
            str_targets, str_predictions, beta=2.0, average=None
        )
        # Assert
        self.assertEqual(type(beta), dict)
        self.assertEqual(set(beta.keys()), set([0, 1, 2]))
        self.assertEqual(set(str_beta.keys()), set(["0", "1", "2"]))

        # Note: Explicitly not putting it into a for loop for ease of
        # debugging when the tests fail.
        self.assertAlmostEqual(skl_beta[0], beta[0])
        self.assertAlmostEqual(skl_beta[0], str_beta["0"])
        self.assertAlmostEqual(skl_beta[1], beta[1])
        self.assertAlmostEqual(skl_beta[1], str_beta["1"])
        self.assertAlmostEqual(skl_beta[2], beta[2])
        self.assertAlmostEqual(skl_beta[2], str_beta["2"])

    def test_f1_binary_score(self):

        # Arrange
        t, p = _generate_classes_and_scores(2, n=100, hard_predictions=True)
        targets = turicreate.SArray(t)
        predictions = turicreate.SArray(p)
        str_targets = targets.astype(str)
        str_predictions = predictions.astype(str)

        # Act
        skl_score = f1_score(list(targets), list(predictions))
        score = turicreate.toolkits.evaluation.f1_score(targets, predictions)
        str_score = turicreate.toolkits.evaluation.f1_score(
            str_targets, str_predictions
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

    def test_f1_multi_class_score(self):
        # Arrange
        t, p = _generate_classes_and_scores(3, n=100, hard_predictions=True)
        targets = turicreate.SArray(t)
        predictions = turicreate.SArray(p)
        str_targets = targets.astype(str)
        str_predictions = predictions.astype(str)

        # Act
        skl_score = f1_score(list(targets), list(predictions), average="macro")
        score = turicreate.toolkits.evaluation.f1_score(targets, predictions)
        str_score = turicreate.toolkits.evaluation.f1_score(
            str_targets, str_predictions
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

        # Act [Average = 'macro']
        skl_score = f1_score(list(targets), list(predictions), average="macro")
        score = turicreate.toolkits.evaluation.f1_score(
            targets, predictions, average="macro"
        )
        str_score = turicreate.toolkits.evaluation.f1_score(
            str_targets, str_predictions, average="macro"
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

        # Act [Average = 'micro']
        skl_score = f1_score(list(targets), list(predictions), average="micro")
        score = turicreate.toolkits.evaluation.f1_score(
            targets, predictions, average="micro"
        )
        str_score = turicreate.toolkits.evaluation.f1_score(
            str_targets, str_predictions, average="micro"
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

        # Act [Average = None]
        skl_score = f1_score(list(targets), list(predictions), average=None)
        score = turicreate.toolkits.evaluation.f1_score(
            targets, predictions, average=None
        )
        str_score = turicreate.toolkits.evaluation.f1_score(
            str_targets, str_predictions, average=None
        )
        # Assert
        self.assertEqual(type(score), dict)
        self.assertEqual(set(score.keys()), set([0, 1, 2]))
        self.assertEqual(set(str_score.keys()), set(["0", "1", "2"]))

        # Note: Explicitly not putting it into a for loop for ease of
        # debugging when the tests fail.
        self.assertAlmostEqual(skl_score[0], score[0])
        self.assertAlmostEqual(skl_score[0], str_score["0"])
        self.assertAlmostEqual(skl_score[1], score[1])
        self.assertAlmostEqual(skl_score[1], str_score["1"])
        self.assertAlmostEqual(skl_score[2], score[2])
        self.assertAlmostEqual(skl_score[2], str_score["2"])

    def test_precision_binary_score(self):
        # Arrange
        t, p = _generate_classes_and_scores(2, n=100, hard_predictions=True)
        targets = turicreate.SArray(t)
        predictions = turicreate.SArray(p)
        str_targets = targets.astype(str)
        str_predictions = predictions.astype(str)

        # Act
        skl_score = precision_score(list(targets), list(predictions))
        score = turicreate.toolkits.evaluation.precision(targets, predictions)
        str_score = turicreate.toolkits.evaluation.precision(
            str_targets, str_predictions
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

    def test_precision_multi_class_score(self):
        # Arrange
        t, p = _generate_classes_and_scores(3, n=100, hard_predictions=True)
        targets = turicreate.SArray(t)
        predictions = turicreate.SArray(p)
        str_targets = targets.astype(str)
        str_predictions = predictions.astype(str)

        # Act
        skl_score = precision_score(list(targets), list(predictions), average="macro")
        score = turicreate.toolkits.evaluation.precision(targets, predictions)
        str_score = turicreate.toolkits.evaluation.precision(
            str_targets, str_predictions
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

        # Act [Average = 'macro']
        skl_score = precision_score(list(targets), list(predictions), average="macro")
        score = turicreate.toolkits.evaluation.precision(
            targets, predictions, average="macro"
        )
        str_score = turicreate.toolkits.evaluation.precision(
            str_targets, str_predictions, average="macro"
        )

        ## Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

        ## Act [Average = 'micro']
        skl_score = precision_score(list(targets), list(predictions), average="micro")
        score = turicreate.toolkits.evaluation.precision(
            targets, predictions, average="micro"
        )
        str_score = turicreate.toolkits.evaluation.precision(
            str_targets, str_predictions, average="micro"
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

        # Act [Average = None]
        skl_score = precision_score(list(targets), list(predictions), average=None)
        score = turicreate.toolkits.evaluation.precision(
            targets, predictions, average=None
        )
        str_score = turicreate.toolkits.evaluation.precision(
            str_targets, str_predictions, average=None
        )
        # Assert
        self.assertEqual(type(score), dict)
        self.assertEqual(set(score.keys()), set([0, 1, 2]))
        self.assertEqual(set(str_score.keys()), set(["0", "1", "2"]))

        # Note: Explicitly not putting it into a for loop for ease of
        # debugging when the tests fail.
        self.assertAlmostEqual(skl_score[0], score[0])
        self.assertAlmostEqual(skl_score[0], str_score["0"])
        self.assertAlmostEqual(skl_score[1], score[1])
        self.assertAlmostEqual(skl_score[1], str_score["1"])
        self.assertAlmostEqual(skl_score[2], score[2])
        self.assertAlmostEqual(skl_score[2], str_score["2"])

    def test_recall_binary_score(self):
        # Arrange
        t, p = _generate_classes_and_scores(2, n=100, hard_predictions=True)
        targets = turicreate.SArray(t)
        predictions = turicreate.SArray(p)
        str_targets = targets.astype(str)
        str_predictions = predictions.astype(str)

        # Act
        skl_score = recall_score(list(targets), list(predictions))
        score = turicreate.toolkits.evaluation.recall(targets, predictions)
        str_score = turicreate.toolkits.evaluation.recall(str_targets, str_predictions)

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

    def test_recall_multi_class_score(self):
        # Arrange
        t, p = _generate_classes_and_scores(3, n=100, hard_predictions=True)
        targets = turicreate.SArray(t)
        predictions = turicreate.SArray(p)
        str_targets = targets.astype(str)
        str_predictions = predictions.astype(str)

        # Act
        skl_score = recall_score(list(targets), list(predictions), average="macro")
        score = turicreate.toolkits.evaluation.recall(targets, predictions)
        str_score = turicreate.toolkits.evaluation.recall(str_targets, str_predictions)

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

        # Act [Average = 'macro']
        skl_score = recall_score(list(targets), list(predictions), average="macro")
        score = turicreate.toolkits.evaluation.recall(
            targets, predictions, average="macro"
        )
        str_score = turicreate.toolkits.evaluation.recall(
            str_targets, str_predictions, average="macro"
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

        # Act [Average = 'micro']
        skl_score = recall_score(list(targets), list(predictions), average="micro")
        score = turicreate.toolkits.evaluation.recall(
            targets, predictions, average="micro"
        )
        str_score = turicreate.toolkits.evaluation.recall(
            str_targets, str_predictions, average="micro"
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

        # Act [Average = None]
        skl_score = recall_score(list(targets), list(predictions), average=None)
        score = turicreate.toolkits.evaluation.recall(
            targets, predictions, average=None
        )
        str_score = turicreate.toolkits.evaluation.recall(
            str_targets, str_predictions, average=None
        )
        # Assert
        self.assertEqual(type(score), dict)
        self.assertEqual(set(score.keys()), set([0, 1, 2]))
        self.assertEqual(set(str_score.keys()), set(["0", "1", "2"]))

        # Note: Explicitly not putting it into a for loop for ease of
        # debugging when the tests fail.
        self.assertAlmostEqual(skl_score[0], score[0])
        self.assertAlmostEqual(skl_score[0], str_score["0"])
        self.assertAlmostEqual(skl_score[1], score[1])
        self.assertAlmostEqual(skl_score[1], str_score["1"])
        self.assertAlmostEqual(skl_score[2], score[2])
        self.assertAlmostEqual(skl_score[2], str_score["2"])

    def test_accuracy_binary_score(self):
        # Arrange
        t, p = _generate_classes_and_scores(2, n=100, hard_predictions=True)
        targets = turicreate.SArray(t)
        predictions = turicreate.SArray(p)
        str_targets = targets.astype(str)
        str_predictions = predictions.astype(str)

        # Act
        skl_score = accuracy_score(list(targets), list(predictions))
        score = turicreate.toolkits.evaluation.accuracy(targets, predictions)
        str_score = turicreate.toolkits.evaluation.accuracy(
            str_targets, str_predictions
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

    def test_accuracy_multi_class_score(self):
        # Arrange
        t, p = _generate_classes_and_scores(3, n=100, hard_predictions=True)
        targets = turicreate.SArray(t)
        predictions = turicreate.SArray(p)
        str_targets = targets.astype(str)
        str_predictions = predictions.astype(str)

        # Act
        skl_score = accuracy_score(list(targets), list(predictions))
        score = turicreate.toolkits.evaluation.accuracy(targets, predictions)
        str_score = turicreate.toolkits.evaluation.accuracy(
            str_targets, str_predictions
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

        # Act [Average = 'macro']
        cls_skl_score = {}
        for i in range(3):
            t = map(lambda x: x == i, list(targets))
            p = map(lambda x: x == i, list(predictions))
            cls_skl_score[i] = accuracy_score(list(t), list(p))

        macro_avg = sum(cls_skl_score.values()) * 1.0 / 3
        score = turicreate.toolkits.evaluation.accuracy(
            targets, predictions, average="macro"
        )
        str_score = turicreate.toolkits.evaluation.accuracy(
            str_targets, str_predictions, average="macro"
        )

        ## Assert
        self.assertAlmostEqual(macro_avg, score)
        self.assertAlmostEqual(macro_avg, str_score)

        ## Act [Average = 'micro']
        skl_score = accuracy_score(list(targets), list(predictions))
        score = turicreate.toolkits.evaluation.accuracy(
            targets, predictions, average="micro"
        )
        str_score = turicreate.toolkits.evaluation.accuracy(
            str_targets, str_predictions, average="micro"
        )

        # Assert
        self.assertAlmostEqual(skl_score, score)
        self.assertAlmostEqual(skl_score, str_score)

        # Act [Average = None]
        prec_score = precision_score(list(targets), list(predictions), average=None)
        score = turicreate.toolkits.evaluation.accuracy(
            targets, predictions, average=None
        )
        str_score = turicreate.toolkits.evaluation.accuracy(
            str_targets, str_predictions, average=None
        )
        # Assert
        self.assertEqual(type(score), dict)
        self.assertEqual(set(score.keys()), set([0, 1, 2]))
        self.assertEqual(set(str_score.keys()), set(["0", "1", "2"]))

        # Note: Explicitly not putting it into a for loop for ease of
        # debugging when the tests fail.
        self.assertAlmostEqual(prec_score[0], score[0])
        self.assertAlmostEqual(prec_score[0], str_score["0"])
        self.assertAlmostEqual(prec_score[1], score[1])
        self.assertAlmostEqual(prec_score[1], str_score["1"])
        self.assertAlmostEqual(prec_score[2], score[2])
        self.assertAlmostEqual(prec_score[2], str_score["2"])

    def test_missing_values(self):
        # Arrange
        t, p = _generate_classes_and_scores(3, n=100, hard_predictions=True)
        pm = [None if x == 2 else x for x in p]
        tm = [None if x == 2 else x for x in t]

        targets = turicreate.SArray(tm)
        predictions = turicreate.SArray(pm)

        # Act & Assert [accuracy]
        skl_score = accuracy_score(t, p)
        score = turicreate.toolkits.evaluation.accuracy(targets, predictions)
        self.assertAlmostEqual(skl_score, score)

        # Act & Assert [precision]
        skl_score = precision_score(t, p, average="macro")
        score = turicreate.toolkits.evaluation.precision(targets, predictions)
        self.assertAlmostEqual(skl_score, score)

        # Act & Assert [recall]
        skl_score = recall_score(t, p, average="macro")
        score = turicreate.toolkits.evaluation.recall(targets, predictions)
        self.assertAlmostEqual(skl_score, score)

        # Act & Assert [f1_score]
        skl_score = f1_score(t, p, average="macro")
        score = turicreate.toolkits.evaluation.f1_score(targets, predictions)
        self.assertAlmostEqual(skl_score, score)

        # Act & Assert [fbeta_score]
        skl_score = fbeta_score(t, p, beta=2.0, average="macro")
        score = turicreate.toolkits.evaluation.fbeta_score(
            targets, predictions, beta=2.0
        )
        self.assertAlmostEqual(skl_score, score)

    def test_auc_basic(self):
        # Arrange
        # Example from p.864
        # https://ccrma.stanford.edu/workshops/mir2009/references/ROCintro.pdf
        y = turicreate.SArray(
            [1, 1, 0, 1, 1, 1, 0, 0, 1, 0, 1, 0, 1, 0, 0, 0, 1, 0, 1, 0]
        )
        yhat = turicreate.SArray(
            [
                0.9,
                0.8,
                0.7,
                0.6,
                0.55,
                0.54,
                0.53,
                0.52,
                0.51,
                0.505,
                0.4,
                0.39,
                0.38,
                0.37,
                0.36,
                0.35,
                0.34,
                0.33,
                0.30,
                0.1,
            ]
        )

        scikit_auc = roc_auc_score(list(y), list(yhat))

        # Act
        glc_int_auc = turicreate.toolkits.evaluation.auc(y, yhat)
        glc_str_auc = turicreate.toolkits.evaluation.auc(y.astype(str), yhat)

        # Assert
        self.assertAlmostEqual(glc_int_auc, scikit_auc)
        self.assertAlmostEqual(glc_str_auc, scikit_auc)

    def test_auc_binary(self):
        # Arrange
        rs = np.random.RandomState(42)
        n = 100
        y = turicreate.SArray(rs.randint(2, size=n))
        yhat = turicreate.SArray(rs.uniform(size=n))

        # Act & Assert
        scikit_auc = roc_auc_score(list(y), list(yhat), average="macro")
        glc_int_auc = turicreate.toolkits.evaluation.auc(y, yhat, "macro")
        glc_str_auc = turicreate.toolkits.evaluation.auc(y.astype(str), yhat, "macro")
        self.assertAlmostEqual(glc_int_auc, scikit_auc)
        self.assertAlmostEqual(glc_str_auc, scikit_auc)

        # Act & Assert
        scikit_auc = roc_auc_score(list(y), list(yhat), average=None)
        glc_int_auc = turicreate.toolkits.evaluation.auc(y, yhat, average=None)
        glc_str_auc = turicreate.toolkits.evaluation.auc(
            y.astype(str), yhat, average=None
        )
        self.assertAlmostEqual(glc_int_auc, scikit_auc)
        self.assertAlmostEqual(glc_str_auc, scikit_auc)

    def test_auc_multi_class_score(self):
        # Arrange
        t, p = _generate_classes_and_scores(3, n=100, hard_predictions=False)
        sk_p = {}
        sk_t = {}
        for i in range(3):
            sk_p[i] = p[:, i]
            sk_t[i] = t == i
        targets = turicreate.SArray(t)
        predictions = turicreate.SArray(p)
        str_targets = targets.astype(str)

        # Act
        sk_score = {}
        for i in range(3):
            sk_score[i] = roc_auc_score(sk_t[i], sk_p[i])

        # Act [Average = None]
        score = turicreate.toolkits.evaluation.auc(targets, predictions, average=None)
        str_score = turicreate.toolkits.evaluation.auc(
            str_targets, predictions, average=None
        )
        # Assert
        self.assertEqual(type(score), dict)
        self.assertEqual(set(score.keys()), set([0, 1, 2]))
        self.assertEqual(set(str_score.keys()), set(["0", "1", "2"]))

        # Note: Explicitly not putting it into a for loop for ease of
        # debugging when the tests fail.
        self.assertAlmostEqual(sk_score[0], score[0])
        self.assertAlmostEqual(sk_score[0], str_score["0"])
        self.assertAlmostEqual(sk_score[1], score[1])
        self.assertAlmostEqual(sk_score[1], str_score["1"])
        self.assertAlmostEqual(sk_score[2], score[2])
        self.assertAlmostEqual(sk_score[2], str_score["2"])

        # Act [Average = 'macro']
        score = turicreate.toolkits.evaluation.auc(
            targets, predictions, average="macro"
        )
        str_score = turicreate.toolkits.evaluation.auc(
            str_targets, predictions, average="macro"
        )
        avg_score = 0.0
        for i in range(3):
            avg_score += sk_score[i]
        avg_score /= 3.0
        self.assertAlmostEqual(avg_score, score)
        self.assertAlmostEqual(avg_score, str_score)

    def test_bogus_input_prob_evaluators(self):
        # Arrange (mismatch number of classes)
        t, _ = _generate_classes_and_scores(3, n=100, hard_predictions=False)
        _, p = _generate_classes_and_scores(4, n=100, hard_predictions=False)
        targets = turicreate.SArray(list(t))
        predictions = turicreate.SArray(list(p))
        float_predictions = predictions.apply(lambda x: x[0])

        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.auc(targets, predictions)
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.roc_curve(targets, predictions)
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.log_loss(targets, predictions)

        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.auc(targets, float_predictions)
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.roc_curve(targets, float_predictions)
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.log_loss(targets, float_predictions)

        bad_range_targets = turicreate.SArray([0, 1, 0, 1])
        bad_range_predictions = turicreate.SArray([1, 2, 3, 4])
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.log_loss(
                bad_range_targets, bad_range_predictions
            )

        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.roc_curve(
                bad_range_targets, bad_range_predictions
            )

        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.auc(
                bad_range_targets, bad_range_predictions
            )

        bad_range_targets = turicreate.SArray([0, 1, 0, 1])
        bad_range_predictions = turicreate.SArray(
            [[1.0, 2.0], [2.0, 3.0], [3.0, 4.0], [4.0, 5.0]]
        )
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.log_loss(
                bad_range_targets, bad_range_predictions
            )

        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.roc_curve(
                bad_range_targets, bad_range_predictions
            )

        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.auc(
                bad_range_targets, bad_range_predictions
            )

        targets = turicreate.SArray([0, 1, 2, 0])
        predictions = turicreate.SArray(
            [
                [0.9, 0.0, 0.1, 0.0],
                [0.8, 0.1, 0.1, 0.0],
                [0.1, 0.1, 0.8, 0.0],
                [0.1, 0.1, 0.8, 0.0],
            ]
        )
        good_index_map = {i: i for i in range(4)}
        incomplete_index_map = {i: i for i in range(3)}
        invalid_range_index_map = {0: 1, 1: 2, 2: 3, 3: 4}
        non_injective_index_map = {0: 0, 1: 0, 2: 2, 3: 3}

        # No exception with a correct index map
        score = turicreate.toolkits.evaluation.auc(
            targets, predictions, index_map=good_index_map
        )
        score = turicreate.toolkits.evaluation.roc_curve(
            targets, predictions, index_map=good_index_map
        )
        score = turicreate.toolkits.evaluation.log_loss(
            targets, predictions, index_map=good_index_map
        )

        # Exception if index_map size does not match prediction vector size
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.auc(
                targets, predictions, index_map=incomplete_index_map
            )
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.roc_curve(
                targets, predictions, index_map=incomplete_index_map
            )
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.log_loss(
                targets, predictions, index_map=incomplete_index_map
            )

        # Exception if index_map values do not span prediction vector indices
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.auc(
                targets, predictions, index_map=invalid_range_index_map
            )
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.roc_curve(
                targets, predictions, index_map=invalid_range_index_map
            )
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.log_loss(
                targets, predictions, index_map=invalid_range_index_map
            )

        # Exception if index_map uses the same index for two labels
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.auc(
                targets, predictions, index_map=non_injective_index_map
            )
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.roc_curve(
                targets, predictions, index_map=non_injective_index_map
            )
        with self.assertRaises(ToolkitError):
            score = turicreate.toolkits.evaluation.log_loss(
                targets, predictions, index_map=non_injective_index_map
            )
