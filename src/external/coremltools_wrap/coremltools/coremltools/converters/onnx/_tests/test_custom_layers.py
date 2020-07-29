from __future__ import absolute_import as _
from __future__ import division as _
from __future__ import print_function as _

import unittest

from coremltools._deps import _HAS_ONNX, MSG_ONNX_NOT_FOUND

if _HAS_ONNX:
    import onnx
    from ._test_utils import _onnx_create_model
    from onnx import helper, ModelProto, TensorProto
    from coremltools.converters.onnx import convert
from coremltools.proto import NeuralNetwork_pb2  # type: ignore


def _make_model_acos_exp_topk():  # type: (...) -> ModelProto
    """
  make a very simple model for testing: input->clip->exp->topk->2 outputs
  """
    inputs = [("input0", (10,), TensorProto.FLOAT), ("K", (1,), TensorProto.INT64)]
    outputs = [
        ("output_values", (3,), TensorProto.FLOAT),
        ("output_indices", (3,), TensorProto.INT64),
    ]
    acos = helper.make_node("Acos", inputs=[inputs[0][0]], outputs=["acos_out"])
    exp = helper.make_node("Exp", inputs=[acos.output[0]], outputs=["exp_out"])
    topk = helper.make_node(
        "TopK",
        inputs=[exp.output[0], inputs[1][0]],
        outputs=[outputs[0][0], outputs[1][0]],
        axis=0,
    )
    return _onnx_create_model([acos, exp, topk], inputs, outputs)


def _make_model_flatten_axis3():  # type: (...) -> ModelProto
    """
  make a simple model: 4-D input -> flatten (axis=3)-> output
  """
    inputs = [("input", (1, 3, 10, 20), TensorProto.FLOAT)]
    outputs = [("output", (30, 20), TensorProto.FLOAT)]
    flatten = helper.make_node(
        "Flatten", inputs=[inputs[0][0]], outputs=[outputs[0][0]], axis=3
    )
    return _onnx_create_model([flatten], inputs, outputs)


@unittest.skipUnless(_HAS_ONNX, MSG_ONNX_NOT_FOUND)
class CustomLayerTest(unittest.TestCase):
    def test_unsupported_ops(self):  # type: () -> None

        onnx_model = _make_model_acos_exp_topk()
        coreml_model = convert(onnx_model, add_custom_layers=True)

        spec = coreml_model.get_spec()
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[0].custom)
        self.assertIsNotNone(layers[2].custom)
        self.assertEqual("Acos", layers[0].custom.className)
        self.assertEqual("TopK", layers[2].custom.className)

    def test_unsupported_ops_provide_functions(self):  # type: () -> None
        def convert_acos(builder, node, graph, err):
            params = NeuralNetwork_pb2.CustomLayerParams()
            params.className = node.op_type
            params.description = "Custom layer that corresponds to the ONNX op {}".format(
                node.op_type,
            )

            builder.add_custom(
                name=node.name,
                input_names=node.inputs,
                output_names=node.outputs,
                custom_proto_spec=params,
            )

        def convert_topk(builder, node, graph, err):
            params = NeuralNetwork_pb2.CustomLayerParams()
            params.className = node.op_type
            params.description = "Custom layer that corresponds to the ONNX op {}".format(
                node.op_type,
            )
            params.parameters["axis"].intValue = node.attrs.get("axis", -1)

            builder.add_custom(
                name=node.name,
                input_names=node.inputs,
                output_names=node.outputs,
                custom_proto_spec=params,
            )

        onnx_model = _make_model_acos_exp_topk()
        coreml_model = convert(
            model=onnx_model,
            add_custom_layers=True,
            custom_conversion_functions={"Acos": convert_acos, "TopK": convert_topk},
        )

        spec = coreml_model.get_spec()
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[0].custom)
        self.assertIsNotNone(layers[2].custom)
        self.assertEqual("Acos", layers[0].custom.className)
        self.assertEqual("TopK", layers[2].custom.className)
        self.assertEqual(0, layers[2].custom.parameters["axis"].intValue)

    def test_node_name_type_custom_functions(self):  # type: () -> None
        def convert_acos(builder, node, graph, err):
            params = NeuralNetwork_pb2.CustomLayerParams()
            params.className = node.op_type
            params.description = "Custom layer that corresponds to the ONNX op {}".format(
                node.op_type,
            )

            builder.add_custom(
                name=node.name,
                input_names=node.inputs,
                output_names=node.outputs,
                custom_proto_spec=params,
            )

        def convert_topk_generic(builder, node, graph, err):
            params = NeuralNetwork_pb2.CustomLayerParams()
            params.className = node.op_type
            params.description = "Custom layer that corresponds to the ONNX op {}".format(
                node.op_type,
            )
            params.parameters["axis"].intValue = node.attrs.get("axis", -1)
            params.parameters["k"].intValue = node.attrs["k"]

            builder.add_custom(
                name=node.name,
                input_names=node.inputs,
                output_names=node.outputs,
                custom_proto_spec=params,
            )

        def convert_topk_node_specific(builder, node, graph, err):
            params = NeuralNetwork_pb2.CustomLayerParams()
            params.className = node.op_type
            params.description = "Custom layer that corresponds to the ONNX op {}".format(
                node.op_type,
            )
            params.parameters["axis"].intValue = node.attrs.get("axis", -1)

            builder.add_custom(
                name=node.name,
                input_names=node.inputs,
                output_names=node.outputs,
                custom_proto_spec=params,
            )

        onnx_model = _make_model_acos_exp_topk()
        coreml_model = convert(
            model=onnx_model,
            add_custom_layers=True,
            custom_conversion_functions={
                "Acos": convert_acos,
                "TopK": convert_topk_generic,
                "output_values_output_indices": convert_topk_node_specific,
            },
        )

        spec = coreml_model.get_spec()
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[0].custom)
        self.assertIsNotNone(layers[2].custom)
        self.assertEqual("Acos", layers[0].custom.className)
        self.assertEqual("TopK", layers[2].custom.className)
        self.assertEqual(0, layers[2].custom.parameters["axis"].intValue)

    def test_unsupported_op_attribute(self):  # type: () -> None
        onnx_model = _make_model_flatten_axis3()
        coreml_model = convert(onnx_model, add_custom_layers=True)

        spec = coreml_model.get_spec()
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[0].custom)
        self.assertEqual("Flatten", layers[0].custom.className)

    def test_unsupported_op_attribute_provide_functions(self):  # type: () -> None
        def convert_flatten(builder, node, graph, err):
            params = NeuralNetwork_pb2.CustomLayerParams()
            params.className = node.op_type
            params.description = "Custom layer that corresponds to the ONNX op {}".format(
                node.op_type,
            )
            params.parameters["axis"].intValue = node.attrs["axis"]

            builder.add_custom(
                name=node.name,
                input_names=node.inputs,
                output_names=node.outputs,
                custom_proto_spec=params,
            )

        def test_conversion(onnx_model, add_custom_layers=False):
            coreml_model = convert(
                onnx_model,
                add_custom_layers=add_custom_layers,
                custom_conversion_functions={"Flatten": convert_flatten},
            )

            spec = coreml_model.get_spec()
            layers = spec.neuralNetwork.layers
            self.assertIsNotNone(layers[0].custom)
            self.assertEqual("Flatten", layers[0].custom.className)
            self.assertEqual(3, layers[0].custom.parameters["axis"].intValue)

        onnx_model = _make_model_flatten_axis3()
        # Test with add_custom_layers True
        convert(
            onnx_model,
            add_custom_layers=True,
            custom_conversion_functions={"Flatten": convert_flatten},
        )

        # Test with add_custom_layers False
        convert(
            onnx_model,
            add_custom_layers=False,
            custom_conversion_functions={"Flatten": convert_flatten},
        )
