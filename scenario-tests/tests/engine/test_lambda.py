# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import turicreate
import logging
import multiprocessing
import sys
import time
import unittest

from turicreate._connect import main as glconnect
from turicreate._cython import cy_test_utils

def fib(i):
    if i <= 2:
        return 1
    else:
        return fib(i - 1) + fib(i - 2)

@unittest.skipIf(sys.platform == 'win32', "No engine_virtualenv test on win32, since there is no 'system Python'")
class LambdaTests(unittest.TestCase):

    def test_simple_evaluation(self):
        x = 3
        self.assertEqual(glconnect.get_unity().eval_lambda(lambda y: y + x, 0), 3)
        self.assertEqual(glconnect.get_unity().eval_lambda(lambda y: y + x, 1), 4)
        self.assertEqual(glconnect.get_unity().eval_lambda(lambda x: x.upper(), 'abc'), 'ABC')
        self.assertEqual(glconnect.get_unity().eval_lambda(lambda x: x.lower(), 'ABC'), 'abc')
        self.assertEqual(glconnect.get_unity().eval_lambda(fib, 1), 1)

    def test_exception(self):
        x = 3
        self.assertRaises(RuntimeError, glconnect.get_unity().eval_lambda, lambda y: x / y, 0)
        self.assertRaises(RuntimeError, glconnect.get_unity().parallel_eval_lambda, lambda y: x / y, [0 for i in range(10)])

    def test_parallel_evaluation(self):
        xin = 33
        repeat = 8
        # execute the task bulk using one process to get a baseline
        start_time = time.time()
        glconnect.get_unity().eval_lambda(lambda x: [fib(i) for i in x], [xin]*repeat)
        single_thread_time = time.time() - start_time
        logging.info("Single thread lambda eval takes %s secs" % single_thread_time)

        # execute the task in parallel
        start_time = time.time()
        ans_list = glconnect.get_unity().parallel_eval_lambda(lambda x: fib(x), [xin]*repeat)
        multi_thread_time = time.time() - start_time
        logging.info("Multi thread lambda eval takes %s secs" % multi_thread_time)

        # test the speed up by running in parallel
        nproc = multiprocessing.cpu_count()
        if (nproc > 1 and multi_thread_time > (single_thread_time / 1.5)):
            logging.warning("Slow parallel processing: single thread takes %s secs, multithread on %s procs takes %s secs" % (single_thread_time, nproc, multi_thread_time))

        # test accuracy
        ans = fib(xin)
        for a in ans_list:
            self.assertEqual(a, ans)

    @unittest.skip("Disabling crash recovery test")
    def test_crash_recovery(self):
        import time, sys
        ls = list(range(1000))

        def good_fun(x):
            return x

        def bad_fun(x):
            if (x+1) % 251 == 0:
                cy_test_utils.force_exit_fun()  # this will force the worker process to exit
            return x
        self.assertRaises(RuntimeError, lambda: glconnect.get_unity().parallel_eval_lambda(lambda x: bad_fun(x), ls))
        glconnect.get_unity().parallel_eval_lambda(lambda x: good_fun(x), ls)
