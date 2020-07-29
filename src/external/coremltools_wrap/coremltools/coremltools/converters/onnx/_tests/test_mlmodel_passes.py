import numpy as np
import unittest
import coremltools.models.datatypes as datatypes
from coremltools.models import neural_network as neural_network
from coremltools.converters.mil.backend.nn.passes.mlmodel_passes import (
    remove_disconnected_layers,
)


class MLModelPassesTest(unittest.TestCase):
    def test_load_constant_remove(self):
        input_features = [("data", datatypes.Array(*(3, 4)))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_activation("relu1", "RELU", "data", "relu1")
        builder.add_load_constant_nd(
            "const1", "c1", constant_value=np.ones((5,)), shape=(5,)
        )
        builder.add_activation("relu2", "RELU", "relu1", "out")
        builder.add_load_constant_nd(
            "const2", "c2", constant_value=np.ones((5,)), shape=(5,)
        )
        builder.add_load_constant_nd(
            "const3", "c3", constant_value=np.ones((5,)), shape=(5,)
        )
        spec = builder.spec
        np.testing.assert_equal(5, len(spec.neuralNetwork.layers))
        remove_disconnected_layers(spec)
        np.testing.assert_equal(2, len(spec.neuralNetwork.layers))
