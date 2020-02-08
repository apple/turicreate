# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import hashlib as _hashlib
import os as _os
import turicreate as _tc
import shutil as _shutil
import tarfile as _tarfile

from six.moves.urllib import parse as _urlparse
from ._pre_trained_models import _download_and_checksum_files
from ._pre_trained_models import _get_cache_dir

DATA_URL_ROOT = "https://docs-assets.developer.apple.com/turicreate/data/"


class OneShotObjectDetectorBackgroundData(object):
    def __init__(self):
        self.source_tar_filename = "one_shot_backgrounds.sarray.tar"
        self.destination_tar_filename = "one_shot_backgrounds.sarray.tar"
        self.destination_sarray_filename = "one_shot_backgrounds.sarray"
        self.destination_sarray_path = _os.path.join(
            _get_cache_dir("data"), self.destination_sarray_filename
        )
        self.sarray_url = _urlparse.urljoin(DATA_URL_ROOT, self.source_tar_filename)
        self.sarray_url_md5_pairs = [
            (self.sarray_url, "08830e90771897c1cd187a07cdcb52b4")
        ]

        self.extracted_file_to_md5 = {
            'dir_archive.ini': '160fe6e7cb81cb0a29fd09239fdb2559',
            'm_d761047844237e5d.0000': 'd29b68f8ba196f60e0ad115f7bfde863',
            'm_d761047844237e5d.sidx': '22b0c297aabb836a21d3179c05c9c455',
            'objects.bin': 'd41d8cd98f00b204e9800998ecf8427e'
        }

    def get_backgrounds(self):
        # Download tar file, if not already downloaded
        # Get tar file path
        tarfile_path = _download_and_checksum_files(
            self.sarray_url_md5_pairs, _get_cache_dir("data")
        )[0]

        # Extract SArray from tar file, if not already extracted
        if _os.path.exists(self.destination_sarray_path):
            backgrounds_tar = _tarfile.open(tarfile_path)
            backgrounds_tar.extractall(_get_cache_dir("data"))

        # Verify and load the extracted SArray
        try:
            # Check we extracted the file we expected
            expected_extracted_files = set(self.extracted_file_to_md5.keys())
            extracted_files = set(_os.listdir(self.destination_sarray_path))
            assert expected_extracted_files == extracted_files

            # Check each of the files is what we expect
            for filename, expected_md5 in self.extracted_file_to_md5.items():
                full_path = _os.path.join(_get_cache_dir("data"), filename)
                md5 = hashlib.md5(full_path).hexdigest()
                assert md5 == expected_md5

            backgrounds = _tc.SArray(self.destination_sarray_path)
        except:
            # delete the incompletely/incorrectly extracted tarball bits on disk
            if _os.path.exists(self.destination_sarray_path):
                _shutil.rmtree(self.destination_sarray_path)
            # and re-extract
            backgrounds_tar = _tarfile.open(tarfile_path)
            backgrounds_tar.extractall(_get_cache_dir("data"))
            backgrounds = _tc.SArray(self.destination_sarray_path)

        return backgrounds
