# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import array
import unittest
from PIL import Image as PIL_Image
import os

from ..data_structures import image
from .. import SArray
from ..toolkits.image_analysis import image_analysis
from ..toolkits._main import ToolkitError

from .._deps import numpy as _np, HAS_NUMPY

current_file_dir = os.path.dirname(os.path.realpath(__file__))


class image_info:
    def __init__(self, url):
        self.url = url
        if "png" in url:
            self.format = "PNG"
        elif "jpg" in url:
            self.format = "JPG"
        if "grey" in url:
            self.channels = 1
        else:
            self.channels = 3


urls = [
    current_file_dir + x
    for x in [
        "/images/nested/sample_grey.jpg",
        "/images/nested/sample_grey.png",
        "/images/sample.jpg",
        "/images/sample.png",
    ]
]

test_image_info = [image_info(u) for u in urls]


class ImageClassTest(unittest.TestCase):
    def __check_raw_image_equals_pilimage(self, glimage, pilimage):
        # size equal
        self.assertEqual((glimage.width, glimage.height), pilimage.size)
        # decode image
        glimage_decoded = image_analysis._decode(glimage)
        self.assertEqual(glimage_decoded._format_enum, image._format["RAW"])
        # Getting data
        if glimage.channels == 1:
            pil_data = bytearray([z for z in pilimage.getdata()])
        else:
            pil_data = bytearray([z for l in pilimage.getdata() for z in l])

        # Size data equal
        self.assertEqual(glimage_decoded._image_data_size, len(pil_data))
        self.assertEqual(len(glimage_decoded._image_data), len(pil_data))
        pixel_diff = [
            abs(x - y) for (x, y) in zip(glimage_decoded._image_data, pil_data)
        ]

        self.assertLess(sum(pixel_diff) / float(len(pil_data)), 2)

    def __check_raw_image_metainfo(self, glimage, meta_image_info):
        # size channels equal
        self.assertEqual(glimage.channels, meta_image_info.channels)
        # size format equal
        self.assertEqual(glimage._format_enum, image._format[meta_image_info.format])
        #
        pilimage = PIL_Image.open(meta_image_info.url)
        self.__check_raw_image_equals_pilimage(glimage, pilimage)

    def test_construct(self):
        """
        Test constructing single image from PNG and JPEG format.
        """
        # Test construction with format hint
        for meta_info in test_image_info:
            glimage = image.Image(path=meta_info.url, format=meta_info.format)
            self.__check_raw_image_metainfo(glimage, meta_info)

        # Test construction with format inference
        for meta_info in test_image_info:
            glimage = image.Image(path=meta_info.url)
            self.__check_raw_image_metainfo(glimage, meta_info)

        # Test construction with wrong format
        for meta_info in test_image_info:
            if meta_info.url == "PNG":
                self.assertRaises(
                    RuntimeError, lambda: image.Image(path=meta_info.url, format="JPG")
                )
            elif meta_info.url == "JPG":
                self.assertRaises(
                    RuntimeError, lambda: image.Image(path=meta_info.url, format="PNG")
                )

        # Test empty image sarray
        sa = SArray([image.Image()] * 100)
        for i in sa:
            self.assertEqual(i.width, 0)
            self.assertEqual(i.height, 0)
            self.assertEqual(i.channels, 0)

    def test_resize(self):
        for meta_info in test_image_info:
            glimage = image.Image(path=meta_info.url, format=meta_info.format)

            # Test resize image to half the size and twice the size
            for scale in [0.5, 2]:
                # Test resize image with color type changes
                for new_channels in [None, 1, 3, 4]:
                    new_width = int(scale * glimage.width)
                    new_height = int(scale * glimage.height)
                    glimage_resized = image_analysis.resize(
                        glimage, new_width, new_height, new_channels
                    )
                    pilimage = PIL_Image.open(meta_info.url).resize(
                        (new_width, new_height), PIL_Image.NEAREST
                    )
                    if new_channels == 1:
                        pilimage = pilimage.convert("L")
                    elif new_channels == 3:
                        pilimage = pilimage.convert("RGB")
                    elif new_channels == 4:
                        pilimage = pilimage.convert("RGBA")
                    self.__check_raw_image_equals_pilimage(glimage_resized, pilimage)

    def test_cmyk_not_supported(self):
        for meta_info in test_image_info:
            input_img = PIL_Image.open(meta_info.url)
            input_img = input_img.convert("CMYK")

            import tempfile

            with tempfile.NamedTemporaryFile() as t:
                input_img.save(t, format="jpeg")
                with self.assertRaises(ToolkitError):
                    cmyk_image = image.Image(path=t.name, format="JPG")

    def test_batch_resize(self):
        image_url_dir = current_file_dir + "/images"
        sa = image_analysis.load_images(image_url_dir, "auto", with_path=False)["image"]
        for new_channels in [1, 3, 4]:
            sa_resized = image_analysis.resize(sa, 320, 280, new_channels)
            for i in sa_resized:
                self.assertEqual(i.width, 320)
                self.assertEqual(i.height, 280)
                self.assertEqual(i.channels, new_channels)

    def test_load_images(self):
        image_url_dir = current_file_dir + "/images"
        # Test auto format, with path and recursive
        sf1 = image_analysis.load_images(image_url_dir, "auto", True, True)
        self.assertEqual(sf1.num_columns(), 2)
        self.assertEqual(sf1.num_rows(), 18)
        self.assertEqual(sf1["image"].dtype, image.Image)

        # Test auto format, with path and non recursive
        sf2 = image_analysis.load_images(image_url_dir, "auto", True, False)
        self.assertEqual(sf2.num_columns(), 2)
        self.assertEqual(sf2.num_rows(), 2)
        self.assertEqual(sf2["image"].dtype, image.Image)

        # Test auto format, without path and recursive
        sf3 = image_analysis.load_images(image_url_dir, "auto", False, True)
        self.assertEqual(sf3.num_columns(), 1)
        self.assertEqual(sf3.num_rows(), 18)
        self.assertEqual(sf3["image"].dtype, image.Image)

        # Test auto format, without path and non recursive
        sf4 = image_analysis.load_images(image_url_dir, "auto", False, False)
        self.assertEqual(sf4.num_columns(), 1)
        self.assertEqual(sf4.num_rows(), 2)

        # Confirm that load_images works with a single image as well
        sf5 = image_analysis.load_images(
            image_url_dir + "/sample.jpg", "auto", False, False
        )
        self.assertEqual(sf5.num_columns(), 1)
        self.assertEqual(sf5.num_rows(), 1)

        # Expect error when trying to load PNG image as JPG
        with self.assertRaises(RuntimeError):
            image_analysis.load_images(image_url_dir, "JPG", ignore_failure=False)

        # Expect error when trying to load JPG image as PNG
        with self.assertRaises(RuntimeError):
            image_analysis.load_images(image_url_dir, "PNG", ignore_failure=False)

        # Setting ignore_failure to True, and we should be able to load images
        # to our best effort without throwing error
        image_analysis.load_images(image_url_dir, "JPG", ignore_failure=True)
        image_analysis.load_images(image_url_dir, "PNG", ignore_failure=True)

    def test_astype_image(self):
        import glob

        imagelist = glob.glob(current_file_dir + "/images/*/**")
        imageurls = SArray(imagelist)
        images = imageurls.astype(image.Image)
        self.assertEqual(images.dtype, image.Image)
        # check that we actually loaded something.
        for i in images:
            self.assertGreater(i.height, 0)
            self.assertGreater(i.width, 0)

        # try a bad image
        imageurls = SArray(["no_image_here", "go_away"])
        self.assertRaises(Exception, lambda: imageurls.astype(image.Image))
        ret = imageurls.astype(image.Image, undefined_on_failure=True)
        self.assertEqual(ret[0], None)
        self.assertEqual(ret[1], None)

    def test_casting(self):
        image_url_dir = current_file_dir + "/images/nested"
        sf = image_analysis.load_images(image_url_dir, "auto", True, True)
        sa = sf["image"]
        sa_vec = sa.astype(array.array)
        sa_img = sa_vec.pixel_array_to_image(sa[0].width, sa[0].height, sa[0].channels)
        sa_str = sa.astype(str)
        sa_str_expected = "Height: " + str(sa[0].height) + " Width: " + str(sa[0].width)
        decoded_image = image_analysis._decode(sa[0])
        self.assertEqual(sa_img[0].height, sa[0].height)
        self.assertEqual(sa_img[0].width, sa[0].width)
        self.assertEqual(sa_img[0].channels, sa[0].channels)
        self.assertEqual(sa_img[0]._image_data_size, decoded_image._image_data_size)
        self.assertEqual(sa_img[0]._image_data, decoded_image._image_data)
        self.assertEqual(sa_str[0], sa_str_expected)

    def test_lambda(self):
        image_url_dir = current_file_dir + "/images"
        sf = image_analysis.load_images(image_url_dir)
        sa = sf["image"]

        # Lambda returning self
        sa_self = sa.apply(lambda x: x)
        for i in range(len(sa_self)):
            self.assertEqual(sa[i], sa_self[i])

        # Lambda returning width
        sa_width = sa.apply(lambda x: x.width)
        for i in range(len(sa_width)):
            self.assertEqual(sa[i].width, sa_width[i])

        # Lambda returning height
        sa_height = sa.apply(lambda x: x.height)
        for i in range(len(sa_height)):
            self.assertEqual(sa[i].height, sa_height[i])

        # Lambda returning channels
        sa_channels = sa.apply(lambda x: x.channels)
        for i in range(len(sa_channels)):
            self.assertEqual(sa[i].channels, sa_channels[i])

        # Lambda returning resized self
        sa_resized = sa.apply(
            lambda x: image_analysis.resize(x, int(x.width / 2), int(x.height / 2))
        )
        for i in range(len(sa_resized)):
            self.assertEqual(sa_resized[i].width, int(sa[i].width / 2))

    def test_generate_mean(self):
        zeros = bytearray(100)
        fifties = bytearray([50] * 100)
        hundreds = bytearray([100] * 100)
        img1 = image.Image(
            _image_data=zeros,
            _channels=1,
            _height=1,
            _width=100,
            _image_data_size=100,
            _format_enum=2,
        )  # format 2 is RAW
        img2 = image.Image(
            _image_data=hundreds,
            _channels=1,
            _height=1,
            _width=100,
            _image_data_size=100,
            _format_enum=2,
        )
        img3 = image.Image(
            _image_data=fifties,
            _channels=1,
            _height=1,
            _width=100,
            _image_data_size=100,
            _format_enum=2,
        )

        sa = SArray([img1, img2])
        average = sa.mean()
        sa2 = SArray([img2, img2])
        average2 = sa2.mean()

        self.assertEqual(average, img3)
        self.assertEqual(average2, img2)

    def test_pixel_data(self):
        fifties = bytearray([50] * 100)
        img = image.Image(
            _image_data=fifties,
            _channels=1,
            _height=1,
            _width=100,
            _image_data_size=100,
            _format_enum=2,
        )
        pixel_data = img.pixel_data.flatten()
        self.assertEqual(pixel_data.shape, (100,))

        self.assertEqual(len(pixel_data), len(fifties))
        for p in range(len(pixel_data)):
            self.assertEqual(pixel_data[p], 50)

        # Load images and make sure shape is right
        img_color = image.Image(os.path.join(current_file_dir, "images", "sample.png"))
        self.assertEqual(img_color.pixel_data.shape, (444, 800, 3))

        img_gray = image.Image(
            os.path.join(current_file_dir, "images", "nested", "sample_grey.png")
        )
        self.assertEqual(img_gray.pixel_data.shape, (444, 800))

    def test_png_bitdepth(self):
        def path(name):
            return os.path.join(current_file_dir, "images", "bitdepths", name)

        # Test images with varying bitdepth and check correctness against 4 reference pixels
        images_info = [
            # path, bitdepth, pixel_data[0, 0], pixel_data[0, 1], pixel_data[0, 200], pixel_data[40, 400]
            (
                path("color_1bit.png"),
                [0, 0, 0],
                [0, 0, 0],
                [0, 255, 255],
                [255, 255, 0],
            ),
            (
                path("color_2bit.png"),
                [0, 0, 0],
                [0, 0, 0],
                [85, 255, 170],
                [170, 170, 85],
            ),
            (
                path("color_4bit.png"),
                [0, 0, 0],
                [0, 0, 0],
                [68, 221, 187],
                [153, 187, 102],
            ),
            (
                path("color_8bit.png"),
                [0, 0, 0],
                [0, 1, 2],
                [73, 219, 182],
                [146, 182, 109],
            ),
            (
                path("color_16bit.png"),
                [0, 0, 0],
                [0, 1, 2],
                [73, 219, 182],
                [146, 182, 109],
            ),
            (path("gray_1bit.png"), 0, 0, 0, 255),
            (path("gray_2bit.png"), 0, 0, 85, 170),
            (path("gray_4bit.png"), 0, 0, 68, 153),
            (path("gray_8bit.png"), 0, 0, 73, 146),
            (path("gray_16bit.png"), 0, 0, 73, 146),
            (
                path("palette_1bit.png"),
                [127, 0, 255],
                [127, 0, 255],
                [127, 0, 255],
                [255, 0, 0],
            ),
            (
                path("palette_2bit.png"),
                [127, 0, 255],
                [127, 0, 255],
                [42, 220, 220],
                [212, 220, 127],
            ),
            (
                path("palette_4bit.png"),
                [127, 0, 255],
                [127, 0, 255],
                [8, 189, 232],
                [178, 242, 149],
            ),
            (
                path("palette_8bit.png"),
                [127, 0, 255],
                [127, 0, 255],
                [18, 199, 229],
                [164, 248, 158],
            ),
        ]

        for path, color_0_0, color_0_1, color_0_200, color_40_400 in images_info:
            img = image.Image(path)
            data = img.pixel_data
            ref_type = type(color_0_0)
            self.assertEqual(ref_type(data[0, 0]), color_0_0)
            self.assertEqual(ref_type(data[0, 1]), color_0_1)
            self.assertEqual(ref_type(data[0, 200]), color_0_200)
            self.assertEqual(ref_type(data[40, 400]), color_40_400)
