#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import sys

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.testing_reqs import *
from .testing_utils import *

backends = testing_reqs.backends

torch = pytest.importorskip("torch")
pytestmark = pytest.mark.skipif(sys.version_info >= (3, 8), reason="Segfault with Python 3.8+") #rdar://problem/65730375

class TestBatchNorm:
    @pytest.mark.parametrize(
        "num_features, eps, backend", itertools.product([5, 3, 2, 1], [0.1, 1e-05, 1e-9], backends),
    )
    def test_batchnorm(self, num_features, eps, backend):
        model = nn.BatchNorm2d(num_features, eps)
        run_compare_torch((1, num_features, 5, 5), model, backend=backend)


class TestLinear:
    @pytest.mark.parametrize(
        "in_features, out_features, backend", itertools.product([10, 25, 100], [3, 6], backends),
    )
    def test_addmm(self, in_features, out_features, backend):
        model = nn.Linear(in_features, out_features)
        run_compare_torch((1, in_features), model, backend=backend)


class TestConv:
    @pytest.mark.parametrize(
        "height, width, in_channels, out_channels, kernel_size, stride, padding, dilation, backend",
        itertools.product(
            [5, 6], [5, 7], [1, 3], [1, 3], [1, 3], [1, 3], [1, 3], [1, 3], backends
        ),
    )
    def test_convolution2d(
            self,
            height,
            width,
            in_channels,
            out_channels,
            kernel_size,
            stride,
            padding,
            dilation,
            backend,
            groups=1,
    ):
        model = nn.Conv2d(
            in_channels=in_channels,
            out_channels=out_channels,
            kernel_size=kernel_size,
            stride=stride,
            padding=padding,
            dilation=dilation,
        )
        run_compare_torch((1, in_channels, height, width), model, backend=backend)

    @pytest.mark.parametrize(
        "height, width, in_channels, out_channels, kernel_size, stride, padding, dilation, backend",
        itertools.product(
            [5, 6], [5, 7], [1, 3], [1, 3], [1, 3], [2, 3], [0, 1], [1, 3], backends
        ),
    )
    def test_convolution_transpose2d(
            self,
            height,
            width,
            in_channels,
            out_channels,
            kernel_size,
            stride,
            padding,
            dilation,
            backend,
            groups=1,
    ):
        model = nn.ConvTranspose2d(
            in_channels=in_channels,
            out_channels=out_channels,
            kernel_size=kernel_size,
            stride=stride,
            padding=padding,
            dilation=dilation,
        )
        run_compare_torch((1, in_channels, height, width), model, backend=backend)

    # TODO: rdar://65588783 ([PyTorch] Define and error out on unsupported configuration for output_padding)
    @pytest.mark.parametrize(
        "height, width, in_channels, out_channels, kernel_size, stride, padding, dilation, output_padding, backend",
        list(itertools.product(
            [10], [10], [1, 3], [1, 3], [1, 3], [1, 2, 3], [1, 3], [1, 2], [1, 2, (1, 2)], backends
        )) + [pytest.param(5, 5, 1, 1, 3, 4, 1, 1, 2, "nn_proto", marks=pytest.mark.xfail),
            pytest.param(5, 5, 1, 1, 3, 2, 1, 3, 2, "nn_proto", marks=pytest.mark.xfail)],
    )
    def test_convolution_transpose2d_output_padding(
        self,
        height,
        width,
        in_channels,
        out_channels,
        kernel_size,
        stride,
        padding,
        dilation,
        output_padding,
        backend,
        groups=1,
    ):

        # Output padding must be less than either stride or dilation
        # Skip testing invalid combinations
        if isinstance(output_padding, int):
            if output_padding >= stride and output_padding >= dilation:
                return
        elif isinstance(output_padding, tuple):
            for _output_padding in output_padding:
                if _output_padding >= stride and _output_padding >= dilation:
                    return

        model = nn.ConvTranspose2d(
            in_channels=in_channels,
            out_channels=out_channels,
            kernel_size=kernel_size,
            stride=stride,
            padding=padding,
            dilation=dilation,
            output_padding=output_padding
        )
        run_compare_torch((1, in_channels, height, width), model, backend=backend)


class TestLoop:
    @pytest.mark.parametrize("backend", backends)
    def test_for_loop(self, backend):
        class TestLayer(nn.Module):
            def __init__(self):
                super(TestLayer, self).__init__()

            def forward(self, x):
                x = 2.0 * x
                return x

        class TestNet(nn.Module):
            input_size = (64,)

            def __init__(self):
                super(TestNet, self).__init__()
                layer = TestLayer()
                self.layer = torch.jit.trace(layer, torch.rand(self.input_size))

            def forward(self, x):
                for _ in range(7):
                    x = self.layer(x)
                return x

        model = TestNet().eval()
        torch_model = torch.jit.script(model)

        run_compare_torch(model.input_size, torch_model, backend=backend)

    @pytest.mark.parametrize("backend", backends)
    def test_while_loop(self, backend):
        class TestLayer(nn.Module):
            def __init__(self):
                super(TestLayer, self).__init__()

            def forward(self, x):
                x = 0.5 * x
                return x

        class TestNet(nn.Module):
            input_size = (1,)

            def __init__(self):
                super(TestNet, self).__init__()
                layer = TestLayer()
                self.layer = torch.jit.trace(layer, torch.rand(self.input_size))

            def forward(self, x):
                while x > 0.01:
                    x = self.layer(x)
                return x

        model = TestNet().eval()
        torch_model = torch.jit.script(model)

        run_compare_torch(model.input_size, torch_model, backend=backend)


class TestUpsample:
    @pytest.mark.parametrize(
        "output_size, align_corners, backend",
        [
            x
            for x in itertools.product(
                [(10, 10), (1, 1), (20, 20), (2, 3), (190, 170)], [True, False], backends
            )
        ],
    )
    def test_upsample_bilinear2d_with_output_size(self, output_size, align_corners, backend):
        input_shape = (1, 3, 10, 10)
        model = ModuleWrapper(
            nn.functional.interpolate,
            {"size": output_size, "mode": "bilinear", "align_corners": align_corners,},
        )
        run_compare_torch(input_shape, model, backend=backend)

    @pytest.mark.parametrize(
        "scales_h, scales_w, align_corners, backend",
        [x for x in itertools.product([2, 3, 4.5], [4, 5, 5.5], [True, False], backends)],
    )
    def test_upsample_bilinear2d_with_scales(self, scales_h, scales_w, align_corners, backend):
        input_shape = (1, 3, 10, 10)
        model = ModuleWrapper(
            nn.functional.interpolate,
            {
                "scale_factor": (scales_h, scales_w),
                "mode": "bilinear",
                "align_corners": align_corners,
            }
        )
        run_compare_torch(input_shape, model, backend=backend)

    @pytest.mark.parametrize(
        "output_size, backend",
        [
            x
            for x in itertools.product(
                [(10, 10), (30, 20), (20, 20), (20, 30), (190, 170)], backends
            )
        ],
    )
    def test_upsample_nearest2d_with_output_size(self, output_size, backend):
        input_shape = (1, 3, 10, 10)
        model = ModuleWrapper(
            nn.functional.interpolate,
            {"size": output_size, "mode": "nearest"},
        )
        run_compare_torch(input_shape, model, backend=backend)

    @pytest.mark.parametrize(
        "scales_h, scales_w, backend",
        [x for x in itertools.product([2, 3, 5], [4, 5, 2], backends)],
    )
    def test_upsample_nearest2d_with_scales(self, scales_h, scales_w, backend):
        input_shape = (1, 3, 10, 10)
        model = ModuleWrapper(
            nn.functional.interpolate,
            {
                "scale_factor": (scales_h, scales_w),
                "mode": "nearest",
            },
        )
        run_compare_torch(input_shape, model, backend=backend)


class TestBranch:
    @pytest.mark.parametrize("backend", backends)
    def test_if(self, backend):
        class TestLayer(nn.Module):
            def __init__(self):
                super(TestLayer, self).__init__()

            def forward(self, x):
                x = torch.mean(x)
                return x

        class TestNet(nn.Module):
            input_size = (64,)

            def __init__(self):
                super(TestNet, self).__init__()
                layer = TestLayer()
                self.layer = torch.jit.trace(layer, torch.rand(self.input_size))

            def forward(self, x):
                m = self.layer(x)
                if m < 0:
                    scale = -2.0
                else:
                    scale = 2.0
                x = scale * x
                return x

        model = TestNet().eval()
        torch_model = torch.jit.script(model)

        run_compare_torch(model.input_size, torch_model, backend=backend)


class TestAvgPool:
    @pytest.mark.xfail(reason="rdar://problem/61064173")
    @pytest.mark.parametrize(
        "input_shape, kernel_size, stride, pad, include_pad, backend",
        itertools.product(
            [(1, 3, 15), (1, 1, 7), (1, 3, 10)],
            [1, 2, 3],
            [1, 2],
            [0, 1],
            [True, False],
            backends,
        ),
    )
    def test_avg_pool1d(self, input_shape, kernel_size, stride, pad, include_pad, backend):
        if pad > kernel_size / 2:
            # Because this test is xfail, we have to fail rather than
            # just return here, otherwise these test cases unexpectedly pass.
            # This can be changed to `return` once the above radar
            # is fixed and the test is no longer xfail.
            raise ValueError("pad must be less than half the kernel size")
        model = nn.AvgPool1d(kernel_size, stride, pad, False, include_pad)
        run_compare_torch(input_shape, model, backend=backend)

    @pytest.mark.parametrize(
        "input_shape, kernel_size, stride, pad, include_pad, backend",
        itertools.product(
            [(1, 3, 15, 15), (1, 1, 7, 7), (1, 3, 10, 10)],
            [1, 2, 3],
            [1, 2],
            [0, 1],
            [True, False],
            backends,
        ),
    )
    def test_avg_pool2d(self, input_shape, kernel_size, stride, pad, include_pad, backend):
        if pad > kernel_size / 2:
            return
        model = nn.AvgPool2d(kernel_size, stride, pad, False, include_pad)
        run_compare_torch(input_shape, model, backend=backend)

    @pytest.mark.parametrize(
        "input_shape, kernel_size, stride, pad, include_pad, backend",
        itertools.product(
            [(1, 3, 15, 15), (1, 1, 7, 7), (1, 3, 10, 10)],
            [3],
            [1, 2],
            [0, 1],
            [True, False],
            backends,
        ),
    )
    def test_avg_pool2d_ceil_mode(
        self, input_shape, kernel_size, stride, pad, include_pad, backend
    ):
        if pad > kernel_size / 2:
            return
        model = nn.AvgPool2d(kernel_size, stride, pad, True, include_pad)
        run_compare_torch(input_shape, model, backend=backend)


class TestMaxPool:
    @pytest.mark.xfail(
        reason="PyTorch convert function for op max_pool1d not implemented, "
        "we will also likely run into rdar://problem/61064173"
    )
    @pytest.mark.parametrize(
        "input_shape, kernel_size, stride, pad, backend",
        itertools.product(
            [(1, 3, 15), (1, 1, 7), (1, 3, 10)], [1, 2, 3], [1, 2], [0, 1], backends
        ),
    )
    def test_max_pool1d(self, input_shape, kernel_size, stride, pad, backend):
        if pad > kernel_size / 2:
            # Because this test is xfail, we have to fail rather than
            # just return here, otherwise these test cases unexpectedly pass.
            # This can be changed to `return` once the above radar
            # is fixed and the test is no longer xfail.
            raise ValueError("pad must be less than half the kernel size")
        model = nn.MaxPool1d(kernel_size, stride, pad, ceil_mode=False)
        run_compare_torch(input_shape, model, backend=backend)

    @pytest.mark.parametrize(
        "input_shape, kernel_size, stride, pad, backend",
        itertools.product(
            [(1, 3, 15, 15), (1, 1, 7, 7), (1, 3, 10, 10)], [1, 2, 3], [1, 2], [0, 1], backends,
        ),
    )
    def test_max_pool2d(self, input_shape, kernel_size, stride, pad, backend):
        if pad > kernel_size / 2:
            return
        model = nn.MaxPool2d(kernel_size, stride, pad, ceil_mode=False)
        run_compare_torch(input_shape, model, backend=backend)

    @pytest.mark.parametrize(
        "input_shape, kernel_size, stride, pad, backend",
        itertools.product(
            [(1, 3, 15, 15), (1, 1, 7, 7), (1, 3, 10, 10)], [3], [1, 2], [0, 1], backends,
        ),
    )
    def test_max_pool2d_ceil_mode(self, input_shape, kernel_size, stride, pad, backend):
        if pad > kernel_size / 2:
            return
        model = nn.MaxPool2d(kernel_size, stride, pad, ceil_mode=True)
        run_compare_torch(input_shape, model, backend=backend)


class TestLSTM:

    def _pytorch_hidden_to_coreml(self, x):
        # Split of Direction axis
        f, b = torch.split(x, [1] * x.shape[0], dim=0)
        # Concat on Hidden Size axis
        x = torch.cat((f, b), dim=2)
        # NOTE:
        # We are ommiting a squeeze because the conversion
        # function for the mil op lstm unsqueezes the num_layers
        # dimension
        return x

    @pytest.mark.parametrize(
        "input_size, hidden_size, num_layers, bias, batch_first, dropout, bidirectional, backend",
        itertools.product([7], [5], [1], [True, False], [False], [0.3], [True, False], backends),
    )
    def test_lstm(
        self,
        input_size,
        hidden_size,
        num_layers,
        bias,
        batch_first,
        dropout,
        bidirectional,
        backend,
    ):
        model = nn.LSTM(
            input_size=input_size,
            hidden_size=hidden_size,
            num_layers=num_layers,
            bias=bias,
            batch_first=batch_first,
            dropout=dropout,
            bidirectional=bidirectional,
        )
        SEQUENCE_LENGTH = 3
        BATCH_SIZE = 2

        num_directions = int(bidirectional) + 1

        # (seq_len, batch, input_size)
        if batch_first:
            _input = torch.rand(BATCH_SIZE, SEQUENCE_LENGTH, input_size)
        else:
            _input = torch.randn(SEQUENCE_LENGTH, BATCH_SIZE, input_size)

        h0 = torch.randn(num_layers * num_directions, BATCH_SIZE, hidden_size)
        c0 = torch.randn(num_layers * num_directions, BATCH_SIZE, hidden_size)

        inputs = (_input, (h0, c0))
        expected_results = model(*inputs)
        # Need to do some output reshaping if bidirectional
        if bidirectional:
            ex_hn = self._pytorch_hidden_to_coreml(expected_results[1][0])
            ex_cn = self._pytorch_hidden_to_coreml(expected_results[1][1])
            expected_results = (expected_results[0], (ex_hn, ex_cn))
        run_compare_torch(inputs, model, expected_results, input_as_shape=False, backend=backend)

    @pytest.mark.parametrize(
        "input_size, hidden_size, num_layers, bias, batch_first, dropout, bidirectional, backend",
        [
            (7, 3, 2, True, True, 0.3, True, list(backends)[-1]),
            (7, 3, 2, False, False, 0.3, False, list(backends)[0]),
        ],
    )
    def test_lstm_xexception(
        self,
        input_size,
        hidden_size,
        num_layers,
        bias,
        batch_first,
        dropout,
        bidirectional,
        backend,
    ):
        with pytest.raises(ValueError):
            self.test_lstm(
                input_size,
                hidden_size,
                num_layers,
                bias,
                batch_first,
                dropout,
                bidirectional,
                backend= backend,
            )


class TestConcat:
    # This tests an edge case where the list of tensors to concatenate only
    # has one item. NN throws an error for this case, hence why we have to
    # run through the full conversion process to test it.
    @pytest.mark.parametrize("backend", backends)
    def test_cat(self, backend):
        class TestNet(nn.Module):
            def __init__(self):
                super(TestNet, self).__init__()

            def forward(self, x):
                x = torch.cat((x,), axis=1)
                return x

        model = TestNet()
        run_compare_torch((1, 3, 16, 16), model, backend=backend)

class TestReduction:
    @pytest.mark.parametrize(
        "input_shape, dim, keepdim, backend",
        itertools.product([(2, 2), (1, 1)], [0, 1], [True, False], backends),
    )
    def test_max(self, input_shape, dim, keepdim, backend):
        class TestMax(nn.Module):
            def __init__(self):
                super(TestMax, self).__init__()

            def forward(self, x):
                return torch.max(x, dim=dim, keepdim=keepdim)

        input_data = torch.rand(input_shape)
        model = TestMax()
        # TODO: Expected results are flipped due to naming issue:
        # rdar://62681982 (Determine the output names of MLModels)
        expected_results = model(input_data)[::-1]
        run_compare_torch(
            input_data, model, expected_results=expected_results, input_as_shape=False, backend=backend
        )

class TestLayerNorm:
    @pytest.mark.parametrize(
        "input_shape, eps, backend",
        itertools.product([(1, 3, 15, 15), (1, 1, 1, 1)], [1e-5, 1e-9], backends),
    )
    def test_layer_norm(self, input_shape, eps, backend):
        model = nn.LayerNorm(input_shape, eps=eps)
        run_compare_torch(input_shape, model, backend=backend)


class TestPixelShuffle:

    @pytest.mark.parametrize(
        "batch_size, CHW, r, backend",
        itertools.product(
            [1, 3],
            [(1, 4, 4), (3, 2, 3)],
            [2, 4],
            backends
        )
    )
    def test_pixel_shuffle(self, batch_size, CHW, r, backend):
        C, H, W = CHW
        input_shape = (batch_size, C*r*r, H, W)
        model = nn.PixelShuffle(upscale_factor=r)
        run_compare_torch(input_shape, model, backend=backend)


class TestElementWiseUnary:
    @pytest.mark.parametrize(
        "use_cpu_only, backend, rank, mode",
        itertools.product(
            [True, False],
            backends,
            [rank for rank in range(1, 6)],
            [
                "sinh",
            ],
        ),
    )
    def test_unary(self, use_cpu_only, backend, rank, mode):
        input_shape = np.random.randint(low=2, high=6, size=rank)
        input_shape = tuple(input_shape)
        if mode == "sinh":
            operation = torch.sinh

        model = ModuleWrapper(function=operation)
        run_compare_torch(input_shape, model, backend=backend)
