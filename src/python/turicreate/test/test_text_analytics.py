# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import math
import unittest
import warnings
import turicreate as tc
from . import util


class FeatureEngineeringTest(unittest.TestCase):
    """
    Test the text utilities for creating and cleaning features.
    """

    @classmethod
    def setUpClass(self):
        self.sa_word = tc.SArray(
            ["I like big dogs. They are fun. I LIKE BIG DOGS", "I like.", "I like big"]
        )
        self.sa_char = tc.SArray(["Fun. is. fun", "Fun is fun.", "fu", "fun"])

        self.languages = tc.SArray(
            ["This is someurl http://someurl!!", "中文 应该也 行", "Сблъсъкът между"]
        )

        self.languages_double = tc.SArray(
            [
                "This is someurl http://someurl!! This is someurl http://someurl!!",
                "中文 应该也 行 中文 应该也 行",
                "Сблъсъкът между Сблъсъкът между",
            ]
        )

        self.punctuated = tc.SArray(
            ["This is some url http://www.someurl.com!!", "Should we? Yes, we should."]
        )
        self.punctuated_double = tc.SArray(
            [
                "This is some url http://www.someurl.com!! This is some url http://www.someurl.com!!",
                "Should we? Yes, we should. Should we? Yes, we should.",
            ]
        )

        self.docs = tc.SArray(
            [
                {"this": 1, "is": 1, "a": 2, "sample": 1},
                {"this": 1, "is": 1, "another": 2, "example": 3},
            ]
        )

        self.sframe_comparer = util.SFrameComparer()

    def test_tokenize(self):
        sa_word_results = tc.text_analytics.tokenize(self.sa_word)

        self.assertEqual(
            sa_word_results[0],
            [
                "I",
                "like",
                "big",
                "dogs",
                "They",
                "are",
                "fun",
                "I",
                "LIKE",
                "BIG",
                "DOGS",
            ],
        )
        self.assertEqual(sa_word_results[1], ["I", "like"])
        self.assertEqual(sa_word_results[2], ["I", "like", "big"])

    def test_count_ngrams(self):
        # Testing word n-gram functionality
        result = tc.text_analytics.count_ngrams(self.sa_word, 3)
        result2 = tc.text_analytics.count_ngrams(self.sa_word, 2)
        result3 = tc.text_analytics.count_ngrams(
            self.sa_word, 3, "word", to_lower=False
        )
        result4 = tc.text_analytics.count_ngrams(
            self.sa_word, 2, "word", to_lower=False
        )
        expected = [
            {
                "fun i like": 1,
                "i like big": 2,
                "they are fun": 1,
                "big dogs they": 1,
                "like big dogs": 2,
                "are fun i": 1,
                "dogs they are": 1,
            },
            {},
            {"i like big": 1},
        ]
        expected2 = [
            {
                "i like": 2,
                "dogs they": 1,
                "big dogs": 2,
                "are fun": 1,
                "like big": 2,
                "they are": 1,
                "fun i": 1,
            },
            {"i like": 1},
            {"i like": 1, "like big": 1},
        ]
        expected3 = [
            {
                "I like big": 1,
                "fun I LIKE": 1,
                "I LIKE BIG": 1,
                "LIKE BIG DOGS": 1,
                "They are fun": 1,
                "big dogs They": 1,
                "like big dogs": 1,
                "are fun I": 1,
                "dogs They are": 1,
            },
            {},
            {"I like big": 1},
        ]
        expected4 = [
            {
                "I like": 1,
                "like big": 1,
                "I LIKE": 1,
                "BIG DOGS": 1,
                "are fun": 1,
                "LIKE BIG": 1,
                "big dogs": 1,
                "They are": 1,
                "dogs They": 1,
                "fun I": 1,
            },
            {"I like": 1},
            {"I like": 1, "like big": 1},
        ]

        self.assertEqual(result.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result, expected)
        self.assertEqual(result2.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result2, expected2)
        self.assertEqual(result3.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result3, expected3)
        self.assertEqual(result4.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result4, expected4)

        # Testing character n-gram functionality
        result5 = tc.text_analytics.count_ngrams(self.sa_char, 3, "character")
        result6 = tc.text_analytics.count_ngrams(self.sa_char, 2, "character")
        result7 = tc.text_analytics.count_ngrams(
            self.sa_char, 3, "character", to_lower=False
        )
        result8 = tc.text_analytics.count_ngrams(
            self.sa_char, 2, "character", to_lower=False
        )
        result9 = tc.text_analytics.count_ngrams(
            self.sa_char, 3, "character", to_lower=False, ignore_space=False
        )
        result10 = tc.text_analytics.count_ngrams(
            self.sa_char, 2, "character", to_lower=False, ignore_space=False
        )
        result11 = tc.text_analytics.count_ngrams(
            self.sa_char, 3, "character", to_lower=True, ignore_space=False
        )
        result12 = tc.text_analytics.count_ngrams(
            self.sa_char, 2, "character", to_lower=True, ignore_space=False
        )
        result13 = tc.text_analytics.count_ngrams(
            self.sa_char,
            3,
            "character",
            to_lower=False,
            ignore_punct=False,
            ignore_space=False,
        )
        result14 = tc.text_analytics.count_ngrams(
            self.sa_char,
            2,
            "character",
            to_lower=False,
            ignore_punct=False,
            ignore_space=False,
        )
        result15 = tc.text_analytics.count_ngrams(
            self.sa_char,
            3,
            "character",
            to_lower=False,
            ignore_punct=False,
            ignore_space=True,
        )
        result16 = tc.text_analytics.count_ngrams(
            self.sa_char,
            2,
            "character",
            to_lower=False,
            ignore_punct=False,
            ignore_space=True,
        )
        expected5 = [
            {"fun": 2, "nis": 1, "sfu": 1, "isf": 1, "uni": 1},
            {"fun": 2, "nis": 1, "sfu": 1, "isf": 1, "uni": 1},
            {},
            {"fun": 1},
        ]
        expected6 = [
            {"ni": 1, "is": 1, "un": 2, "sf": 1, "fu": 2},
            {"ni": 1, "is": 1, "un": 2, "sf": 1, "fu": 2},
            {"fu": 1},
            {"un": 1, "fu": 1},
        ]
        expected7 = [
            {"sfu": 1, "Fun": 1, "uni": 1, "fun": 1, "nis": 1, "isf": 1},
            {"sfu": 1, "Fun": 1, "uni": 1, "fun": 1, "nis": 1, "isf": 1},
            {},
            {"fun": 1},
        ]
        expected8 = [
            {"ni": 1, "Fu": 1, "is": 1, "un": 2, "sf": 1, "fu": 1},
            {"ni": 1, "Fu": 1, "is": 1, "un": 2, "sf": 1, "fu": 1},
            {"fu": 1},
            {"un": 1, "fu": 1},
        ]
        expected9 = [
            {
                " fu": 1,
                " is": 1,
                "s f": 1,
                "un ": 1,
                "Fun": 1,
                "n i": 1,
                "fun": 1,
                "is ": 1,
            },
            {
                " fu": 1,
                " is": 1,
                "s f": 1,
                "un ": 1,
                "Fun": 1,
                "n i": 1,
                "fun": 1,
                "is ": 1,
            },
            {},
            {"fun": 1},
        ]
        expected10 = [
            {" f": 1, "fu": 1, "n ": 1, "is": 1, " i": 1, "un": 2, "s ": 1, "Fu": 1},
            {" f": 1, "fu": 1, "n ": 1, "is": 1, " i": 1, "un": 2, "s ": 1, "Fu": 1},
            {"fu": 1},
            {"un": 1, "fu": 1},
        ]
        expected11 = [
            {" fu": 1, " is": 1, "s f": 1, "un ": 1, "n i": 1, "fun": 2, "is ": 1},
            {" fu": 1, " is": 1, "s f": 1, "un ": 1, "n i": 1, "fun": 2, "is ": 1},
            {},
            {"fun": 1},
        ]
        expected12 = [
            {" f": 1, "fu": 2, "n ": 1, "is": 1, " i": 1, "un": 2, "s ": 1},
            {" f": 1, "fu": 2, "n ": 1, "is": 1, " i": 1, "un": 2, "s ": 1},
            {"fu": 1},
            {"un": 1, "fu": 1},
        ]
        expected13 = [
            {
                " fu": 1,
                "s. ": 1,
                " is": 1,
                "n. ": 1,
                "Fun": 1,
                ". i": 1,
                "is.": 1,
                "fun": 1,
                ". f": 1,
                "un.": 1,
            },
            {
                " fu": 1,
                " is": 1,
                "s f": 1,
                "un ": 1,
                "Fun": 1,
                "n i": 1,
                "fun": 1,
                "is ": 1,
                "un.": 1,
            },
            {},
            {"fun": 1},
        ]
        expected14 = [
            {
                " f": 1,
                "fu": 1,
                "n.": 1,
                ". ": 2,
                "is": 1,
                " i": 1,
                "un": 2,
                "s.": 1,
                "Fu": 1,
            },
            {
                " f": 1,
                "fu": 1,
                "n.": 1,
                "n ": 1,
                "is": 1,
                " i": 1,
                "un": 2,
                "s ": 1,
                "Fu": 1,
            },
            {"fu": 1},
            {"un": 1, "fu": 1},
        ]
        expected15 = [
            {
                "s.f": 1,
                "n.i": 1,
                "Fun": 1,
                ".fu": 1,
                "is.": 1,
                "fun": 1,
                ".is": 1,
                "un.": 1,
            },
            {"sfu": 1, "Fun": 1, "uni": 1, "fun": 1, "nis": 1, "isf": 1, "un.": 1},
            {},
            {"fun": 1},
        ]
        expected16 = [
            {".i": 1, "fu": 1, "n.": 1, "is": 1, ".f": 1, "un": 2, "s.": 1, "Fu": 1},
            {"ni": 1, "fu": 1, "n.": 1, "is": 1, "un": 2, "sf": 1, "Fu": 1},
            {"fu": 1},
            {"un": 1, "fu": 1},
        ]

        self.assertEqual(result5.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result5, expected5)
        self.assertEqual(result6.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result6, expected6)
        self.assertEqual(result7.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result7, expected7)
        self.assertEqual(result8.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result8, expected8)
        self.assertEqual(result9.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result9, expected9)
        self.assertEqual(result10.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result10, expected10)
        self.assertEqual(result11.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result11, expected11)
        self.assertEqual(result12.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result12, expected12)
        self.assertEqual(result13.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result13, expected13)
        self.assertEqual(result14.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result14, expected14)
        self.assertEqual(result15.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result15, expected15)
        self.assertEqual(result16.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result16, expected16)

        ## Bogus input types and values
        sa = tc.SArray([1, 2, 3])
        with self.assertRaises(RuntimeError):
            tc.text_analytics.count_ngrams(sa)

        with self.assertRaises(TypeError):
            tc.text_analytics.count_ngrams(self.sa_word, n=1.01)

        with self.assertRaises(ValueError):
            tc.text_analytics.count_ngrams(self.sa_word, n=0)

        with self.assertRaises(ValueError):
            tc.text_analytics.count_ngrams(self.sa_char, n=3, method="bla")

        with warnings.catch_warnings(record=True) as context:
            warnings.simplefilter("always")
            tc.text_analytics.count_ngrams(self.sa_word, n=10, method="word")
            assert len(context) == 1

    def test_drop_words(self):
        ## Bogus input type
        sa = tc.SArray([1, 2, 3])
        with self.assertRaises(RuntimeError):
            tc.text_analytics.drop_words(sa)

        sa = tc.SArray(["str", None])
        # no throw, just give warning and skip
        # avoid segfault
        stop_words = tc.text_analytics.stop_words()
        tc.text_analytics.drop_words(sa, stop_words=stop_words)

        ## Other languages
        expected = [
            "this is someurl http someurl this is someurl http someurl",
            "中文 应该也 行 中文 应该也 行",
            "Сблъсъкът между Сблъсъкът между",
        ]

        expected2 = [
            "This is someurl http someurl This is someurl http someurl",
            "中文 应该也 行 中文 应该也 行",
            "Сблъсъкът между Сблъсъкът между",
        ]

        result = tc.text_analytics.drop_words(self.languages_double)
        self.assertEqual(result.dtype, str)
        self.sframe_comparer._assert_sarray_equal(result, expected)

        result = tc.text_analytics.drop_words(self.languages_double, to_lower=False)
        self.assertEqual(result.dtype, str)
        self.sframe_comparer._assert_sarray_equal(result, expected2)

        ## Check that delimiters work properly by default and when modified.
        expected1 = [
            "this is some url http www someurl com this is some url http www someurl com",
            "should we yes we should should we yes we should",
        ]
        expected2 = [
            "this is some url http://www.someurl.com this is some url http://www.someurl.com",
            "should we yes we should. should we yes we should.",
        ]
        expected3 = ["url http www someurl url http www someurl", ""]

        word_counts1 = tc.text_analytics.drop_words(self.punctuated_double)
        word_counts2 = tc.text_analytics.drop_words(
            self.punctuated_double, delimiters=["?", "!", ",", " "]
        )
        word_counts3 = tc.text_analytics.drop_words(
            self.punctuated_double, stop_words=tc.text_analytics.stop_words()
        )

        self.assertEqual(word_counts1.dtype, str)
        self.sframe_comparer._assert_sarray_equal(word_counts1, expected1)
        self.assertEqual(word_counts2.dtype, str)
        self.sframe_comparer._assert_sarray_equal(word_counts2, expected2)
        self.assertEqual(word_counts3.dtype, str)
        self.sframe_comparer._assert_sarray_equal(word_counts3, expected3)

    def test_count_words(self):
        ## Bogus input type
        sa = tc.SArray([1, 2, 3])
        with self.assertRaises(RuntimeError):
            tc.text_analytics.count_words(sa)

        ## Other languages
        expected = [
            {"this": 1, "http": 1, "someurl": 2, "is": 1},
            {"中文": 1, "应该也": 1, "行": 1},
            {"Сблъсъкът": 1, "между": 1},
        ]
        expected2 = [
            {"This": 1, "http": 1, "someurl": 2, "is": 1},
            {"中文": 1, "应该也": 1, "行": 1},
            {"Сблъсъкът": 1, "между": 1},
        ]

        result = tc.text_analytics.count_words(self.languages)
        self.assertEqual(result.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result, expected)

        result = tc.text_analytics.count_words(self.languages, to_lower=False)
        self.assertEqual(result.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result, expected2)

        ## Check that delimiters work properly by default and when modified.
        expected1 = [
            {
                "this": 1,
                "is": 1,
                "some": 1,
                "url": 1,
                "http": 1,
                "www": 1,
                "someurl": 1,
                "com": 1,
            },
            {"should": 2, "we": 2, "yes": 1},
        ]
        expected2 = [
            {"this is some url http://www.someurl.com": 1},
            {"should we": 1, " yes": 1, " we should.": 1},
        ]

        word_counts1 = tc.text_analytics.count_words(self.punctuated)
        word_counts2 = tc.text_analytics.count_words(
            self.punctuated, delimiters=["?", "!", ","]
        )

        self.assertEqual(word_counts1.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(word_counts1, expected1)
        self.assertEqual(word_counts2.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(word_counts2, expected2)

    def test_stop_words(self):
        """
        Check that the stop words can be accessed properly as part of the text
        analytics toolkit.
        """
        words = tc.text_analytics.stop_words()
        self.assertTrue(len(words) > 400)
        self.assertTrue("a" in words)

    def test_tf_idf(self):
        """
        Check correctness of the tf-idf mapping.
        """
        # Use the example on wikipedia
        tfidf_docs = tc.text_analytics.tf_idf(self.docs)

        self.assertAlmostEqual(tfidf_docs[1]["example"], 3 * math.log(2))
        self.assertAlmostEqual(tfidf_docs[0]["is"], 1 * math.log(1))

        empty_sa = tc.text_analytics.tf_idf(tc.SArray())
        self.assertEqual(len(empty_sa), 0)

        empty_dict_sf = tc.text_analytics.tf_idf(tc.SArray([{}]))
        assert len(empty_dict_sf) == 1
        assert len(empty_dict_sf.apply(lambda x: len(x) == 0)) == 1

    def test_count_words_dict_type(self):
        sa = tc.SArray(
            [
                {"Alice bob mike": 1, "Bob Alice Sue": 0.5, "": 100},
                {"a dog cow": 0, "a dog cat ": 5, "mice dog": -1, "mice cat": 2},
            ]
        )
        result = tc.text_analytics.count_words(sa)
        expected = [
            {"bob": 1.5, "mike": 1.0, "sue": 0.5, "alice": 1.5},
            {"a": 5.0, "mice": 1.0, "dog": 4.0, "cow": 0.0, "cat": 7.0},
        ]
        self.assertEqual(result.dtype, dict)
        self.sframe_comparer._assert_sarray_equal(result, expected)


class RandomWordSplitTest(unittest.TestCase):
    """
    Test that the random split utility in the text analytics toolkit works
    properly.
    """

    @classmethod
    def setUpClass(self):
        self.docs = tc.SArray([{"a": 3, "b": 5}, {"b": 5, "c": 7}, {"a": 2, "d": 3}])

    def test_random_split(self):
        """
        Test that the random split utility in the text analytics toolkit works
        properly.
        """

        train, test = tc.text_analytics.random_split(self.docs)

        assert len(train) == len(self.docs)
        assert len(test) == len(self.docs)

        # Iterate through each doc and each word
        for i in range(len(self.docs)):
            a = train[i]
            b = test[i]

            # Make sure there are no zero values
            for (k, v) in a.items():
                assert v != 0
            for (k, v) in b.items():
                assert v != 0

            # Make sure the counts add up to the original counts
            for (k, v) in self.docs[i].items():
                av = 0
                bv = 0
                if k in a:
                    av = a[k]
                if k in b:
                    bv = b[k]
                assert v == av + bv

        # Check that a low probability puts fewer items in the test set
        train, test = tc.text_analytics.random_split(self.docs, prob=0.001)

        total_in_train = train.dict_values().apply(lambda x: sum(x)).sum()
        total_in_test = test.dict_values().apply(lambda x: sum(x)).sum()
        assert total_in_train > total_in_test


class RetrievalTest(unittest.TestCase):
    """
    Test document retrieval functions in the `text_analytics` toolkit.
    """

    @classmethod
    def setUpClass(self):
        self.data = tc.SArray(
            [
                {"a": 5, "b": 7, "c": 10},
                {"a": 3, "c": 1, "d": 2},
                {"a": 10, "b": 3, "e": 5},
                {"a": 1},
                {"f": 5},
            ]
        )

    def test_bm25(self):
        """
        Check correctness of the BM2.5 query.
        """

        # Test input formats
        query = ["a", "b", "c"]
        assert tc.text_analytics.bm25(self.data, query) is not None

        query = tc.SArray(["a", "b", "c"])
        assert tc.text_analytics.bm25(self.data, query) is not None

        query = {"a": 5, "b": 3, "c": 1}
        assert tc.text_analytics.bm25(self.data, query) is not None

        # Only documents containing query words are included in result
        assert tc.text_analytics.bm25(self.data, query).num_rows() == 4

        dataset = tc.SArray(
            [
                {"a": 5, "b": 7, "c": 10},
                {"a": 3, "c": 1, "d": 2},
                None,
                {"a": 1},
                {"f": 5},
            ]
        )

        res = tc.text_analytics.bm25(dataset, query)
        assert res.num_rows() == 3
