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

MODELS_URL_ROOT = 'https://docs-assets.developer.apple.com/turicreate/models/'

def _get_model_cache_dir():
    tmp_dir = _tc.config.get_runtime_config()['TURI_CACHE_FILE_LOCATIONS']
    return _os.path.join(tmp_dir, 'model_cache')

def _download_and_checksum_files(urls, dirname, delete=False):
    if not _os.path.exists(dirname):
        _os.makedirs(dirname)

    def url_sha_pair(url_or_pair):
        if isinstance(url_or_pair, tuple):
            return url_or_pair
        else:
            return url_or_pair, None

    urls, shas = zip(*[url_sha_pair(pair) for pair in urls])
    fns = [
        _os.path.join(dirname, _os.path.basename(_urlparse.urlparse(url).path))
        for url in urls
    ]

    if delete:
        for fn in fns:
            if _os.path.exists(fn):
                _os.remove(fn)

    try:
        for url, fn, sha in zip(urls, fns, shas):
            if sha is not None:
                md5 = _hashlib.md5()

            if not _os.path.exists(fn):
                r = _requests.get(url, stream=True)
                assert r.status_code == 200, "%s (%d)" % (r.reason, r.status_code)
                print('Downloading', url)
                with open(fn, 'wb') as f:
                    BUFFER = 1 << 16
                    for i, chunk in enumerate(r.iter_content(chunk_size=BUFFER)):
                        if chunk:
                            f.write(chunk)
                            if sha is not None:
                                md5.update(chunk)
                if sha is not None:
                    assert sha == md5.hexdigest(), "mismatched checksum, please try the command again"
                print('Download completed:', fn)
    except (KeyboardInterrupt, AssertionError, _requests.RequestException) as e:
        # Only print if message is available (not the case for KeyboardInterrupt)
        if e:
            print('ERROR: Download failed:', e, file=_sys.stderr)
        for fn in fns:
            if _os.path.exists(fn):
                _os.remove(fn)
        _sys.exit(1)
    return fns

class ImageClassifierPreTrainedModel(object):
    @classmethod
    def _is_gl_pickle_safe(cls):
        return False


class ResNetImageClassifier(ImageClassifierPreTrainedModel):
    def __init__(self):
        self.name = 'resnet-50'
        self.num_classes = 1000
        self.feature_layer_size = 2048
        self.data_layer = 'data'
        self.output_layer = 'softmax_output'
        self.label_layer = 'softmax_label'
        self.feature_layer = 'flatten0_output'
        self.is_feature_layer_final = False
        self.input_image_shape = (3, 224, 224)
        epoch = 0
        self.symbols_url = _urlparse.urljoin(MODELS_URL_ROOT, '%s-symbol.json' % self.name)
        self.symbols_md5 = '2989c88d1d6629b777949a3ae695a42e'
        self.params_url = _urlparse.urljoin(MODELS_URL_ROOT, '%s-%04d.params' % (self.name, epoch))
        self.params_md5 = '246423771006aaf77acf68c99852f5b5'
        path = _get_model_cache_dir()
        _download_and_checksum_files([
            (self.symbols_url, self.symbols_md5),
            (self.params_url, self.params_md5),
        ], path)
        import mxnet as _mx
        self.mxmodel = _mx.model.load_checkpoint(_os.path.join(path, self.name), epoch)

class SqueezeNetImageClassifierV1_1(ImageClassifierPreTrainedModel):
    def __init__(self):
        self.name = 'squeezenet_v1.1'
        self.num_classes = 1000
        self.feature_layer_size = 1000
        self.is_feature_layer_final = True
        self.data_layer = 'data'
        self.output_layer = 'prob_output'
        self.label_layer = 'prob_label'
        self.feature_layer = 'flatten_output'
        self.input_image_shape = (3, 227, 227)
        epoch = 0
        self.symbols_url = _urlparse.urljoin(MODELS_URL_ROOT, '%s-symbol.json' % self.name)
        self.symbols_md5 = 'bab4d80f45e9285cf9f4a3f01f07022e'
        self.params_url = _urlparse.urljoin(MODELS_URL_ROOT, '%s-%04d.params' % (self.name, epoch))
        self.params_md5 = '05b1eb6acabdaaee37c9c9ff666c1b51'
        path = _get_model_cache_dir()
        _download_and_checksum_files([
            (self.symbols_url, self.symbols_md5),
            (self.params_url, self.params_md5),
        ], path)
        import mxnet as _mx
        self.mxmodel = _mx.model.load_checkpoint(_os.path.join(path, self.name), epoch)

MODELS = {
    'resnet-50': ResNetImageClassifier,
    'squeezenet_v1.1': SqueezeNetImageClassifierV1_1
}

class ObjectDetectorBasePreTrainedModel(object):
    @classmethod
    def _is_gl_pickle_safe(cls):
        return True


class DarkNetObjectDetectorBase(ObjectDetectorBasePreTrainedModel):
    def __init__(self):
        self.name = 'darknet'
        self.spatial_reduction = 32
        self.source_url = _urlparse.urljoin(MODELS_URL_ROOT, 'darknet.params')
        self.source_md5 = '1d7eea1fd286d2cfd7f2d9c93cbbdf9d'
        self.weight_names = []
        for i in range(7):
            self.weight_names += [
                'conv%d_weight' % i,
                'batchnorm%d_gamma' % i,
                'batchnorm%d_beta' % i,
            ]
        self.model_path = _download_and_checksum_files([
            (self.source_url, self.source_md5)
        ], _get_model_cache_dir())[0]

    def available_parameters_subset(self, mx_params):
        """
        Takes an mxnet parameter collect (from Block.collect_params()) and
        subsets it with the parameters available in this base network.
        """
        from copy import copy
        from collections import OrderedDict
        subset_params = copy(mx_params)
        subset_params._params = OrderedDict([
            (k, v) for k, v in mx_params.items() if k in self.weight_names
        ])
        return subset_params


OBJECT_DETECTION_BASE_MODELS = {
    'darknet': DarkNetObjectDetectorBase,
}
