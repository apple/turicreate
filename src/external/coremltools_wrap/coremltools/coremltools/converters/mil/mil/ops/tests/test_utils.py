#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import numpy as np

from coremltools.converters.mil.mil.ops.defs._utils import (
    aggregated_pad,
    effective_kernel,
    spatial_dimensions_out_shape,
)


class TestDilation:
    def test_kernel_and_dilations_not_same_size(self):
        np.testing.assert_raises_regex(
            ValueError,
            "kernel_shape.*dilations.*length",
            effective_kernel,
            kernel_shape=(1, 2, 3),
            dilations=(1, 2),
        )

    def test_effective_kernel_dilation_1(self):
        actual = effective_kernel(kernel_shape=(1, 2, 3), dilations=(1, 1, 1))

        expected = [1, 2, 3]
        np.testing.assert_equal(actual, expected)

    def test_effective_kernel_dilation_2(self):
        actual = effective_kernel(kernel_shape=(1, 2, 3), dilations=(2, 2, 2))

        expected = [1, 3, 5]
        np.testing.assert_equal(actual, expected)

    def test_effective_kernel_dilation_3(self):
        actual = effective_kernel(kernel_shape=(1, 2, 3), dilations=(3, 3, 3))

        expected = [1, 4, 7]
        np.testing.assert_equal(actual, expected)


class TestAggregatePadding:
    def test_invalid_pad_type(self):
        np.testing.assert_raises_regex(
            ValueError,
            "Invalid padding pad_type",
            aggregated_pad,
            pad_type="bananas",
            kernel_shape=(1, 2, 3),
        )

    def test_dilations_rank_different_from_input_rank(self):
        np.testing.assert_raises_regex(
            ValueError,
            "dilations must have same length as kernel_shape",
            aggregated_pad,
            pad_type="valid",  # doesn't matter
            kernel_shape=(1, 2, 3),
            dilations=(4, 5),
        )

    def test_custom_pad(self):
        actual = aggregated_pad(
            pad_type="custom", kernel_shape=(1, 2, 3), custom_pad=(7, 8, 9, 10, 11, 12)
        )

        expected = [7 + 8, 9 + 10, 11 + 12]
        np.testing.assert_equal(actual, expected)

    def test_custom_pad_none(self):
        np.testing.assert_raises_regex(
            ValueError,
            "Invalid custom_pad",
            aggregated_pad,
            pad_type="custom",
            kernel_shape=(1, 2, 3),  # doesn't matter
            custom_pad=None,
        )

    def test_custom_pad_invalid(self):
        np.testing.assert_raises_regex(
            ValueError,
            "Invalid custom_pad",
            aggregated_pad,
            pad_type="custom",
            kernel_shape=(1, 2, 3),  # doesn't matter
            custom_pad=(7, 8, 9, 10),  # too few elements
        )

    def test_valid_pad(self):
        actual = aggregated_pad(pad_type="valid", kernel_shape=(1, 2, 3),)

        expected = [0, 0, 0]
        np.testing.assert_equal(actual, expected)

    def test_valid_pad_4d(self):
        actual = aggregated_pad(pad_type="valid", kernel_shape=(1, 2, 3, 4),)

        expected = [0, 0, 0, 0]
        np.testing.assert_equal(actual, expected)

    def test_valid_pad_2d(self):
        actual = aggregated_pad(pad_type="valid", kernel_shape=(1, 2),)

        expected = [0, 0]
        np.testing.assert_equal(actual, expected)

    def test_valid_pad_1d(self):
        actual = aggregated_pad(pad_type="valid", kernel_shape=[4])

        expected = [0]
        np.testing.assert_equal(actual, expected)

    def test_same_padding_no_dilation(self):
        actual = aggregated_pad(
            pad_type="same",
            input_shape=(5, 6, 7),
            kernel_shape=(2, 2, 2),
            strides=(1, 2, 2),
        )

        expected = [1, 0, 1]
        np.testing.assert_equal(actual, expected)

    def test_same_padding_dilation_with_dilation(self):
        actual = aggregated_pad(
            pad_type="same",
            input_shape=(19, 20, 21),
            kernel_shape=(2, 2, 2),
            strides=(1, 2, 2),
            dilations=(5, 6, 7),
        )

        expected = [5, 5, 7]
        np.testing.assert_equal(actual, expected)

    def test_same_padding_stride_same_as_input(self):
        actual = aggregated_pad(
            pad_type="same", input_shape=(5, 5), kernel_shape=(3, 3), strides=(5, 5),
        )

        expected = [0, 0]
        np.testing.assert_equal(actual, expected)

    def test_same_padding_stride_larger_than_kernel_but_less_than_input(self):
        actual = aggregated_pad(
            pad_type="same", input_shape=(5, 5), kernel_shape=(3, 3), strides=(4, 4),
        )

        expected = [2, 2]
        np.testing.assert_equal(actual, expected)

    def test_same_padding_none_input_shape(self):
        np.testing.assert_raises_regex(
            ValueError,
            "input_shape.*None",
            aggregated_pad,
            pad_type="same",
            kernel_shape=(1, 2, 3),
            strides=(1, 2, 3),
        )

    def test_same_padding_input_shape_wrong_size(self):
        np.testing.assert_raises_regex(
            ValueError,
            "input_shape.*same length",
            aggregated_pad,
            pad_type="same",
            kernel_shape=(1, 2, 3),
            input_shape=(1, 2),
            strides=(1, 2, 3),
        )

    def test_same_padding_none_strides(self):
        np.testing.assert_raises_regex(
            ValueError,
            "strides.*None",
            aggregated_pad,
            pad_type="same",
            kernel_shape=(1, 2, 3),
            input_shape=(1, 2, 3),
        )

    def test_same_padding_strides_wrong_size(self):
        np.testing.assert_raises_regex(
            ValueError,
            "strides.*same length",
            aggregated_pad,
            pad_type="same",
            kernel_shape=(1, 2, 3),
            input_shape=(1, 2, 3),
            strides=(1, 2),
        )


class TestOutputShape:
    def test_custom_padding_shape(self):
        actual = spatial_dimensions_out_shape(
            pad_type="custom",
            input_shape=(3, 3, 3),
            kernel_shape=(2, 2, 2),
            strides=(2, 2, 2),
            custom_pad=(2, 0, 1, 2, 2, 3),
        )

        expected = [2, 3, 4]
        np.testing.assert_equal(actual, expected)

    def test_valid_padding_shape(self):
        actual = spatial_dimensions_out_shape(
            pad_type="valid", input_shape=(7, 7), kernel_shape=(3, 3), strides=(1, 1)
        )

        expected = [5, 5]
        np.testing.assert_equal(actual, expected)

    def test_valid_padding_shape_dilation_2(self):
        actual = spatial_dimensions_out_shape(
            pad_type="valid",
            input_shape=(7, 7),
            kernel_shape=(3, 3),
            strides=(1, 1),
            dilations=(2, 2),
        )

        expected = [3, 3]
        np.testing.assert_equal(actual, expected)

    def test_valid_padding_shape_with_stride_2(self):
        actual = spatial_dimensions_out_shape(
            pad_type="valid", input_shape=(7, 7), kernel_shape=(3, 3), strides=(2, 2)
        )

        expected = [3, 3]
        np.testing.assert_equal(actual, expected)

    def test_same_padding_shape(self):
        actual = spatial_dimensions_out_shape(
            pad_type="same", input_shape=(6, 6), kernel_shape=(2, 2), strides=(2, 2)
        )

        expected = [3, 3]
        np.testing.assert_equal(actual, expected)

    def test_same_padding_shape_stride_2_input_not_multiple_of_kernel(self):
        actual = spatial_dimensions_out_shape(
            pad_type="same", input_shape=(5, 5), kernel_shape=(2, 2), strides=(2, 2)
        )

        expected = [3, 3]
        np.testing.assert_equal(actual, expected)

    def test_same_padding_shape_dilation_2(self):
        actual = spatial_dimensions_out_shape(
            pad_type="same",
            input_shape=(5, 5),
            kernel_shape=(2, 2),
            strides=(1, 1),
            dilations=(2, 2),
        )

        expected = [5, 5]
        np.testing.assert_equal(actual, expected)
