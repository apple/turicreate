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

import tempfile
import os
import math
import shutil
import pytest
import boto3


@pytest.mark.skipif(
    os.environ.get("DTURI_ENABLE_SF_S3") is None,
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
        self.s3_prefix = "integration/manual/"

    @classmethod
    def teardown_class(self):
        shutil.rmtree(self.my_tempdir)

    def _assert_sarray_equal(self, sa1, sa2):
        l1 = list(sa1)
        l2 = list(sa2)
        self.assertEqual(len(l1), len(l2))
        for i in range(len(l1)):
            v1 = l1[i]
            v2 = l2[i]
            if v1 is None:
                self.assertEqual(v2, None)
            else:
                if type(v1) == dict:
                    self.assertEqual(len(v1), len(v2))
                    for key in v1:
                        self.assertTrue(key in v1)
                        self.assertEqual(v1[key], v2[key])

                elif hasattr(v1, "__iter__"):
                    self.assertEqual(len(v1), len(v2))
                    for j in range(len(v1)):
                        t1 = v1[j]
                        t2 = v2[j]
                        if type(t1) == float:
                            if math.isnan(t1):
                                self.assertTrue(math.isnan(t2))
                            else:
                                self.assertEqual(t1, t2)
                        else:
                            self.assertEqual(t1, t2)
                else:
                    self.assertEqual(v1, v2)

    def _test_sframe_equal(self, sf1, sf2):
        # asserts two frames are equal, ignoring column ordering.
        self.assertEqual(sf1.num_rows(), sf2.shape[0])
        self.assertEqual(sf1.num_columns(), sf2.shape[1])
        for col in sf1.column_names():
            self._assert_sarray_equal(sf1[col], sf2[col])

    def test_s3_csv(self):
        fname = os.path.join(self.my_tempdir, "mushroom.csv")
        obj_key = os.path.join(self.s3_prefix, "csvs", "mushroom.csv")
        self.s3_client.download_file(self.bucket, obj_key, fname)
        sf_from_disk = SFrame(fname)
        s3_url = os.path.join("s3://", self.bucket, obj_key)
        sf_from_s3 = SFrame(s3_url)
        self._test_sframe_equal(sf_from_disk, sf_from_s3)

    @pytest.mark.parametrize(
        "folder", ["big_sframe_od", "medium_sframe_ac", "small_sframe_dc"]
    )
    def test_s3_sframe_download_and_upload(self, folder):
        tmp_folder = os.path.join(self.my_tempdir, folder)
        try:
            shutil.rmtree(tmp_folder)
        except FileNotFoundError:
            pass
        read_prefix = os.path.join(self.s3_prefix, "sframes/")
        folder_to_read = os.path.join(read_prefix, folder) + "/"
        result = self.s3_client.list_objects_v2(
            Bucket=self.bucket, Delimiter="/", Prefix=folder_to_read
        )

        self.assertEqual(len(result["CommonPrefix"]), 0)
        for s3_object in result["Contents"]:
            key = s3_object["Key"].decode("utf-8")
            fname = os.path.join(tmp_folder, os.path.basename(key))
            self.s3_client.download_file(self.bucket, key, fname)
        sf_from_disk = SFrame(tmp_folder)

        obj_key = os.path.join(read_prefix, folder)
        s3_url = os.path.join("s3://", self.bucket, obj_key)
        sf_from_s3 = SFrame(s3_url)
        self._test_sframe_equal(sf_from_disk, sf_from_s3)

        upload_path = os.path.join(self.s3_prefix, "upload", folder)
        result = self.s3_client.list_objects_v2(
            Bucket=self.bucket, Delimiter="/", Prefix=upload_path + "/"
        )

        if result.get("Contents", None):
            s3_objects = result["Contents"]
            self.s3_client.delete_objects(
                Bucket=self.bucket, Key={"Objects": s3_objects}
            )

        sf_from_disk.save(upload_path)
        sf_from_s3_upload = SFrame(upload_path)
        self._test_sframe_equal(sf_from_disk, sf_from_s3_upload)
