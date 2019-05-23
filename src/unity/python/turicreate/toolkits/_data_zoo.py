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
        self.source_tar_filename = "one_shot_backgrounds.sarray.tar"
        self.destination_tar_filename = "one_shot_backgrounds.sarray.tar"
        self.sarray_url = _urlparse.urljoin(
            DATA_URL_ROOT, self.source_tar_filename)
        self.sarray_url_md5_pairs = [
            (self.sarray_url, "08830e90771897c1cd187a07cdcb52b4")
            ]

    def get_backgrounds_path(self):
        backgrounds_path = _download_and_checksum_files(
            self.sarray_url_md5_pairs, _os.path.join(_get_data_cache_dir(), self.destination_tar_filename)
            )[0]
        return backgrounds_path
