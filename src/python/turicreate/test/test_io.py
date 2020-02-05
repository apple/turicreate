# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import sys

if sys.version_info.major >= 3:
    import subprocess as commands
else:
    import commands

import json
import logging
import os
import fnmatch
import tempfile
import unittest
import pandas

from .._connect import main as glconnect
from .. import _sys_util
from .. import util
from .. import SGraph, SFrame, load_sgraph, load_model, load_sframe
from turicreate.toolkits._model import Model
from pandas.util.testing import assert_frame_equal

restricted_place = "/root"
if sys.platform == "win32":
    restricted_place = "C:/Windows/System32/config/RegBack"
elif sys.platform == "darwin":
    restricted_place = "/System"

if sys.version_info.major >= 3:
    unichr = chr


def _test_save_load_object_helper(testcase, obj, path):
    """
    Helper function to test save and load a server side object to a given url.
    """

    def cleanup(url):
        """
        Remove the saved file from temp directory.
        """
        tempdir = tempfile.gettempdir()
        pattern = path + "*"
        for f in os.listdir(tempdir):
            if fnmatch.fnmatch(pattern, f):
                os.remove(os.path.join(tempdir, f))

    def assert_same_elements(x, y):
        if sys.version_info.major >= 3:
            testcase.assertCountEqual(x, y)
        else:
            testcase.assertItemsEqual(x, y)

    if isinstance(obj, SGraph):
        obj.save(path + ".graph")
        newobj = load_sgraph(path + ".graph")
        assert_same_elements(obj.get_fields(), newobj.get_fields())
        testcase.assertDictEqual(obj.summary(), newobj.summary())
    elif isinstance(obj, Model):
        obj.save(path + ".model")
        newobj = load_model(path + ".model")
        assert_same_elements(obj._list_fields(), newobj._list_fields())
        testcase.assertEqual(type(obj), type(newobj))
    elif isinstance(obj, SFrame):
        obj.save(path + ".frame_idx")
        newobj = load_sframe(path + ".frame_idx")
        testcase.assertEqual(obj.shape, newobj.shape)
        testcase.assertEqual(obj.column_names(), newobj.column_names())
        testcase.assertEqual(obj.column_types(), newobj.column_types())
        assert_frame_equal(
            obj.head(obj.num_rows()).to_dataframe(),
            newobj.head(newobj.num_rows()).to_dataframe(),
        )
    else:
        raise TypeError
    cleanup(path)


def create_test_objects():
    vertices = pandas.DataFrame(
        {
            "vid": ["1", "2", "3"],
            "color": ["g", "r", "b"],
            "vec": [[0.1, 0.1, 0.1], [0.1, 0.1, 0.1], [0.1, 0.1, 0.1]],
        }
    )
    edges = pandas.DataFrame(
        {
            "src_id": ["1", "2", "3"],
            "dst_id": ["2", "3", "4"],
            "weight": [0.0, 0.1, 1.0],
        }
    )

    graph = SGraph().add_vertices(vertices, "vid").add_edges(edges, "src_id", "dst_id")
    sframe = SFrame(edges)
    return (graph, sframe)


class LocalFSConnectorTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.tempfile = tempfile.NamedTemporaryFile().name
        (self.graph, self.sframe) = create_test_objects()

    def _test_read_write_helper(self, url, content):
        url = util._make_internal_url(url)
        glconnect.get_unity().__write__(url, content)
        content_read = glconnect.get_unity().__read__(url)
        self.assertEqual(content_read, content)
        if os.path.exists(url):
            os.remove(url)

    def test_object_save_load(self):
        _test_save_load_object_helper(self, self.graph, self.tempfile)
        _test_save_load_object_helper(self, self.sframe, self.tempfile)

    def test_basic(self):
        self._test_read_write_helper(self.tempfile, "hello world")

    def test_gzip(self):
        self._test_read_write_helper(self.tempfile + ".gz", "hello world")
        self._test_read_write_helper(self.tempfile + ".csv.gz", "hello world")


class HttpConnectorTests(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.url = "http://s3-us-west-2.amazonaws.com/testdatasets/a_to_z.txt.gz"

    def _test_read_helper(self, url, content_expected):
        url = util._make_internal_url(url)
        content_read = glconnect.get_unity().__read__(url)
        self.assertEqual(content_read, content_expected)

    def test_read(self):
        expected = "\n".join([str(unichr(i + ord("a"))) for i in range(26)])
        expected = expected + "\n"
        self._test_read_helper(self.url, expected)

    def test_exception(self):
        self.assertRaises(
            IOError, lambda: glconnect.get_unity().__write__(self.url, ".....")
        )


@unittest.skip("Disabling HDFS Connector Tests")
class HDFSConnectorTests(unittest.TestCase):
    # This test requires hadoop to be installed and available in $PATH.
    # If not, the tests will be skipped.
    @classmethod
    def setUpClass(self):
        self.has_hdfs = len(_sys_util.get_hadoop_class_path()) > 0
        self.tempfile = tempfile.NamedTemporaryFile().name
        (self.graph, self.sframe) = create_test_objects()

    def _test_read_write_helper(self, url, content_expected):
        url = util._make_internal_url(url)
        glconnect.get_unity().__write__(url, content_expected)
        content_read = glconnect.get_unity().__read__(url)
        self.assertEqual(content_read, content_expected)
        # clean up the file we wrote
        status, output = commands.getstatusoutput("hadoop fs -test -e " + url)
        if status is 0:
            commands.getstatusoutput("hadoop fs -rm " + url)

    def test_basic(self):
        if self.has_hdfs:
            self._test_read_write_helper("hdfs://" + self.tempfile, "hello,world,woof")
        else:
            logging.getLogger(__name__).info("No hdfs available. Test pass.")

    def test_gzip(self):
        if self.has_hdfs:
            self._test_read_write_helper(
                "hdfs://" + self.tempfile + ".gz", "hello,world,woof"
            )
            self._test_read_write_helper(
                "hdfs://" + self.tempfile + ".csv.gz", "hello,world,woof"
            )
        else:
            logging.getLogger(__name__).info("No hdfs available. Test pass.")

    def test_object_save_load(self):
        if self.has_hdfs:
            prefix = "hdfs://"
            _test_save_load_object_helper(self, self.graph, prefix + self.tempfile)
            _test_save_load_object_helper(self, self.sframe, prefix + self.tempfile)
        else:
            logging.getLogger(__name__).info("No hdfs available. Test pass.")

    def test_exception(self):
        bad_url = "hdfs:///root/"
        if self.has_hdfs:
            self.assertRaises(
                IOError, lambda: glconnect.get_unity().__read__("hdfs:///")
            )
            self.assertRaises(
                IOError, lambda: glconnect.get_unity().__read__("hdfs:///tmp")
            )
            self.assertRaises(
                IOError,
                lambda: glconnect.get_unity().__read__("hdfs://" + self.tempfile),
            )
            self.assertRaises(
                IOError,
                lambda: glconnect.get_unity().__write__(
                    bad_url + "/tmp", "somerandomcontent"
                ),
            )
            self.assertRaises(IOError, lambda: self.graph.save(bad_url + "x.graph"))
            self.assertRaises(
                IOError, lambda: self.sframe.save(bad_url + "x.frame_idx")
            )
            self.assertRaises(IOError, lambda: load_sgraph(bad_url + "mygraph"))
            self.assertRaises(IOError, lambda: load_sframe(bad_url + "x.frame_idx"))
            self.assertRaises(IOError, lambda: load_model(bad_url + "x.model"))
        else:
            logging.getLogger(__name__).info("No hdfs available. Test pass.")


@unittest.skip("Disabling S3 Connector Tests")
class S3ConnectorTests(unittest.TestCase):
    # This test requires aws cli to be installed. If not, the tests will be skipped.
    @classmethod
    def setUpClass(self):
        status, output = commands.getstatusoutput("aws s3api list-buckets")
        self.has_s3 = status is 0
        self.standard_bucket = None
        self.regional_bucket = None
        # Use aws cli s3api to find a bucket with "gl-testdata" in the name, and use it as out test bucket.
        # Temp files will be read from /written to the test bucket's /tmp folder and be cleared on exist.
        if self.has_s3:
            try:
                json_output = json.loads(output)
                bucket_list = [b["Name"] for b in json_output["Buckets"]]
                assert "gl-testdata" in bucket_list
                assert "gl-testdata-oregon" in bucket_list
                self.standard_bucket = "gl-testdata"
                self.regional_bucket = "gl-testdata-oregon"
                self.tempfile = tempfile.NamedTemporaryFile().name
                (self.graph, self.sframe) = create_test_objects()
            except:
                logging.getLogger(__name__).warning(
                    "Fail parsing ioutput of s3api into json. Please check your awscli version."
                )
                self.has_s3 = False

    def _test_read_write_helper(self, url, content_expected):
        s3url = util._make_internal_url(url)
        glconnect.get_unity().__write__(s3url, content_expected)
        content_read = glconnect.get_unity().__read__(s3url)
        self.assertEqual(content_read, content_expected)
        (status, output) = commands.getstatusoutput(
            "aws s3 rm --region us-west-2 " + url
        )
        if status is not 0:
            logging.getLogger(__name__).warning("Cannot remove file: " + url)

    def test_basic(self):
        if self.has_s3:
            for bucket in [self.standard_bucket, self.regional_bucket]:
                self._test_read_write_helper(
                    "s3://" + bucket + self.tempfile, "hello,world,woof"
                )
        else:
            logging.getLogger(__name__).info("No s3 bucket available. Test pass.")

    def test_gzip(self):
        if self.has_s3:
            self._test_read_write_helper(
                "s3://" + self.standard_bucket + self.tempfile + ".gz",
                "hello,world,woof",
            )
        else:
            logging.getLogger(__name__).info("No s3 bucket available. Test pass.")

    def test_object_save_load(self):
        if self.has_s3:
            prefix = "s3://" + self.standard_bucket
            _test_save_load_object_helper(self, self.graph, prefix + self.tempfile)
            _test_save_load_object_helper(self, self.sframe, prefix + self.tempfile)
        else:
            logging.getLogger(__name__).info("No s3 bucket available. Test pass.")

    def test_exception(self):
        if self.has_s3:
            bad_bucket = "i_am_a_bad_bucket"
            prefix = "s3://" + bad_bucket
            self.assertRaises(IOError, lambda: glconnect.get_unity().__read__("s3:///"))
            self.assertRaises(
                IOError,
                lambda: glconnect.get_unity().__read__(
                    "s3://" + self.standard_bucket + "/somerandomfile"
                ),
            )
            self.assertRaises(
                IOError,
                lambda: glconnect.get_unity().__read__("s3://" + "/somerandomfile"),
            )
            self.assertRaises(
                IOError,
                lambda: glconnect.get_unity().__write__(
                    "s3://" + "/somerandomfile", "somerandomcontent"
                ),
            )
            self.assertRaises(
                IOError,
                lambda: glconnect.get_unity().__write__(
                    "s3://" + self.standard_bucket + "I'amABadUrl/", "somerandomcontent"
                ),
            )
            self.assertRaises(IOError, lambda: self.graph.save(prefix + "/x.graph"))
            self.assertRaises(
                IOError, lambda: self.sframe.save(prefix + "/x.frame_idx")
            )
            self.assertRaises(IOError, lambda: load_sgraph(prefix + "/x.graph"))
            self.assertRaises(IOError, lambda: load_sframe(prefix + "/x.frame_idx"))
            self.assertRaises(IOError, lambda: load_model(prefix + "/x.model"))
        else:
            logging.getLogger(__name__).info("No s3 bucket available. Test pass.")
