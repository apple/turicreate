# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import os as _os
import sys as _sys
import requests as _requests
import turicreate as _tc
import hashlib as _hashlib
from six.moves.urllib import parse as _urlparse
from ._pre_trained_models import _download_and_checksum_files

DATA_URL_ROOT = 'https://docs-assets.developer.apple.com/turicreate/data/'

def _get_data_cache_dir():
    cache_dir = _tc.config.get_runtime_config()['TURI_CACHE_FILE_LOCATIONS']
    download_path = _os.path.join(cache_dir, 'data_cache')

    if not _os.path.exists(download_path):
        try:
            _os.makedirs(download_path)
        except:
            raise RuntimeError("Could not write to the turicreate file cache, which is currently set to \"{cache_dir}\".\n"
                               "To continue you must update this location to a writable path by calling:\n"
                               "\ttc.config.set_runtime_config(\'TURI_CACHE_FILE_LOCATIONS\', <path>)\n"
                               "Where <path> is a writable file path that exists.".format(cache_dir=cache_dir))

    return download_path

class OneShotObjectDetectorBackgroundData(object):
    def __init__(self):
        self.sarray_url = _urlparse.urljoin(
            DATA_URL_ROOT, "one_shot_backgrounds.sarray")
        self.sarray_url_md5_pairs = [
            (_urlparse.urljoin(self.sarray_url, "dir_archive.ini"), "160fe6e7cb81cb0a29fd09239fdb2559"),
            (_urlparse.urljoin(self.sarray_url, "m_d761047844237e5d.sidx"), "22b0c297aabb836a21d3179c05c9c455"),
            (_urlparse.urljoin(self.sarray_url, "m_d761047844237e5d.0000"), "d29b68f8ba196f60e0ad115f7bfde863"),
            (_urlparse.urljoin(self.sarray_url, "objects.bin"), "d41d8cd98f00b204e9800998ecf8427e")
            ]

    def get_backgrounds_path(self):
        backgrounds_path = _download_and_checksum_files(
            self.sarray_url_md5_pairs, _get_data_cache_dir()
            )[0]
        return backgrounds_path
