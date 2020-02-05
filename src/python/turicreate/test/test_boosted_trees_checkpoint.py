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
import os
import shutil


class BoostedTreesRegressionCheckpointTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        sf = tc.SFrame(
            {
                "cat[1]": ["1", "1", "2", "2", "2"] * 20,
                "cat[2]": ["1", "3", "3", "1", "1"] * 20,
                "target": [random.random() for i in range(100)],
            }
        )
        cls.train, cls.test = sf.random_split(0.5, seed=5)
        cls.model = tc.boosted_trees_regression
        cls.metrics = ["rmse", "max_error"]
        return cls

    def setUp(self):
        self.checkpoint_dir = tempfile.mkdtemp()

    def tearDown(self):
        shutil.rmtree(self.checkpoint_dir)

    def test_default_checkpoint_interval(self):
        max_iterations = 20
        default_interval = 5

        # Train 20 iterations, and checkpoint every 5
        m = self.model.create(
            self.train,
            "target",
            validation_set=self.test,
            max_depth=2,
            random_seed=1,
            max_iterations=max_iterations,
            model_checkpoint_path=self.checkpoint_dir,
        )

        # Resume training checkpoint from iterations 5, 10, 15, ...
        for i in range(default_interval, max_iterations, default_interval):
            checkpoint = os.path.join(self.checkpoint_dir, "model_checkpoint_%d" % i)
            m_resume = self.model.create(
                self.train,
                "target",
                validation_set=self.test,
                resume_from_checkpoint=checkpoint,
            )
            # Check the progress is the same as the reference model
            for col in m.progress.column_names():
                if col != "Elapsed Time":
                    self.assertListEqual(
                        list(m.progress[col]), list(m_resume.progress[col])
                    )

    def test_non_default_checkpoint_interval(self):
        max_iterations = 5
        default_interval = 2
        # Train 5 iterations, and checkpoint every 2
        m = self.model.create(
            self.train,
            "target",
            validation_set=self.test,
            max_depth=2,
            random_seed=1,
            max_iterations=max_iterations,
            model_checkpoint_path=self.checkpoint_dir,
            model_checkpoint_interval=default_interval,
        )
        # Resume training checkpoint from iterations 2, 4
        for i in range(default_interval, max_iterations, default_interval):
            checkpoint = os.path.join(self.checkpoint_dir, "model_checkpoint_%d" % i)
            m_resume = self.model.create(
                self.train,
                "target",
                validation_set=self.test,
                resume_from_checkpoint=checkpoint,
            )
            # Check the progress is the same as the reference model
            for col in m.progress.column_names():
                if col != "Elapsed Time":
                    self.assertListEqual(
                        list(m.progress[col]), list(m_resume.progress[col])
                    )

    def test_restore_with_different_data(self):
        max_iterations = 20
        default_interval = 5
        # Train 20 iterations, and checkpoint every 5
        m = self.model.create(
            self.train,
            "target",
            validation_set=self.test,
            max_depth=2,
            random_seed=1,
            max_iterations=max_iterations,
            model_checkpoint_path=self.checkpoint_dir,
        )

        # Resume training checkpoint from iterations 5, 10, 15, ... using "self.test"
        for i in range(default_interval, max_iterations, default_interval):
            checkpoint = os.path.join(self.checkpoint_dir, "model_checkpoint_%d" % i)
            m_resume = self.model.create(
                self.test,
                "target",
                validation_set=self.test,
                resume_from_checkpoint=checkpoint,
            )


class BoostedTreesClassifierCheckpointTest(BoostedTreesRegressionCheckpointTest):
    @classmethod
    def setUpClass(cls):
        sf = tc.SFrame(
            {
                "cat[1]": ["1", "1", "2", "2", "2"] * 20,
                "cat[2]": ["1", "3", "3", "1", "1"] * 20,
                "target": [0, 1] * 50,
            }
        )
        cls.train, cls.test = sf.random_split(0.5, seed=5)
        cls.model = tc.boosted_trees_classifier
        return cls
