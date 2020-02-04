# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import os
import tempfile
import shutil
import unittest
import turicreate
from turicreate.toolkits._main import ToolkitError
from turicreate.toolkits.topic_model import topic_model
from turicreate.toolkits.text_analytics import parse_sparse
from turicreate.toolkits.text_analytics import parse_docword
from turicreate.toolkits.topic_model import perplexity
from . import util as test_util
import time
import random
import numpy as np
import itertools as _itertools

DELTA = 0.0000001
examples = {}


def generate_bar_example(
    num_topics=10, num_documents=500, num_words_per_doc=100, alpha=1, beta=1, seed=None
):
    """
    Generate the classic "bars" example, a synthetic data set of small
    black 5x5 pixel images with a single white bar that is either horizontal
    or vertical.

    See Steyvers' MATLAB Topic Modeling Toolbox,
    http://psiexp.ss.uci.edu/research/programs_data/exampleimages1.html,

    and the original paper:
    Griffiths, T., & Steyvers, M. (2004). Finding Scientific Topics.
    Proceedings of the National Academy of Sciences, 101 (suppl. 1), 5228-5235.

    Returns
    -------
    out : SArray
        Each element represents a 'document' where the words are strings that
        represent a single pixel in the image in a colon-separated format.
        For example, 'horizontal_location:vertical_location'. Each word is
        associated with a count of the number of generated occurrences.
    """

    width = 5

    vocab_size = width * width
    rng = random.Random()
    if seed is not None:
        rng.seed(seed)

    zeros = [[0 for i in range(width)] for j in range(width)]
    topic_squares = [zeros for i in range(num_topics)]
    for i in range(width):
        for j in range(width):
            topic_squares[i][i][j] = 1.0 / width
    for i in range(width):
        for j in range(width):
            topic_squares[width + i][j][i] = 1.0 / width
    topics = []
    for k in range(num_topics):
        topics.append(list(_itertools.chain(*topic_squares[k])))

    def weighted_choice(probs):
        total = sum(probs)
        r = rng.uniform(0, total)
        upto = 0
        for i, w in enumerate(probs):
            if upto + w > r:
                return i
            upto += w
        assert False, "Shouldn't get here"

    documents = []
    thetas = []
    for d in range(num_documents):
        doc = [0 for i in range(width * width)]
        topic_dist = [rng.gammavariate(1, 1) for k in range(num_topics)]
        topic_dist = [z / sum(topic_dist) for z in topic_dist]
        for i in range(num_words_per_doc):
            k = weighted_choice(topic_dist)
            w = weighted_choice(topics[k])
            doc[w] += 1
        thetas.append(topic_dist)
        documents.append(doc)

    sparse_documents = []
    for d in documents:
        sd = {}
        for i in range(width):
            for j in range(width):
                k = str(i) + "," + str(j)
                sd[k] = d[i * width + j]
        sparse_documents.append(sd)
    bow_documents = turicreate.SArray(sparse_documents)
    return bow_documents


class BasicTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):

        # Create an example containing synthetic data along
        # with fitted models (with default parameters).
        docs = generate_bar_example(num_documents=1000, seed=12345)
        models = []

        # Test a model that used CGS
        m = topic_model.create(docs, num_topics=10)
        models.append(m)

        # Test a model with many topics
        m = topic_model.create(docs, method="cgs", num_topics=100, num_iterations=2)
        models.append(m)
        m = topic_model.create(docs, method="alias", num_topics=100, num_iterations=2)
        models.append(m)

        # Test a model serialized after using CGS
        with test_util.TempDirectory() as f:
            m.save(f)
            m2 = turicreate.load_model(f)

        models.append(m2)

        # Save
        examples["synthetic"] = {"docs": docs, "models": models}
        self.docs = examples["synthetic"]["docs"]
        self.models = examples["synthetic"]["models"]

    def test_set_burnin(self):
        m = topic_model.create(self.docs, num_burnin=25, num_iterations=1)
        self.assertTrue(m.num_burnin == 25)

    def test_no_validation_print(self):
        m = topic_model.create(
            self.docs, num_burnin=25, num_iterations=2, print_interval=0
        )
        self.assertTrue(m is not None)
        self.assertEqual(m.num_burnin, 25)

    def test_validation_set(self):
        m = topic_model.create(self.docs, validation_set=self.docs)
        self.assertTrue("validation_perplexity" in m._list_fields())

        # Test that an SFrame can be used
        sf = turicreate.SFrame({"text": self.docs})
        m = topic_model.create(self.docs, validation_set=sf)
        self.assertTrue("validation_perplexity" in m._list_fields())

    def test_set_associations(self):
        associations = turicreate.SFrame()
        associations["word"] = ["1,1", "1,2", "1,3"]
        associations["topic"] = [0, 0, 0]
        m = topic_model.create(self.docs, associations=associations)

        # In this context, the "words" '1,1', '1,2', '1,3' should be
        # the first three words in the vocabulary.
        self.assertEqual(list(m.topics["vocabulary"].head(3)), ["1,1", "1,2", "1,3"])

        # For each of these words, the probability of topic 0 should
        # be largest.
        probs = m.topics["topic_probabilities"]
        largest = probs.apply(lambda x: np.argmax(x))
        self.assertEqual(list(largest.head(3)), [0, 0, 0])

    def test_model_runs(self):
        """
        Test that the model runs and returns the proper type of object.
        """

        for m in self.models:
            self.assertTrue(m is not None)
            self.assertTrue(isinstance(m, topic_model.TopicModel))

    def test_get_topics(self):
        """
        Test that we can retrieve the topic probabilities from the model.
        """

        for m in self.models:

            topics = m.topics
            self.assertTrue(isinstance(topics, turicreate.SFrame))
            self.assertEqual(topics.num_rows(), 25)
            self.assertEqual(topics.num_columns(), 2)
            z = m.topics["topic_probabilities"]
            for k in range(m.num_topics):
                self.assertTrue(
                    abs(sum(z.vector_slice(k)) - 1) < DELTA,
                    "Returned probabilities do not sum to 1.",
                )

            # Make sure returned object is an SFrame of the right size
            topics = m.get_topics()
            self.assertTrue(isinstance(topics, turicreate.SFrame))
            self.assertTrue(
                topics.num_columns() == 3,
                "Returned SFrame should have a topic, word, and probs.",
            )

            # Make sure that requesting a single topic returns only that topic
            num_words = 8
            topics = m.get_topics([5], num_words=num_words)
            self.assertTrue(
                all(topics["topic"] == 5), "Returned topics do not have the right id."
            )
            self.assertEqual(topics.num_rows(), num_words)
            topics = m.get_topics([2, 4], num_words=num_words)
            self.assertEqual(set(list(topics["topic"])), set([2, 4]))
            self.assertEqual(topics.num_rows(), num_words + num_words)

            # Make sure the cumulative probability of the returned words is
            # is less than the cutoff we provided.
            # A cutoff of 1.0 should return num_words for every topic.
            cutoff = 1.0
            topics = m.get_topics(cdf_cutoff=cutoff, num_words=len(m.vocabulary))
            totals = topics.groupby(
                "topic", {"total_score": turicreate.aggregate.SUM("score")}
            )
            self.assertTrue(
                all(totals["total_score"] <= (cutoff + DELTA)),
                "More words were returned than expected for this cutoff.",
            )

            # Make sure we raise errors for bad input
            with self.assertRaises(ValueError):
                m.get_topics([-1])
            with self.assertRaises(ValueError):
                m.get_topics([10000])
            with self.assertRaises(ToolkitError):
                topics = m.get_topics(output_type="other")

            # Test getting topic_words
            topic_words = m.get_topics(output_type="topic_words", num_words=5)
            self.assertEqual(type(topic_words), turicreate.SFrame)

            # Test words are sorted correctly for the first topic
            # TODO: Make this more deterministic.

            # topic_probs = m.get_topics(num_words=5)
            # expected = [w for w in topic_probs['word'][:5]]
            # observed = topic_words['words'][0]
            # self.assertEqual(observed[0], expected[0])

    def test_get_vocabulary(self):
        """
        Test that we can retrieve the vocabulary from the model.
        """

        for m in self.models:
            vocab = m.vocabulary
            self.assertTrue(isinstance(vocab, turicreate.SArray))
            self.assertEqual(len(vocab), 25)

    def test_predict(self):
        """
        Test that we can make predictions using the model.
        """

        docs = self.docs
        for m in self.models:
            preds = m.predict(docs)
            self.assertTrue(isinstance(preds, turicreate.SArray))
            self.assertEqual(len(preds), len(docs))
            self.assertEqual(preds.dtype, int)

            preds = m.predict(docs, output_type="probability")
            self.assertTrue(isinstance(preds, turicreate.SArray))
            self.assertTrue(len(preds) == len(docs))
            s = preds.apply(lambda x: sum(x))
            self.assertTrue((s.apply(lambda x: abs(x - 1)) < 0.000001).all())

            # Test predictions when docs have new words
            new_docs = turicreate.SArray([{"-1,-1": 3.0, "0,4": 5.0, "0,3": 2.0}])
            preds = m.predict(new_docs)
            self.assertEqual(len(preds), len(new_docs))

            # Test additional burnin. Ideally we could show that things
            # converge as you increase burnin.
            preds_no_burnin = m.predict(docs, output_type="probability", num_burnin=0)
            self.assertEqual(len(preds_no_burnin), len(docs))

    def test_save_load(self):

        for i, m in enumerate(self.models):

            with test_util.TempDirectory() as f:
                m.save(f)

                m2 = turicreate.load_model(f)
                self.assertTrue(m2 is not None)
                self.assertEqual(m.__str__(), m2.__str__())

                diff = (
                    m.topics["topic_probabilities"] - m2.topics["topic_probabilities"]
                )
                zeros = diff * 0

                for i in range(len(zeros)):
                    observed = np.array(diff[i])
                    expected = np.array(zeros[i])
                    np.testing.assert_array_almost_equal(observed, expected)

                topics = m2.get_topics()
                self.assertEqual(topics.num_columns(), 3)

    def test_initialize(self):
        """
        The initial_topics argument allows one to fit a model from a
        particular set of parameters.
        """

        for m in self.models:
            start_docs = turicreate.SArray(self.docs.tail(3))
            m = topic_model.create(
                start_docs,
                num_topics=20,
                method="cgs",
                alpha=0.1,
                beta=0.01,
                num_iterations=1,
                print_interval=1,
            )
            start_topics = turicreate.SFrame(m.topics.head(100))
            m2 = topic_model.create(
                self.docs,
                num_topics=20,
                initial_topics=start_topics,
                method="cgs",
                alpha=0.1,
                beta=0.01,
                num_iterations=0,
                print_interval=1,
            )

            # Check that the vocabulary of the new model is the same as
            # the one we used to initialize the model.
            self.assertTrue(
                (start_topics["vocabulary"] == m2.topics["vocabulary"]).all()
            )

            # Check that the previously most probable word is still the most
            # probable after 0 iterations, i.e. just initialization.
            old_prob = start_topics["topic_probabilities"].vector_slice(0)
            new_prob = m2.topics["topic_probabilities"].vector_slice(0)
            self.assertTrue(np.argmax(list(old_prob)) == np.argmax(list(new_prob)))

    def test_exceptions(self):

        good1 = turicreate.SArray([{"a": 5, "b": 7}])
        good2 = turicreate.SFrame({"bow": good1})
        good3 = turicreate.SArray([{}])
        bad1 = turicreate.SFrame({"x": [0, 1, 2, 3]})
        bad2 = turicreate.SFrame({"x": [{"0": 3}], "y": [{"3": 5}]})
        bad3 = turicreate.SArray([{"a": 5, "b": 3}, None, {"a": 10}])
        bad4 = turicreate.SArray([{"a": 5, "b": None}, {"a": 3}])

        for d in [good1, good2, good3]:
            m = topic_model.create(d)
            self.assertTrue(m is not None)

        # Test that create() throws on bad input
        with self.assertRaises(Exception):
            m = topic_model.create(bad1)
        with self.assertRaises(Exception):
            m = topic_model.create(bad2)
        with self.assertRaises(ToolkitError):
            m = topic_model.create(bad3)
        with self.assertRaises(ToolkitError):
            m = topic_model.create(bad4)

        m = self.models[0]
        with self.assertRaises(Exception):
            pr = m.predict(bad1)
        with self.assertRaises(Exception):
            pr = m.predict(bad2)
        with self.assertRaises(Exception):
            pr = m.predict(bad3)

    def test_evaluate(self):

        for m in self.models:
            # Check evaluate on the training set returns an answer
            perp = m.evaluate(self.docs)
            self.assertTrue(isinstance(perp, dict))
            self.assertTrue(isinstance(perp["perplexity"], float))

            # Check that the answer is within 5% of the one reported
            # by the model during training.
            if "validation_perplexity" in m._list_fields():
                perp2 = m.validation_perplexity
            # TEMP: Disable since we now only compute this if validatino set is provided
            # assert abs(perp - perp2)/perp < .05

            # Check evaluate on the test set returns an answer
            perp = m.evaluate(self.docs, self.docs)
            self.assertTrue(isinstance(perp, dict))

    def test__training_stats(self):
        expected_fields = ["training_iterations", "training_time"]
        for m in self.models:
            actual_fields = m._training_stats()
            for f in expected_fields:
                self.assertTrue(f in actual_fields)
                self.assertTrue(m._get(f) is not None)

    def test_summary(self):

        expected_fields = [
            "num_topics",  # model parameters
            "alpha",
            "beta",
            "topics",
            "vocabulary",
            "num_iterations",  # stats and options
            "print_interval",
            "training_time",
            "training_iterations",
            "num_burnin",
        ]

        for m in self.models:
            actual_fields = m._list_fields()
            for f in expected_fields:
                self.assertTrue(f in actual_fields)
                self.assertTrue(m._get(f) is not None)


class ParsersTest(unittest.TestCase):
    def setUp(self):
        self.tmpfile_a = tempfile.NamedTemporaryFile(delete=False).name
        with open(self.tmpfile_a, "w") as o:
            o.write("3 1:5 2:10 5:353\n")
            o.write("0 0:7 6:3 3:100")

        self.tmpfile_vocab = tempfile.NamedTemporaryFile(delete=False).name
        with open(self.tmpfile_vocab, "w") as o:
            o.write("\n".join(["a", "b", "c", "d", "e", "f", "g"]))

        self.tmpfile_b = tempfile.NamedTemporaryFile(delete=False).name
        with open(self.tmpfile_b, "w") as o:
            o.write("2\n5\n6\n")
            o.write("1 2 5\n")
            o.write("1 3 10\n")
            o.write("1 6 353\n")
            o.write("2 1 7\n")
            o.write("2 7 3\n")
            o.write("2 4 100")

    def test_parse_sparse(self):
        d = parse_sparse(self.tmpfile_a, self.tmpfile_vocab)
        self.assertTrue(d[0] == {"b": 5, "c": 10, "f": 353})
        self.assertTrue(d[1] == {"a": 7, "g": 3, "d": 100})

    def test_parse_docword(self):
        d = parse_docword(self.tmpfile_b, self.tmpfile_vocab)
        self.assertTrue(d[0] == {"b": 5, "c": 10, "f": 353})
        self.assertTrue(d[1] == {"a": 7, "g": 3, "d": 100})

    def tearDown(self):
        os.remove(self.tmpfile_a)
        os.remove(self.tmpfile_b)
        os.remove(self.tmpfile_vocab)


class UtilitiesTest(unittest.TestCase):
    def setUp(self):

        docs = turicreate.SArray([{"b": 5, "a": 3}, {"c": 7, "b": 5}, {"a": 2, "d": 3}])

        doc_topics = turicreate.SArray([[0.9, 0.1], [0.7, 0.3], [0.1, 0.9]])

        word_topics = turicreate.SArray([[0.5, 0.5], [0.1, 0.9], [0.25, 0.75]])

        vocabulary = turicreate.SArray(["a", "b", "c"])

        self.docs = docs
        self.word_topics = word_topics
        self.doc_topics = doc_topics
        self.vocabulary = vocabulary
        self.num_topics = 2

    def test_perplexity(self):

        prob_0_a = 0.9 * 0.5 + 0.1 * 0.5
        prob_0_b = 0.9 * 0.1 + 0.1 * 0.9
        prob_1_b = 0.7 * 0.1 + 0.3 * 0.9
        prob_1_c = 0.7 * 0.25 + 0.3 * 0.75
        prob_2_a = 0.1 * 0.5 + 0.9 * 0.5
        prob_2_d = 0

        perp = 0.0
        perp += 3 * np.log(prob_0_a) + 5 * np.log(prob_0_b)
        perp += 5 * np.log(prob_1_b) + 7 * np.log(prob_1_c)
        perp += 2 * np.log(prob_2_a)
        perp = np.exp(-perp / (3 + 5 + 5 + 7 + 2))

        observed_perp = perplexity(
            self.docs, self.doc_topics, self.word_topics, self.vocabulary
        )

        self.assertAlmostEqual(perp, observed_perp, delta=0.0001)
