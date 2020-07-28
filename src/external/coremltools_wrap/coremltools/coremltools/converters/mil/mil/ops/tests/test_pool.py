#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.testing_reqs import *

from .testing_utils import run_compare_builder

backends = testing_reqs.backends


class TestAvgPool:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array(
            [
                [
                    [[-10.80291205, -6.42076184], [-7.07910997, 9.1913279]],
                    [[-3.18181497, 0.9132147], [11.9785544, 7.92449539]],
                ]
            ],
            dtype=np.float32,
        )
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.avg_pool(x=x, kernel_sizes=[1, 2], strides=[2, 1], pad_type="valid"),
                mb.avg_pool(
                    x=x,
                    kernel_sizes=[2, 1],
                    strides=[1, 2],
                    pad_type="same",
                    exclude_padding_from_average=True,
                ),
            ]

        expected_output_types = [(1, 2, 1, 1, types.fp32), (1, 2, 2, 1, types.fp32)]
        expected_outputs = [
            np.array([[[[-8.611837]], [[-1.1343001]]]], dtype=np.float32),
            np.array(
                [[[[-8.94101143], [-7.07911015]], [[4.39836979], [11.97855473]]]],
                dtype=np.float32,
            ),
        ]

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestL2Pool:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array(
            [[[[-10.0, -6.0], [-7.0, 9.0]], [[-3.0, 0.0], [11.0, 7.0]]]],
            dtype=np.float32,
        )
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.l2_pool(x=x, kernel_sizes=[1, 2], strides=[2, 1], pad_type="valid"),
            ]

        expected_output_types = [(1, 2, 1, 1, types.fp32)]
        expected_outputs = [np.array([[[[11.66190338]], [[3.0]]]], dtype=np.float32)]

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )


class TestMaxPool:
    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends,)
    )
    def test_builder_to_backend_smoke(self, use_cpu_only, backend):
        x_val = np.array(
            [
                [
                    [[-10.80291205, -6.42076184], [-7.07910997, 9.1913279]],
                    [[-3.18181497, 0.9132147], [11.9785544, 7.92449539]],
                ]
            ],
            dtype=np.float32,
        )
        input_placeholders = {"x": mb.placeholder(shape=x_val.shape)}
        input_values = {"x": x_val}

        def build(x):
            return [
                mb.max_pool(x=x, kernel_sizes=[1, 2], strides=[2, 1], pad_type="valid")
            ]

        expected_output_types = [(1, 2, 1, 1, types.fp32)]
        expected_outputs = [
            np.array([[[[-6.42076184]], [[0.9132147]]]], dtype=np.float32)
        ]

        run_compare_builder(
            build,
            input_placeholders,
            input_values,
            expected_output_types,
            expected_outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )
