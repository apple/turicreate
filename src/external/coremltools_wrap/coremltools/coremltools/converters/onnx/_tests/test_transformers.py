from __future__ import absolute_import as _
from __future__ import division as _
from __future__ import print_function as _
from __future__ import unicode_literals as _

import pytest

onnx = pytest.importorskip("onnx")

import unittest
import numpy as np
import numpy.testing as npt  # type: ignore

from coremltools._deps import _HAS_ONNX, MSG_ONNX_NOT_FOUND

if _HAS_ONNX:
    import onnx
    from onnx import helper, numpy_helper, TensorProto

    from coremltools.converters.onnx import convert
    from coremltools.converters.onnx._graph import Graph
    from coremltools.converters.onnx._transformers import (
        ConvAddFuser,
        DropoutRemover,
        ImageScalerRemover,
    )
    from ._test_utils import (
        _onnx_create_model,
        _test_onnx_model,
        _conv_pool_output_size,
        _random_array,
    )


@unittest.skipUnless(_HAS_ONNX, MSG_ONNX_NOT_FOUND)
class ConvAddFuserTest(unittest.TestCase):
    def test_fuse_conv_without_bias(self):  # type: () -> None
        kernel_shape = (3, 2)
        strides = (2, 3)
        pads = (4, 2, 4, 2)
        dilations = (1, 2)
        group = 1
        weight = numpy_helper.from_array(_random_array((16, 3, 3, 2)), name="weight")

        input_shape = (1, 3, 224, 224)
        output_size = _conv_pool_output_size(
            input_shape, dilations, kernel_shape, pads, strides
        )

        output_shape = (1, int(weight.dims[0]), output_size[0], output_size[1])

        inputs = [("input0", input_shape)]
        outputs = [("output0", output_shape, TensorProto.FLOAT)]

        conv = helper.make_node(
            "Conv",
            inputs=[inputs[0][0], "weight"],
            outputs=["conv_output"],
            dilations=dilations,
            group=group,
            kernel_shape=kernel_shape,
            pads=pads,
            strides=strides,
        )

        b = _random_array((int(weight.dims[0]),))
        bias = numpy_helper.from_array(b, name="bias")

        add = helper.make_node(
            "Add",
            inputs=[conv.output[0], "bias"],
            outputs=[outputs[0][0]],
            broadcast=1,
            axis=1,
        )

        model = _onnx_create_model([conv, add], inputs, outputs, [weight, bias])
        graph_ = Graph.from_onnx(model.graph, onnx_ir_version=5)
        fused_graph = graph_.transformed([ConvAddFuser()])

        self.assertEqual(len(fused_graph.nodes), 1)
        node = fused_graph.nodes[0]
        self.assertEqual(len(node.inputs), 3)
        npt.assert_equal(node.input_tensors[node.inputs[2]], b)
        self.assertEqual(fused_graph.nodes[0].outputs[0], outputs[0][0])

    def test_fuse_conv_with_bias(self):  # type: () -> None
        kernel_shape = (3, 2)
        strides = (2, 3)
        pads = (4, 2, 4, 2)
        dilations = (1, 2)
        group = 1
        weight = numpy_helper.from_array(_random_array((16, 3, 3, 2)), name="weight")
        b = _random_array((int(weight.dims[0]),))
        bias = numpy_helper.from_array(b, name="bias")

        input_shape = (1, 3, 224, 224)
        output_size = _conv_pool_output_size(
            input_shape, dilations, kernel_shape, pads, strides
        )

        output_shape = (1, int(weight.dims[0]), output_size[0], output_size[1])

        inputs = [("input0", input_shape)]
        outputs = [("output0", output_shape, TensorProto.FLOAT)]

        conv = helper.make_node(
            "Conv",
            inputs=[inputs[0][0], "weight", "bias"],
            outputs=["conv_output"],
            dilations=dilations,
            group=group,
            kernel_shape=kernel_shape,
            pads=pads,
            strides=strides,
        )

        add = helper.make_node(
            "Add",
            inputs=[conv.output[0], "bias"],
            outputs=[outputs[0][0]],
            broadcast=1,
            axis=1,
        )

        model = _onnx_create_model([conv, add], inputs, outputs, [weight, bias])
        graph_ = Graph.from_onnx(model.graph, onnx_ir_version=5)
        fused_graph = graph_.transformed([ConvAddFuser()])

        self.assertEqual(len(fused_graph.nodes), 1)
        node = fused_graph.nodes[0]
        self.assertEqual(len(node.inputs), 3)
        npt.assert_equal(node.input_tensors[node.inputs[2]], b * 2)
        self.assertEqual(fused_graph.nodes[0].outputs[0], outputs[0][0])


@unittest.skipUnless(_HAS_ONNX, MSG_ONNX_NOT_FOUND)
class NodeRemoverTests(unittest.TestCase):
    def test_dropout_remover(self):  # type: () -> None
        inputs = [("input", (1, 3, 50, 50))]
        outputs = [("out", (1, 5, 50, 50), TensorProto.FLOAT)]
        weight = numpy_helper.from_array(_random_array((5, 3, 1, 1)), name="weight")
        conv = helper.make_node(
            "Conv",
            inputs=["input", "weight"],
            outputs=["conv_output"],
            kernel_shape=(1, 1),
            strides=(1, 1),
        )
        drop = helper.make_node(
            "Dropout", inputs=["conv_output"], outputs=["drop_output"],
        )
        exp = helper.make_node("Exp", inputs=["drop_output"], outputs=["out"])

        onnx_model = _onnx_create_model([conv, drop, exp], inputs, outputs)

        graph = Graph.from_onnx(onnx_model.graph, onnx_ir_version=5)
        new_graph = graph.transformed([DropoutRemover()])
        self.assertEqual(len(graph.nodes), 3)
        self.assertEqual(len(new_graph.nodes), 2)
        self.assertEqual(new_graph.nodes[0].inputs[0], "input")
        self.assertEqual(new_graph.nodes[1].inputs[0], new_graph.nodes[0].outputs[0])
        self.assertEqual(new_graph.nodes[1].outputs[0], "out")

    def test_image_scaler_remover(self):  # type: () -> None
        inputs = [("input", (1, 3, 50, 50))]
        outputs = [("out", (1, 3, 50, 50), TensorProto.FLOAT)]

        im_scaler = helper.make_node(
            "ImageScaler",
            inputs=["input"],
            outputs=["scaler_out"],
            bias=[10, -6, 20],
            scale=3.0,
        )

        exp = helper.make_node("Exp", inputs=["scaler_out"], outputs=["out"])

        onnx_model = _onnx_create_model([im_scaler, exp], inputs, outputs)

        graph = Graph.from_onnx(onnx_model.graph, onnx_ir_version=5)
        new_graph = graph.transformed([ImageScalerRemover()])
        self.assertEqual(len(graph.nodes), 2)
        self.assertEqual(len(new_graph.nodes), 1)
        self.assertEqual(new_graph.nodes[0].inputs[0], "input")
        self.assertEqual(new_graph.nodes[0].outputs[0], "out")

        coreml_model = convert(onnx_model)
        spec = coreml_model.get_spec()

        self.assertEqual(spec.neuralNetwork.preprocessing[0].scaler.channelScale, 3.0)
        self.assertEqual(spec.neuralNetwork.preprocessing[0].scaler.blueBias, 20.0)
        self.assertEqual(spec.neuralNetwork.preprocessing[0].scaler.greenBias, -6.0)
        self.assertEqual(spec.neuralNetwork.preprocessing[0].scaler.redBias, 10.0)

    def test_multiple_image_scaler(self):  # type : () -> None
        inputs = [("input_color", (1, 3, 10, 10)), ("input_gray", (1, 1, 10, 10))]
        outputs = [("out", (1, 4, 10, 10), TensorProto.FLOAT)]

        im_scaler1 = helper.make_node(
            "ImageScaler",
            inputs=["input_color"],
            outputs=["scaler_out_1"],
            bias=[10, -6, 20],
            scale=3.0,
        )

        im_scaler2 = helper.make_node(
            "ImageScaler",
            inputs=["input_gray"],
            outputs=["scaler_out_2"],
            bias=[-13],
            scale=5.0,
        )

        concat = helper.make_node(
            "Concat", inputs=["scaler_out_1", "scaler_out_2"], outputs=["out"], axis=1
        )

        onnx_model = _onnx_create_model(
            [im_scaler1, im_scaler2, concat], inputs, outputs
        )

        spec = convert(onnx_model).get_spec()
        self.assertEqual(len(spec.neuralNetwork.layers), 1)
        self.assertEqual(len(spec.neuralNetwork.preprocessing), 2)
        self.assertEqual(spec.neuralNetwork.preprocessing[0].scaler.channelScale, 3.0)
        self.assertEqual(spec.neuralNetwork.preprocessing[0].scaler.blueBias, 20.0)
        self.assertEqual(spec.neuralNetwork.preprocessing[0].scaler.greenBias, -6.0)
        self.assertEqual(spec.neuralNetwork.preprocessing[0].scaler.redBias, 10.0)
        self.assertEqual(spec.neuralNetwork.preprocessing[1].scaler.channelScale, 5.0)
        self.assertEqual(spec.neuralNetwork.preprocessing[1].scaler.grayBias, -13.0)


@unittest.skipUnless(_HAS_ONNX, MSG_ONNX_NOT_FOUND)
class PixelShuffleFuserTest(unittest.TestCase):
    def test_pixel_shuffle(self):  # type: () -> None
        scale_factor = 2
        input_shape = (1, 8, 2, 2)
        output_shape = (
            input_shape[0],
            int(input_shape[1] / (scale_factor ** 2)),
            input_shape[2] * scale_factor,
            input_shape[3] * scale_factor,
        )

        inputs = [("input0", input_shape)]
        outputs = [("output0", output_shape, TensorProto.FLOAT)]

        shape1 = [
            output_shape[0],
            output_shape[1],
            scale_factor,
            scale_factor,
            input_shape[2],
            input_shape[3],
        ]

        shape1 = numpy_helper.from_array(np.asarray(shape1), name="shape1")
        shape2 = numpy_helper.from_array(np.asarray(list(output_shape)), name="shape2")

        node_0 = helper.make_node(
            "Reshape", inputs=[inputs[0][0], "shape1"], outputs=["node0"],
        )
        node_1 = helper.make_node(
            "Transpose", inputs=["node0"], outputs=["node1"], perm=[0, 1, 4, 2, 5, 3]
        )
        node_2 = helper.make_node(
            "Reshape", inputs=["node1", "shape2"], outputs=[outputs[0][0]],
        )
        model = _onnx_create_model(
            [node_0, node_1, node_2], inputs, outputs, initializer=[shape1, shape2]
        )
        _test_onnx_model(model, decimal=7)
