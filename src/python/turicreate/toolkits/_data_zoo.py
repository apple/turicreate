# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import os as _os
import turicreate as _tc
import shutil as _shutil
import tarfile as _tarfile

from six.moves.urllib import parse as _urlparse
from ._pre_trained_models import _download_and_checksum_files
from ._pre_trained_models import _get_cache_dir

DATA_URL_ROOT = 'https://docs-assets.developer.apple.com/turicreate/data/'

class OneShotObjectDetectorBackgroundData(object):
    def __init__(self):
        self.source_tar_filename = "one_shot_backgrounds.sarray.tar"
        self.destination_tar_filename = "one_shot_backgrounds.sarray.tar"
        self.destination_sarray_filename = "one_shot_backgrounds.sarray"
        self.destination_sarray_path = _os.path.join(_get_cache_dir("data"),
            self.destination_sarray_filename)
        self.sarray_url = _urlparse.urljoin(
            DATA_URL_ROOT, self.source_tar_filename)
        self.sarray_url_md5_pairs = [
            (self.sarray_url, "08830e90771897c1cd187a07cdcb52b4")
            ]

    def get_backgrounds(self):
        tarfile_path = _download_and_checksum_files(
            self.sarray_url_md5_pairs, _get_cache_dir("data")
            )[0]
        backgrounds_tar = _tarfile.open(tarfile_path)
        try:
            backgrounds = _tc.SArray(self.destination_sarray_path)
        except:
            # delete the incompletely extracted tarball bits on disk
            if _os.path.exists(self.destination_sarray_path):
                _shutil.rmtree(self.destination_sarray_path)
            # and re-extract
            backgrounds_tar.extractall(_get_cache_dir("data"))
            backgrounds = _tc.SArray(self.destination_sarray_path)

        return backgrounds
