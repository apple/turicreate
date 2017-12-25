# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import os
import unittest
import tempfile
import shutil
import uuid
import sys as _sys

from .. import util as glutil
from .. import SFrame, SArray, SGraph
from ..util import get_turicreate_object_type
from ..config import get_runtime_config, set_runtime_config
from . import util


class UtilTests(unittest.TestCase):
    def test_archive_utils(self):
        # Arrange
        sf = SFrame([1, 2, 3, 4, 5])
        dir = tempfile.mkdtemp(prefix='archive-tests')
        try:
            sf.save(dir)

            # Act & Assert
            self.assertTrue(glutil.is_directory_archive(dir))
            self.assertEqual(glutil.get_archive_type(dir), 'sframe')
            self.assertFalse(glutil.is_directory_archive('/tmp'))
            self.assertRaises(TypeError, lambda: glutil.get_archive_type('/tmp'))
        finally:
            shutil.rmtree(dir)

    def test_crossproduct(self):
        s = util.SFrameComparer()

        d = {'opt1': [1, 2, 3],
             'opt2': ['a', 'b']}
        actual = glutil.crossproduct(d)
        actual = actual.sort('opt1')
        expected = SFrame({'opt1': [1, 1, 2, 2, 3, 3],
                                    'opt2': ['a', 'b', 'a', 'b', 'a', 'b']})
        # Check columns individually since there is no
        # guaranteed ordering among columns.
        for k in d.keys():
            s._assert_sarray_equal(actual[k],
                                   expected[k])


    def _validate_gl_object_type(self, obj, expected):
        with util.TempDirectory() as temp_dir:
            obj.save(temp_dir)
            t = get_turicreate_object_type(temp_dir)
            self.assertEqual(t, expected)

    def test_get_turicreate_object_type(self):
        sf = SFrame({"a":[1,2]})
        self._validate_gl_object_type(sf, 'sframe')

        sa = SArray([1,2])
        self._validate_gl_object_type(sa, 'sarray')

        d = SFrame(
                {"__src_id":[175343, 163607, 44041, 101370, 64892],
                 "__dst_id":[1011, 7928, 7718, 12966, 11080]})

        g = SGraph()
        self._validate_gl_object_type(g, 'sgraph')

    def test_sframe_equals(self):
        # Empty SFrames should be equal
        sf_a = SFrame()
        sf_b = SFrame()

        glutil._assert_sframe_equal(sf_a, sf_b)

        the_data = [i for i in range(0,10)]
        sf = SFrame()
        sf['ints'] = SArray(data=the_data, dtype=int)
        sf['floats'] = SArray(data=the_data, dtype=float)
        sf['floats'] = sf['floats'] * .5
        sf['strings'] = SArray(data=the_data, dtype=str)
        sf['strings'] = sf['strings'].apply(lambda x: x+x+x)

        # Make sure these aren't pointing to the same SFrame
        sf_a = sf.filter_by([43], 'ints', exclude=True)
        sf_b = sf.filter_by([43], 'ints', exclude=True)

        glutil._assert_sframe_equal(sf_a, sf_b)

        # Difference in number of columns
        sf_a['extra'] = SArray(data=the_data)
        with self.assertRaises(AssertionError):
            glutil._assert_sframe_equal(sf_a, sf_b)

        del sf_a['extra']
        glutil._assert_sframe_equal(sf_a, sf_b)

        # Difference in number of rows
        with self.assertRaises(AssertionError):
            glutil._assert_sframe_equal(sf_a, sf_b[0:5])

        # Difference in types
        sf_a['diff_type'] = sf_a['ints'].astype(str)
        sf_b['diff_type'] = sf_b['ints']
        with self.assertRaises(AssertionError):
            glutil._assert_sframe_equal(sf_a, sf_b)

        del sf_a['diff_type']
        del sf_b['diff_type']
        glutil._assert_sframe_equal(sf_a, sf_b)

        # Difference in column name
        sf_a.rename({'strings':'string'}, inplace=True)
        with self.assertRaises(AssertionError):
            glutil._assert_sframe_equal(sf_a, sf_b)

        glutil._assert_sframe_equal(sf_a, sf_b, check_column_names=False)

        sf_a.rename({'string':'strings'}, inplace=True)
        glutil._assert_sframe_equal(sf_a, sf_b)

        sf_a.rename({'ints':'floats1'}, inplace=True)
        sf_a.rename({'floats':'ints'}, inplace=True)
        sf_a.rename({'floats1':'floats'}, inplace=True)
        glutil._assert_sframe_equal(sf_a, sf_b, check_column_names=False)

        sf_a = sf.filter_by([43], 'ints', exclude=True)

        # Difference in column order
        sf_a.swap_columns('strings', 'ints', inplace=True)
        with self.assertRaises(AssertionError):
            glutil._assert_sframe_equal(sf_a, sf_b)

        glutil._assert_sframe_equal(sf_a, sf_b, check_column_order=False)

        sf_a.swap_columns('strings', 'ints', inplace=True)
        glutil._assert_sframe_equal(sf_a, sf_b)

        # Difference in row order
        sf_a = sf_a.append(sf[0:5])
        sf_b = sf[0:5].append(sf_b)
        with self.assertRaises(AssertionError):
            glutil._assert_sframe_equal(sf_a, sf_b)

        glutil._assert_sframe_equal(sf_a, sf_b, check_row_order=False)

        # Difference in column order AND row order
        sf_a.swap_columns('floats', 'strings', inplace=True)
        with self.assertRaises(AssertionError):
            glutil._assert_sframe_equal(sf_a, sf_b)

        glutil._assert_sframe_equal(sf_a, sf_b, check_column_order=False, check_row_order=False)

        # Column order, row order, names
        sf_a.rename({'floats':'foo','strings':'bar','ints':'baz'}, inplace=True)
        with self.assertRaises(AssertionError):
            glutil._assert_sframe_equal(sf_a, sf_b)

        # Illegal stuff
        with self.assertRaises(ValueError):
            glutil._assert_sframe_equal(sf_a, sf_b, check_column_names=False, check_column_order=False)

        with self.assertRaises(ValueError):
            glutil._assert_sframe_equal(sf_a, sf_b, check_column_names=False, check_column_order=False, check_row_order=False)

        with self.assertRaises(TypeError):
            glutil._assert_sframe_equal(sf_b['floats'], sf_a['foo'])

    def test_get_temp_file_location(self):
        from ..util import _get_temp_file_location
        from ..util import _convert_slashes
        location = _get_temp_file_location()
        self.assertTrue(os.path.isdir(location))

        tmp = tempfile.mkdtemp(prefix='test_gl_util')
        default_tmp = get_runtime_config()['TURI_CACHE_FILE_LOCATIONS']
        try:
            set_runtime_config('TURI_CACHE_FILE_LOCATIONS', tmp)
            location = _convert_slashes(_get_temp_file_location())
            self.assertTrue(location.startswith(_convert_slashes(tmp)))
        finally:
            shutil.rmtree(tmp)
            set_runtime_config('TURI_CACHE_FILE_LOCATIONS', default_tmp)

    def test_make_temp_directory(self):
        from ..util import _make_temp_directory, _get_temp_file_location
        tmp_root = _get_temp_file_location()

        location = _make_temp_directory(prefix=None)
        try:
            self.assertTrue(os.path.isdir(location))
            self.assertTrue(location.startswith(tmp_root))
        finally:
            shutil.rmtree(location)

        prefix = 'abc_'
        location = _make_temp_directory(prefix=prefix)
        try:
            self.assertTrue(os.path.isdir(location))
            self.assertTrue(location.startswith(tmp_root))
            self.assertTrue(os.path.basename(location).startswith(prefix))
        finally:
            shutil.rmtree(location)

    def test_make_temp_filename(self):
        from ..util import _make_temp_filename, _get_temp_file_location
        tmp_root = _get_temp_file_location()

        location = _make_temp_filename(prefix=None)
        self.assertFalse(os.path.isfile(location))
        self.assertFalse(os.path.exists(location))
        self.assertTrue(location.startswith(tmp_root))
        self.assertTrue(isinstance(location, str))

        prefix = 'abc_'
        location = _make_temp_filename(prefix=prefix)
        self.assertFalse(os.path.isfile(location))
        self.assertFalse(os.path.exists(location))
        self.assertTrue(location.startswith(tmp_root))
        self.assertTrue(isinstance(location, str))
        self.assertTrue(os.path.basename(location).startswith(prefix))


@unittest.skipIf(_sys.platform == 'win32' or _sys.platform == 'darwin', 'Not supported on Windows or Mac')
class SubprocessExecTest(unittest.TestCase):
    def test_exec(self):
        ret_dict = glutil.subprocess_exe('echo', ['hello_world'])
        self.assertEqual(ret_dict['success'], True)
        self.assertEqual(ret_dict['return_code'], 0)
        self.assertEqual(ret_dict['stdout'], 'hello_world\n')
        self.assertEqual(ret_dict['stderr'], '')
        self.assertEqual(ret_dict['python_exception'], None)

    def test_exec_with_setup_teardown(self):
        f = tempfile.NamedTemporaryFile(delete=False)

        def setup_fn():
            f.write(b'hello_world\n')
            f.close()

        def teardown_fn():
            os.remove(f.name)

        ret_dict = glutil.subprocess_exe('cat', [f.name], setup_fn, teardown_fn)
        self.assertEqual(ret_dict['success'], True)
        self.assertEqual(ret_dict['return_code'], 0)
        self.assertEqual(ret_dict['stdout'], 'hello_world\n')
        self.assertEqual(ret_dict['stderr'], '')
        self.assertEqual(ret_dict['python_exception'], None)

    def test_setup_exception(self):
        def setup_fn():
            raise RuntimeError('error')

        ret_dict = glutil.subprocess_exe('ls', [], setup_fn)
        self.assertEqual(ret_dict['success'], False)
        self.assertEqual(ret_dict['return_code'], None)
        self.assertEqual(ret_dict['stdout'], None)
        self.assertEqual(ret_dict['stderr'], None)
        self.assertEqual(type(ret_dict['python_exception']), RuntimeError)
        self.assertEqual(str(ret_dict['python_exception']), 'error')

    def test_process_exception(self):
        ret_dict = glutil.subprocess_exe('cp', [])
        self.assertEqual(ret_dict['success'], False)
        self.assertNotEquals(ret_dict['return_code'], 0)
        self.assertEqual(ret_dict['stdout'], "")
        self.assertNotEquals(ret_dict['stderr'], "")
        self.assertEqual(ret_dict['python_exception'], None)

    def test_tear_down_exception(self):
        def teardown_fn():
            raise RuntimeError('error')
        ret_dict = glutil.subprocess_exe('echo', [], teardown=teardown_fn)
        self.assertEqual(ret_dict['success'], True)
        self.assertEqual(ret_dict['return_code'], 0)
        self.assertEqual(ret_dict['stdout'], "\n")
        self.assertEqual(ret_dict['stderr'], "")
        self.assertEqual(ret_dict['python_exception'], None)
        self.assertEqual(type(ret_dict['_tear_down_exception']), RuntimeError)
        self.assertEqual(str(ret_dict['_tear_down_exception']), 'error')

    def test_exec_with_logfile(self):
        log_prefix = 'test-log-' + str(uuid.uuid4())
        expected_fout = log_prefix + '.stdout'
        expected_ferr = log_prefix + '.stderr'
        ret_dict = glutil.subprocess_exe('echo', ['hello_world'],
                                      out_log_prefix=log_prefix)
        self.assertEqual(ret_dict['success'], True)
        self.assertEqual(ret_dict['return_code'], 0)
        self.assertEqual(ret_dict['stdout'], expected_fout)
        self.assertEqual(ret_dict['stderr'], expected_ferr)
        self.assertEqual(ret_dict['python_exception'], None)
        with open(expected_fout) as f:
            stdout = f.read()
            self.assertEqual(stdout, 'hello_world\n')
        os.remove(expected_fout)
        os.remove(expected_ferr)

    def test_exec_with_logfile_exception(self):
        log_prefix = 'unknown_protocol://test-log-' + str(uuid.uuid4())
        ret_dict = glutil.subprocess_exe('echo', ['hello_world'],
                                      out_log_prefix=log_prefix)
        self.assertEqual(ret_dict['success'], True)
        self.assertEqual(ret_dict['return_code'], 0)
        self.assertEqual(ret_dict['stdout'], None)
        self.assertEqual(ret_dict['stderr'], None)
        self.assertEqual(ret_dict['python_exception'], None)
        self.assertNotEquals(ret_dict['_save_log_exception'], None)
