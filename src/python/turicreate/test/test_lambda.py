# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import multiprocessing
import time
import unittest
import logging
from .._connect import main as glconnect
from .._cython import cy_test_utils
import os


def fib(i):
    if i <= 2:
        return 1
    else:
        return fib(i - 1) + fib(i - 2)


class LambdaTests(unittest.TestCase):
    def test_simple_evaluation(self):
        x = 3
        self.assertEqual(glconnect.get_unity().eval_lambda(lambda y: y + x, 0), 3)
        self.assertEqual(glconnect.get_unity().eval_lambda(lambda y: y + x, 1), 4)
        self.assertEqual(
            glconnect.get_unity().eval_lambda(lambda x: x.upper(), "abc"), "ABC"
        )
        self.assertEqual(
            glconnect.get_unity().eval_lambda(lambda x: x.lower(), "ABC"), "abc"
        )
        self.assertEqual(glconnect.get_unity().eval_lambda(fib, 1), 1)

    def test_exception(self):
        x = 3
        self.assertRaises(
            RuntimeError, glconnect.get_unity().eval_lambda, lambda y: x / y, 0
        )
        self.assertRaises(
            RuntimeError,
            glconnect.get_unity().parallel_eval_lambda,
            lambda y: x / y,
            [0 for i in range(10)],
        )

    def test_parallel_evaluation(self):
        xin = 33
        repeat = 8
        # execute the task bulk using one process to get a baseline
        start_time = time.time()
        glconnect.get_unity().eval_lambda(lambda x: [fib(i) for i in x], [xin] * repeat)
        single_thread_time = time.time() - start_time
        logging.info("Single thread lambda eval takes %s secs" % single_thread_time)

        # execute the task in parallel
        start_time = time.time()
        ans_list = glconnect.get_unity().parallel_eval_lambda(
            lambda x: fib(x), [xin] * repeat
        )
        multi_thread_time = time.time() - start_time
        logging.info("Multi thread lambda eval takes %s secs" % multi_thread_time)

        # test the speed up by running in parallel
        nproc = multiprocessing.cpu_count()
        if nproc > 1 and multi_thread_time > (single_thread_time / 1.5):
            logging.warning(
                "Slow parallel processing: single thread takes %s secs, multithread on %s procs takes %s secs"
                % (single_thread_time, nproc, multi_thread_time)
            )

        # test accuracy
        ans = fib(xin)
        for a in ans_list:
            self.assertEqual(a, ans)

    def test_environments(self):
        def test_env(i):

            if i > 500:
                assert os.environ["OMP_NUM_THREADS"] == "1"
                assert os.environ["OPENBLAS_NUM_THREADS"] == "1"
                assert os.environ["MKL_NUM_THREADS"] == "1"
                assert os.environ["MKL_DOMAIN_NUM_THREADS"] == "1"
                assert os.environ["NUMBA_NUM_THREADS"] == "1"

            return i

        import turicreate as tc

        x = tc.SArray(range(10000))
        y = x.apply(test_env)

        self.assertTrue((x == y).all())

    @unittest.skip("Disabling crash recovery test")
    def test_crash_recovery(self):
        import time, sys

        ls = range(1000)

        def good_fun(x):
            return x

        def bad_fun(x):
            if (x + 1) % 251 == 0:
                cy_test_utils.force_exit_fun()  # this will force the worker process to exit
            return x

        self.assertRaises(
            RuntimeError,
            lambda: glconnect.get_unity().parallel_eval_lambda(
                lambda x: bad_fun(x), ls
            ),
        )
        glconnect.get_unity().parallel_eval_lambda(lambda x: good_fun(x), ls)

    @unittest.skip(
        "Disabling test as previous runs of this can mess up import.  Reenamble when lambda workers can be reliably restarted."
    )
    def test_expensive_packages_not_imported_in_lambda(self):

        import turicreate as tc

        expensive_packages = ["mxnet", "resampy"]

        # we don't want mxnet to be imported in the worker code
        def lambda_func(x):

            import sys

            if x >= 1000:
                for p in expensive_packages:
                    assert p not in sys.modules

            return x + 1

        x = tc.SArray(range(2000)).apply(lambda_func)

        self.assertTrue((x == tc.SArray(range(1, 2001))).all())
