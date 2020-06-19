# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import turicreate as _tc

import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits._main import ToolkitError as _ToolkitError
import turicreate.toolkits._internal_utils as _tkutl
from .._internal_utils import _mac_ver
from .. import _pre_trained_models
from ...data_structures.image import Image as _Image
from .. import _image_feature_extractor

MODEL_TO_FEATURE_SIZE_MAPPING = {
    "resnet-50": 2048,
    "squeezenet_v1.1": 1000,
    "VisionFeaturePrint_Scene": 2048
}

def load_images(
    url,
    format="auto",
    with_path=True,
    recursive=True,
    ignore_failure=True,
    random_order=False,
):
    """
    Loads images from a directory. JPEG and PNG images are supported.

    Parameters
    ----------
    url : str
        The string of the path where all the images are stored.

    format : {'PNG' | 'JPG' | 'auto'}, optional
        The format of the images in the directory. The default 'auto' parameter
        value tries to infer the image type from the file extension. If a
        format is specified, all images must be of that format.

    with_path : bool, optional
        Indicates whether a path column is added to the SFrame. If 'with_path'
        is set to True,  the returned SFrame contains a 'path' column, which
        holds a path string for each Image object.

    recursive : bool, optional
        Indicates whether 'load_images' should do recursive directory traversal,
        or a flat directory traversal.

    ignore_failure : bool, optional
        If true, prints warning for failed images and keep loading the rest of
        the images.

    random_order : bool, optional
        Load images in random order.

    Returns
    -------
    out : SFrame
        Returns an SFrame with either an 'image' column or both an 'image' and
        a 'path' column. The 'image' column is a column of Image objects. If
        with_path is True, there is also a 'path' column which contains the image
        path for each of each corresponding Image object.

    Examples
    --------

    >>> url ='https://static.turi.com/datasets/images/nested'
    >>> image_sframe = turicreate.image_analysis.load_images(url, "auto", with_path=False,
    ...                                                       recursive=True)
    """
    from ... import extensions as _extensions
    from ...util import _make_internal_url

    url = _make_internal_url(url)
    return _extensions.load_images(
        url, format, with_path, recursive, ignore_failure, random_order
    )


def _decode(image_data):
    """
    Internal helper function for decoding a single Image or an SArray of Images
    """
    from ...data_structures.sarray import SArray as _SArray
    from ... import extensions as _extensions

    if type(image_data) is _SArray:
        return _extensions.decode_image_sarray(image_data)
    elif type(image_data) is _Image:
        return _extensions.decode_image(image_data)


def resize(image, width, height, channels=None, decode=False, resample="nearest"):
    """
    Resizes the image or SArray of Images to a specific width, height, and
    number of channels.

    Parameters
    ----------

    image : turicreate.Image | SArray
        The image or SArray of images to be resized.
    width : int
        The width the image is resized to.
    height : int
        The height the image is resized to.
    channels : int, optional
        The number of channels the image is resized to. 1 channel
        corresponds to grayscale, 3 channels corresponds to RGB, and 4
        channels corresponds to RGBA images.
    decode : bool, optional
        Whether to store the resized image in decoded format. Decoded takes
        more space, but makes the resize and future operations on the image faster.
    resample : 'nearest' or 'bilinear'
        Specify the resampling filter:

            - ``'nearest'``: Nearest neigbhor, extremely fast
            - ``'bilinear'``: Bilinear, fast and with less aliasing artifacts

    Returns
    -------
    out : turicreate.Image
        Returns a resized Image object.

    Notes
    -----
    Grayscale Images -> Images with one channel, representing a scale from
    white to black

    RGB Images -> Images with 3 channels, with each pixel having Green, Red,
    and Blue values.

    RGBA Images -> An RGB image with an opacity channel.

    Examples
    --------

    Resize a single image

    >>> img = turicreate.Image('https://static.turi.com/datasets/images/sample.jpg')
    >>> resized_img = turicreate.image_analysis.resize(img,100,100,1)

    Resize an SArray of images

    >>> url ='https://static.turi.com/datasets/images/nested'
    >>> image_sframe = turicreate.image_analysis.load_images(url, "auto", with_path=False,
    ...                                                    recursive=True)
    >>> image_sarray = image_sframe["image"]
    >>> resized_images = turicreate.image_analysis.resize(image_sarray, 100, 100, 1)
    """

    if height < 0 or width < 0:
        raise ValueError("Cannot resize to negative sizes")
    if resample not in ("nearest", "bilinear"):
        raise ValueError("Unknown resample option: '%s'" % resample)

    from ...data_structures.sarray import SArray as _SArray
    from ... import extensions as _extensions
    import turicreate as _tc

    if type(image) is _Image:

        assert resample in ("nearest", "bilinear")
        resample_method = 0 if resample == "nearest" else 1

        if channels is None:
            channels = image.channels
        if channels <= 0:
            raise ValueError("cannot resize images to 0 or fewer channels")
        return _extensions.resize_image(
            image, width, height, channels, decode, resample_method
        )
    elif type(image) is _SArray:
        if channels is None:
            channels = 3
        if channels <= 0:
            raise ValueError("cannot resize images to 0 or fewer channels")
        return image.apply(
            lambda x: _tc.image_analysis.resize(
                x, width, height, channels, decode, resample
            )
        )
    else:
        raise ValueError(
            "Cannot call 'resize' on objects that are not either an Image or SArray of Images"
        )


def get_deep_features(images, model_name, batch_size=64, verbose=True):
    """
    Extracts features from images from a specific model.

    Parameters
    ----------
    images : SArray
        Input data.

    model_name : string
        string optional
        Uses a pretrained model to bootstrap an image classifier:

           - "resnet-50" : Uses a pretrained resnet model.
                           Exported Core ML model will be ~90M.

           - "squeezenet_v1.1" : Uses a pretrained squeezenet model.
                                 Exported Core ML model will be ~4.7M.

           - "VisionFeaturePrint_Scene": Uses an OS internal feature extractor.
                                          Only on available on iOS 12.0+,
                                          macOS 10.14+ and tvOS 12.0+.
                                          Exported Core ML model will be ~41K.

        Models are downloaded from the internet if not available locally. Once
        downloaded, the models are cached for future use.

    Returns
    -------
    out : SArray
        Returns an SArray with all the extracted features.

    Examples
    --------
    # Get Deep featuers from an sarray of images
    >>> url ='https://static.turi.com/datasets/images/nested'
    >>> image_sframe = turicreate.image_analysis.load_images(url, "auto", with_path=False, recursive=True)
    >>> image_sarray = image_sframe["image"]
    >>> deep_features_sframe = turicreate.image_analysis.get_deep_features(image_sarray, model_name="resnet-50")

    """

    # Check model parameter
    allowed_models = list(_pre_trained_models.IMAGE_MODELS.keys())
    if _mac_ver() >= (10, 14):
        allowed_models.append("VisionFeaturePrint_Scene")
    _tkutl._check_categorical_option_type("model", model_name, allowed_models)

    # Check images parameter
    if not isinstance(images, _tc.SArray):
        raise TypeError("Unrecognized type for 'images'. An SArray is expected.")
    if len(images) == 0:
        raise _ToolkitError("Unable to extract features on an empty SArray object")

    if batch_size < 1:
        raise ValueError("'batch_size' must be greater than or equal to 1")

    # Extract features
    feature_extractor = _image_feature_extractor._create_feature_extractor(model_name)
    images_sf = _tc.SFrame({"image":images})
    return feature_extractor.extract_features(images_sf, "image", verbose=verbose, 
        batch_size=batch_size)


def _find_only_image_extracted_features_column(sframe, model_name):
    """
    Finds the only column in `sframe` with a type of array.array and has
    the length same as the last layer of the model in use.
    If there are zero or more than one image columns, an exception will
    be raised.
    """
    from array import array

    feature_column = _tkutl._find_only_column_of_type(sframe, target_type=array, type_name="array", col_name="deep_features")
    if _is_image_deep_feature_sarray(sframe[feature_column], model_name):
        return feature_column
    else:
        raise _ToolkitError('No "{col_name}" column specified and no column with expected type "{type_name}" is found.'.format(col_name="deep_features", type_name="array")
        )


def _is_image_deep_feature_sarray(feature_sarray, model_name):
    """
    Finds if the given `SArray` has extracted features for a given model_name.
    """
    from array import array

    if not (len(feature_sarray) > 0):
        return False
    if feature_sarray.dtype != array:
        return False
    if type(feature_sarray[0]) != array:
        return False
    if len(feature_sarray[0]) != MODEL_TO_FEATURE_SIZE_MAPPING[model_name]:
        return False
    return True
