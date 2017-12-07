# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
try:
    from io import BytesIO as _StringIO
except ImportError:
    from StringIO import StringIO as _StringIO

from ..deps import numpy as _np, HAS_NUMPY as _HAS_NUMPY
import array as _array

_JPG = "JPG"
_PNG = "PNG"
_RAW = "RAW"


_UNDEFINED = "UNDEFINED"
_CURRENT_VERSION = 0


_format = {_JPG: 0, _PNG: 1, _RAW: 2, _UNDEFINED: 3}


class Image(object):
    """
    A class whose objects contains image data, meta-data, and supports useful
    utility methods. This is intended to make image pre-processing easier for
    input into subsequent learning methods. Supports reading of JPEG and PNG
    formats.


    Parameters
    ----------

    path : string
        The path of the location of the image file to be loaded into the Image
        object.

    format : {'auto' | 'JPG' | 'PNG'}, optional
        Defaults to 'auto'. A format hint for the constructor. If left as
        'auto', constructor tries to infer file format from file extension.
        Otherwise, tries to decode file as whatever the format hint suggests.

    See Also
    --------
    turicreate.image_analysis.load_images
    turicreate.image_analysis.resize

    Examples
    --------

    >>> img = turicreate.Image('https://static.turi.com/datasets/images/sample.jpg')
    >>> turicreate.SArray([img]).show()
    """

    def __init__(self, path=None, format='auto', **__internal_kw_args):
        self._image_data = bytearray()
        self._height = 0
        self._width = 0
        self._channels = 0
        self._image_data_size = 0
        self._version = _CURRENT_VERSION
        self._format_enum = _format[_UNDEFINED]

        if (path is not None):
            from ..util import _make_internal_url
            from .. import extensions as _extensions
            img = _extensions.load_image(_make_internal_url(path), format)
            for key, value in list(img.__dict__.items()):
                setattr(self, key, value)
        else:
            for key, value in list(__internal_kw_args.items()):
                setattr(self, key, value)

    @property
    def height(self):
        """
        Returns the height of the image stored in the Image object.

        Returns
        -------
        out : int
            The height of the image stored in the Image object.

        See Also
        --------
        width, channels, pixel_data

        Examples
        --------

        >>> img = turicreate.Image('https://static.turi.com/datasets/images/sample.jpg')
        >>> img.height

        """
        return self._height

    @property
    def width(self):
        """
        Returns the width of the image stored in the Image object.

        Returns
        -------
        out : int
            The width of the image stored in the Image object.

        See Also
        --------
        height, channels, pixel_data

        Examples
        --------

        >>> img = turicreate.Image('https://static.turi.com/datasets/images/sample.jpg')
        >>> img.width

        """
        return self._width

    @property
    def channels(self):
        """
        Returns the number of channels in the image stored in the Image object.

        Returns
        -------
        out : int
            The number of channels in the image stored in the Image object.

        See Also
        --------
        width, height, pixel_data

        Examples
        --------

        >>> img = turicreate.Image('https://static.turi.com/datasets/images/sample.jpg')
        >>> img.channels

        """
        return self._channels

    @property
    def pixel_data(self):
        """
        Returns the pixel data stored in the Image object.

        Returns
        -------
        out : {array.array | numpy.array}
            The pixel data of the Image object. If user has numpy, it
            is returned as a multi-dimensional numpy array, where the shape of the
            array represents the shape of the image.  Otherwise, it is
            returned as a flat array.array with interleaved pixel values
            ie. it is stored as RGBRGBRGB, where each sequence of three values
            represents the Red, Green, and Blue. If the image is grayscale,
            there is only one value per pixel. If the image is RGBA, there are
            four values per pixel. The pixels are stored in row-major order.


        See Also
        --------
        width, channels, height

        Examples
        --------

        >>> img = turicreate.Image('https://static.turi.com/datasets/images/sample.jpg')
        >>> image_array = img.pixel_data
        """

        try:
            pil_img = self._to_pil_image()
            if _HAS_NUMPY:
                return _np.asarray(pil_img)
            else:
                ret = _array.array('B')
                if self._channels == 1:
                    ret.fromlist([z for z in pil_img.getdata()])
                else:
                    ret.fromlist([z for i in pil_img.getdata() for z in i])
                return ret
        except ImportError:
            print("Install pillow to get the pixel_data property")

    def __eq__(self, other):
        if isinstance(other, self.__class__):
            return self.__dict__ == other.__dict__
        else:
            return False

    def __ne__(self, other):
        return not self.__eq__(other)

    def __str__(self):
        return self.__repr__()

    def __repr__(self):
        ret = "Height: " + str(self._height) + "px\n"
        ret = ret + "Width: " + str(self._width) + "px\n"
        ret = ret + "Channels: " + str(self._channels) + "\n"
        return ret

    def _to_pil_image(self):
        from PIL import Image as _PIL_image
        if self._format_enum == _format[_RAW]:
            if self.channels == 1:
                img = _PIL_image.frombytes('L', (self._width, self._height), bytes(self._image_data))
            elif self.channels == 3:
                img = _PIL_image.frombytes('RGB', (self._width, self._height), bytes(self._image_data))
            elif self.channels == 4:
                img = _PIL_image.frombytes('RGBA', (self._width, self._height), bytes(self._image_data))
            else:
                raise ValueError('Unsupported channel size: ' + str(self.channels))
        else:
            img = _PIL_image.open(_StringIO(self._image_data))
        return img

    def show(self):
        """
        Displays the image. Requires PIL/Pillow.

        Alternatively, you can create an :class:`turicreate.SArray` of this image
        and use py:func:`turicreate.SArray.show()`

        See Also
        --------
        turicreate.image_analysis.resize

        Examples
        --------

        >>> img = turicreate.Image('https://static.turi.com/datasets/images/sample.jpg')
        >>> img.show()

        """
        try:
            img = self._to_pil_image()
            img.show()
        except ImportError:
            print("Install pillow to use the .show() method.")

