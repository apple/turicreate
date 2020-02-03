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

MODELS_URL_ROOT = "https://docs-assets.developer.apple.com/turicreate/models/"


def _get_cache_dir(_type="model"):
    cache_dir = _tc.config.get_runtime_config()["TURI_CACHE_FILE_LOCATIONS"]
    if _type == "model":
        download_path = _os.path.join(cache_dir, "model_cache")
    else:
        download_path = _os.path.join(cache_dir, "data_cache")

    if not _os.path.exists(download_path):
        try:
            _os.makedirs(download_path)
        except:
            raise RuntimeError(
                'Could not write to the turicreate file cache, which is currently set to "{cache_dir}".\n'
                "To continue you must update this location to a writable path by calling:\n"
                "\ttc.config.set_runtime_config('TURI_CACHE_FILE_LOCATIONS', <path>)\n"
                "Where <path> is a writable file path that exists.".format(
                    cache_dir=cache_dir
                )
            )

    return download_path


def _download_and_checksum_files(urls, dirname, delete=False):
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
                print("Downloading", url)
                with open(fn, "wb") as f:
                    BUFFER = 1 << 16
                    for i, chunk in enumerate(r.iter_content(chunk_size=BUFFER)):
                        if chunk:
                            f.write(chunk)
                            if sha is not None:
                                md5.update(chunk)
                if sha is not None:
                    assert (
                        sha == md5.hexdigest()
                    ), "mismatched checksum, please try the command again"
                print("Download completed:", fn)
    except (KeyboardInterrupt, AssertionError, _requests.RequestException) as e:
        # Only print if message is available (not the case for KeyboardInterrupt)
        if e:
            print("ERROR: Download failed:", e, file=_sys.stderr)
        for fn in fns:
            if _os.path.exists(fn):
                _os.remove(fn)
        _sys.exit(1)
    return fns


class ImageClassifierPreTrainedModel(object):
    @classmethod
    def _is_gl_pickle_safe(cls):
        return False

    def get_model_path(self, format):
        assert format in ("coreml", "tensorflow")

        filename = self.name + "-TuriCreate-6.0"
        if format == "coreml":
            filename = filename + ".mlmodel"
        else:
            filename = filename + ".h5"

        url = _urlparse.urljoin(MODELS_URL_ROOT, filename)
        checksum = self.source_md5[format]
        model_path = _download_and_checksum_files([(url, checksum)], _get_cache_dir())[
            0
        ]

        return model_path


class ResNetImageClassifier(ImageClassifierPreTrainedModel):
    input_image_shape = (3, 224, 224)

    def __init__(self):
        self.name = "resnet-50"
        self.input_is_BGR = False

        self.coreml_data_layer = "data"
        self.coreml_feature_layer = "flatten0"

        self.source_md5 = {
            "coreml": "8503ef18f368b65ebaaa07ba5689b5f8",
            "tensorflow": "ac73d2cc03700035c6cd756742bd59d6",
        }


class SqueezeNetImageClassifierV1_1(ImageClassifierPreTrainedModel):
    input_image_shape = (3, 227, 227)

    def __init__(self):
        self.name = "squeezenet_v1.1"
        self.input_is_BGR = True

        self.coreml_data_layer = "data"
        self.coreml_feature_layer = "flatten"

        self.source_md5 = {
            "coreml": "5d8a41bb9a48f71b779a98b345de0900",
            "tensorflow": "60d5afff4c5bc535bc29655feac5571f",
        }


IMAGE_MODELS = {
    "resnet-50": ResNetImageClassifier,
    "squeezenet_v1.1": SqueezeNetImageClassifierV1_1,
}


class ObjectDetectorBasePreTrainedModel(object):
    @classmethod
    def _is_gl_pickle_safe(cls):
        return True


class DarkNetObjectDetectorBase(ObjectDetectorBasePreTrainedModel):
    def __init__(self):
        self.name = "darknet"
        self.spatial_reduction = 32
        self.source_url = _urlparse.urljoin(MODELS_URL_ROOT, "darknet.params")
        self.source_md5 = "1d7eea1fd286d2cfd7f2d9c93cbbdf9d"
        self.weight_names = []
        for i in range(7):
            self.weight_names += [
                "conv%d_weight" % i,
                "batchnorm%d_gamma" % i,
                "batchnorm%d_beta" % i,
            ]
        self.model_path = _download_and_checksum_files(
            [(self.source_url, self.source_md5)], _get_cache_dir()
        )[0]

    def available_parameters_subset(self, mx_params):
        """
        Takes an mxnet parameter collect (from Block.collect_params()) and
        subsets it with the parameters available in this base network.
        """
        from copy import copy
        from collections import OrderedDict

        subset_params = copy(mx_params)
        subset_params._params = OrderedDict(
            [(k, v) for k, v in mx_params.items() if k in self.weight_names]
        )
        return subset_params


class DarkNetObjectDetectorModel(ObjectDetectorBasePreTrainedModel):
    def __init__(self):
        self.name = "darknet"
        self.source_url = _urlparse.urljoin(MODELS_URL_ROOT, "darknet.mlmodel")
        self.source_md5 = "a06761976a0472cf0553b64ecc15b0fe"

    def get_model_path(self):
        model_path = _download_and_checksum_files(
            [(self.source_url, self.source_md5)], _get_cache_dir()
        )[0]
        return model_path


OBJECT_DETECTION_BASE_MODELS = {
    "darknet": DarkNetObjectDetectorBase,
    "darknet_mlmodel": DarkNetObjectDetectorModel,
}


class StyleTransferTransformer:
    def __init__(self):
        self.name = "resnet-16"
        self.source_md5 = {
            "mxnet": "ac232afa6d0ead93a8c75b6c455f6dd3",
            "coreml": "e0f3adaa9952ecc7d96f5e4eefb0d690",
        }

    def get_model_path(self, format):
        assert format in ("coreml", "mxnet")
        if format == "coreml":
            filename = self.name + ".mlmodel"
        else:
            filename = self.name + ".params"
        url = _urlparse.urljoin(MODELS_URL_ROOT, filename)
        checksum = self.source_md5[format]
        model_path = _download_and_checksum_files([(url, checksum)], _get_cache_dir())[
            0
        ]
        return model_path


class Vgg16:
    def __init__(self):
        self.name = "Vgg16-conv1_1-4_3"
        self.source_md5 = {
            "mxnet": "52e75e03160e64e5aa9cfbbc62a92345",
            "coreml": "9c9508a8256d9ca1c113ac94bc9f8c6f",
        }

    def get_model_path(self, format):
        assert format in ("coreml", "mxnet")
        if format in "coreml":
            filename = "vgg16-conv1_1-4_3.mlmodel"
        else:
            filename = "vgg16-conv1_1-4_3.params"
        url = _urlparse.urljoin(MODELS_URL_ROOT, filename)
        checksum = self.source_md5[format]
        model_path = _download_and_checksum_files([(url, checksum)], _get_cache_dir())[
            0
        ]
        return model_path


STYLE_TRANSFER_BASE_MODELS = {"resnet-16": StyleTransferTransformer, "Vgg16": Vgg16}


class VGGish:
    def __init__(self):
        self.name = "VGGishFeatureEmbedding-v1"
        self.source_md5 = {
            "coreml": "e8ae7d8cbcabb988b6ed6c0bf3f45571",
            "tensorflow": "1ae04d42492703e75fa79304873c642a",
        }

    def get_model_path(self, format):
        assert format in ("coreml", "tensorflow")

        if format == "coreml":
            filename = self.name + ".mlmodel"
        else:
            filename = self.name + ".h5"
        url = _urlparse.urljoin(MODELS_URL_ROOT, filename)

        checksum = self.source_md5[format]
        model_path = _download_and_checksum_files([(url, checksum)], _get_cache_dir())[
            0
        ]

        return model_path


class DrawingClassifierPreTrainedModel(object):
    def __init__(self, warm_start="auto"):
        self.model_to_filename = {
            "auto": "drawing_classifier_pre_trained_model_245_classes_v0.params",
            "quickdraw_245_v0": "drawing_classifier_pre_trained_model_245_classes_v0.params",
        }
        self.source_url = _urlparse.urljoin(
            MODELS_URL_ROOT, self.model_to_filename[warm_start]
        )
        # @TODO: Think about how to bypass the md5 checksum if the user wants to
        # provide their own pretrained model.
        self.source_md5 = "71ba78e48a852f35fb22999650f0a655"

    def get_model_path(self):
        model_path = _download_and_checksum_files(
            [(self.source_url, self.source_md5)], _get_cache_dir()
        )[0]
        return model_path


class DrawingClassifierPreTrainedMLModel(object):
    def __init__(self):
        self.source_url = _urlparse.urljoin(
            MODELS_URL_ROOT,
            "drawing_classifier_pre_trained_model_245_classes_v0.mlmodel",
        )
        self.source_md5 = "fc1c04126728514c47991a62b9e66715"

    def get_model_path(self):
        model_path = _download_and_checksum_files(
            [(self.source_url, self.source_md5)], _get_cache_dir()
        )[0]
        return model_path
