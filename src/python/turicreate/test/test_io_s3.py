# -*- coding: utf-8 -*-
# Copyright © 2020 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
#
from __future__ import print_function as _  # noqa
from __future__ import division as _  # noqa
from __future__ import absolute_import as _  # noqa
from ..data_structures.sframe import SFrame
from ..data_structures.sarray import SArray
from turicreate.util import _assert_sframe_equal

import tempfile
import os
import six
import shutil
import boto3
import warnings

import pytest

pytestmark = [pytest.mark.minimal]

# size from small to big: 76K, 21MB, 77MB.
# 64MB is the cache block size. The big sframe with 77MB is used to
# ensure there's no issues when crossing different cache blocks.
remote_sframe_folders = ["small_sframe_dc", "medium_sframe_ac", "big_sframe_od"]
remote_sarray_folders = ["tiny_array"]


@pytest.mark.skipif(
    os.environ.get("TC_ENABLE_S3_TESTS", "0") == "0",
    reason="slow IO test; enabled when needed",
)
class TestSFrameS3(object):
    @classmethod
    def setup_class(self):
        self.my_tempdir = tempfile.mkdtemp()
        self.s3_client = boto3.client(
            "s3",
            endpoint_url=os.environ["TURI_S3_ENDPOINT"],
            region_name=os.environ["TURI_S3_REGION"],
        )
        self.bucket = "tc_qa"
        self.s3_root_prefix = "integration/manual/"
        self.s3_sframe_prefix = os.path.join(self.s3_root_prefix, "sframes/")
        self.s3_sarray_prefix = os.path.join(self.s3_root_prefix, "sarrays/")

        # download all related files once
        self.downloaded_files = dict()

        for folder in remote_sframe_folders:
            tmp_folder = os.path.join(self.my_tempdir, folder)
            # force clean in case same tempdir is reused without cleaning
            try:
                shutil.rmtree(tmp_folder)
            except OSError:
                pass
            except Exception as e:
                warnings.warn(
                    "Error raised while cleaning up %s: %s" % (tmp_folder, str(e))
                )
                raise e

            os.mkdir(tmp_folder)

            folder_to_read = os.path.join(self.s3_sframe_prefix, folder)

            if not folder_to_read.endswith("/"):
                folder_to_read += "/"

            result = self.s3_client.list_objects_v2(
                Bucket=self.bucket, Delimiter="/", Prefix=folder_to_read
            )

            # assert the folder doesn't contain sub-folders
            assert len(result.get("CommonPrefixes", [])) == 0

            for s3_object in result["Contents"]:
                key = s3_object["Key"]
                fname = os.path.join(tmp_folder, os.path.basename(key))
                self.s3_client.download_file(self.bucket, key, fname)

            self.downloaded_files[folder + "_path"] = tmp_folder
            self.downloaded_files[folder + "_sframe"] = SFrame(tmp_folder)

    @classmethod
    def teardown_class(self):
        try:
            shutil.rmtree(self.my_tempdir)
        except Exception as e:
            warnings.warn(
                "Error raised while cleaning up %s: %s" % (self.my_tempdir, str(e))
            )

    def test_s3_csv(self):
        fname = os.path.join(self.my_tempdir, "mushroom.csv")
        obj_key = os.path.join(self.s3_root_prefix, "csvs", "mushroom.csv")
        self.s3_client.download_file(self.bucket, obj_key, fname)
        sf_from_disk = SFrame(fname)
        s3_url = os.path.join("s3://", self.bucket, obj_key)
        sf_from_s3 = SFrame(s3_url)
        _assert_sframe_equal(sf_from_disk, sf_from_s3)

    @pytest.mark.parametrize("folder", remote_sframe_folders)
    def test_s3_sframe_download(self, folder):
        sf_from_disk = self.downloaded_files[folder + "_sframe"]
        obj_key = os.path.join(self.s3_sframe_prefix, folder)
        s3_url = os.path.join("s3://", self.bucket, obj_key)
        sf_from_s3 = SFrame(s3_url)
        _assert_sframe_equal(sf_from_disk, sf_from_s3)

    @pytest.mark.parametrize("folder", remote_sarray_folders)
    def test_s3_sarray_download(self, folder):
        s3_url = os.path.join("s3://", self.bucket, self.s3_sarray_prefix, folder)
        array = SArray(s3_url)
        assert len(array) == 1
        assert array[0] == 1

    @pytest.mark.skip(
        reason=(
            "need a S3 service that allows us delete directory."
            "See details from https://github.com/apple/turicreate/issues/3169"
        )
    )
    @pytest.mark.parametrize("folder", remote_sframe_folders)
    def test_s3_sframe_upload(self, folder):
        # s3 only writes when it receives all parts
        # it's sort of atmoic write on file level.
        sf_from_disk = self.downloaded_files[folder + "_sframe"]
        obj_key = os.path.join(self.s3_root_prefix, "upload", folder)
        s3_url = os.path.join("s3://", self.bucket, obj_key)
        # should not raise any thing
        sf_from_disk.save(s3_url)
        # we can download it again since there's no deletion there
        # but it's quite time consumming
        # we can trust the upload becuase if the upload fails,
        # s3 will respond with 5xx

    @pytest.mark.parametrize(
        "url",
        ["s3://gui/dummy", "./willy-nily/blah", "https://foo.com", "http://hao.com"],
    )
    def test_s3_sframe_load_from_wrong_path(self, url):
        # s3 only writes when it receives all parts
        # it's sort of atmoic write on file level.
        if six.PY2:
            with pytest.raises(IOError):
                SFrame(url)
        else:
            with pytest.raises(OSError):
                SFrame(url)

    def test_s3_sframe_upload_throw(self):
        # s3 only writes when it receives all parts
        # it's sort of atmoic write on file level.
        non_exist_folder = "not_a_folder_@a@"
        sf = SFrame({"a": [1, 2, 3]})
        obj_key = os.path.join(self.s3_root_prefix, "avalon", non_exist_folder)
        s3_url = os.path.join("s3://", self.bucket, obj_key)
        if six.PY2:
            with pytest.raises(IOError):
                sf.save(s3_url)
        else:
            with pytest.raises(OSError):
                sf.save(s3_url)
