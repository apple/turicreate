# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ...data_structures.image import Image as _Image


def load_images(url, format='auto', with_path=True, recursive=True, ignore_failure=True, random_order=False):
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
    >>> image_sarray = turicreate.image_analysis.load_images(url, "auto", with_path=False,
    ...                                                    recursive=True)
    """
    from ... import extensions as _extensions
    from ...util import _make_internal_url
    return _extensions.load_images(_make_internal_url(url), format, with_path,
                                     recursive, ignore_failure, random_order)


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



def resize(image, width, height, channels=None, decode=False):
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

    from ...data_structures.sarray import SArray as _SArray
    from ... import extensions as _extensions
    if type(image) is _Image:
        if channels is None:
            channels = image.channels
        if channels <= 0:
            raise ValueError("cannot resize images to 0 or fewer channels")
        return _extensions.resize_image(image, width, height, channels, decode)
    elif type(image) is _SArray:
        if channels is None:
            channels = 3
        if channels <= 0:
            raise ValueError("cannot resize images to 0 or fewer channels")
        return image.apply(lambda x: _extensions.resize_image(x, width, height, channels, decode))
    else:
        raise ValueError("Cannot call 'resize' on objects that are not either an Image or SArray of Images")
