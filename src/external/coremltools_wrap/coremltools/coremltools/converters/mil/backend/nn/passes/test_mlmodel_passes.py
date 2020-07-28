#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import copy
import pytest
import numpy as np
import unittest
from sys import platform

from coremltools._deps import _IS_MACOS
import coremltools.models.datatypes as datatypes
from coremltools.models.utils import _macos_version
from coremltools.models import neural_network as neural_network
from coremltools.models import MLModel
from coremltools.models.neural_network.printer import print_network_spec
from coremltools.converters.mil.backend.nn.passes.mlmodel_passes import (
    remove_disconnected_layers,
    transform_conv_crop,
    remove_redundant_transposes,
)

DEBUG = False
np.random.seed(10)


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

    def test_dead_layer_remove(self):
        input_features = [("data", datatypes.Array(*(3, 4)))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_activation("relu1", "RELU", "data", "relu1")
        builder.add_load_constant_nd(
            "const1", "c1", constant_value=np.ones((5,)), shape=(5,)
        )
        builder.add_load_constant_nd(
            "const2", "c2", constant_value=np.ones((5,)), shape=(5,)
        )
        builder.add_split_nd(
            "splitnd1", "const2", ["s1", "s2", "s3"], axis=0, num_splits=3
        )
        builder.add_squeeze("squeeze", "s1", "squeeze_out")
        builder.add_activation("relu4", "RELU", "s2", "relu4")
        builder.add_activation("relu5", "RELU", "relu4", "relu5")
        builder.add_load_constant_nd(
            "const3", "c3", constant_value=np.ones((5,)), shape=(5,)
        )
        builder.add_activation("relu2", "RELU", "relu1", "out")
        spec = builder.spec
        np.testing.assert_equal(9, len(spec.neuralNetwork.layers))
        remove_disconnected_layers(spec)
        np.testing.assert_equal(2, len(spec.neuralNetwork.layers))

    @pytest.mark.xfail
    def test_dead_layer_remove_branch(self):
        convergence_tolerance = 1e-8

        input_features = [("input", datatypes.Array(*(2,)))]
        output_features = [("out", None)]

        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        # add condition to break from the loop, if convergence criterion is met
        builder.add_less_than("cond", ["input"], "cond", alpha=convergence_tolerance)
        branch_layer = builder.add_branch("branch_layer", "cond")
        builder_ifbranch = neural_network.NeuralNetworkBuilder(
            nn_spec=branch_layer.branch.ifBranch
        )
        builder_ifbranch.add_activation("relu1", "RELU", "input", "relu1_out")
        builder_ifbranch.add_activation("relu2_out", "RELU", "relu1_out", "relu2_out")
        builder_elsebranch = neural_network.NeuralNetworkBuilder(
            nn_spec=branch_layer.branch.elseBranch
        )
        builder_elsebranch.add_activation("linear1", "LINEAR", "input", "linear1_out")
        builder_elsebranch.add_activation(
            "linear2", "LINEAR", "linear1_out", "relu2_out"
        )
        builder.add_squeeze("out", "input", "out", squeeze_all=True)

        mlmodel = MLModel(builder.spec)
        data = np.random.rand(2,)
        data_dict = {"input": data}
        if _IS_MACOS:
            before_pass_out = mlmodel.predict(data_dict)["out"]
            if DEBUG:
                print(
                    "\n mlmodel description before remove disconnected layers pass: \n"
                )
                print_network_spec(builder.spec, style="coding")
            remove_disconnected_layers(builder.spec)
            if DEBUG:
                print(
                    "\n mlmodel description after remove disconnected layers pass: \n"
                )
                print_network_spec(builder.spec, style="coding")
            mlmodel = MLModel(builder.spec)
            after_pass_out = mlmodel.predict(data_dict)["out"]

            np.testing.assert_almost_equal(before_pass_out, after_pass_out, decimal=2)
            np.testing.assert_equal(len(builder.spec.neuralNetwork.layers), 1)

    @pytest.mark.xfail
    def test_dead_layer_partial_branch(self):
        convergence_tolerance = 1e-8

        input_features = [("input", datatypes.Array(*(2,)))]
        output_features = [("out", None)]

        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        # add condition to break from the loop, if convergence criterion is met
        builder.add_less_than("cond", ["input"], "cond", alpha=convergence_tolerance)
        branch_layer = builder.add_branch("branch_layer", "cond")
        builder_ifbranch = neural_network.NeuralNetworkBuilder(
            nn_spec=branch_layer.branch.ifBranch
        )
        builder_ifbranch.add_activation("relu1", "RELU", "input", "relu1_out")
        builder_ifbranch.add_activation("relu2_out", "RELU", "relu1_out", "relu2_out")
        builder_elsebranch = neural_network.NeuralNetworkBuilder(
            nn_spec=branch_layer.branch.elseBranch
        )
        builder_elsebranch.add_activation("linear1", "LINEAR", "input", "linear1_out")
        builder_elsebranch.add_activation(
            "linear_red_1", "LINEAR", "input", "linear_red1_out"
        )
        builder_elsebranch.add_activation(
            "linear_red_2", "LINEAR", "linear_red1_out", "linear_red2_out"
        )
        builder_elsebranch.add_activation(
            "linear2", "LINEAR", "linear1_out", "relu2_out"
        )
        builder.add_squeeze("out", "relu2_out", "out", squeeze_all=True)

        mlmodel = MLModel(builder.spec)
        data = np.random.rand(2,)
        data_dict = {"input": data}
        before_pass_out = mlmodel.predict(data_dict)["out"]
        if DEBUG:
            print("\n mlmodel description before remove disconnected layers pass: \n")
            print_network_spec(builder.spec, style="coding")
        old_spec = copy.copy(builder.spec)
        remove_disconnected_layers(builder.spec)
        if DEBUG:
            print("\n mlmodel description after remove disconnected layers pass: \n")
            print_network_spec(builder.spec, style="coding")
        mlmodel = MLModel(builder.spec)
        after_pass_out = mlmodel.predict(data_dict)["out"]

        np.testing.assert_almost_equal(before_pass_out, after_pass_out, decimal=2)
        np.testing.assert_equal(
            len(old_spec.neuralNetwork.layers[1].branch.ifBranch.layers),
            len(builder.spec.neuralNetwork.layers[1].branch.ifBranch.layers),
        )
        np.testing.assert_equal(
            len(builder.spec.neuralNetwork.layers[1].branch.elseBranch.layers), 2
        )

    def test_conv_crop_bn_to_conv_bn_crop(self):
        input_features = [("data", datatypes.Array(1, 10, 10))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)
        W = np.ones((1, 2, 2, 2), dtype=np.float32)
        builder.add_convolution(
            name="conv",
            kernel_channels=1,
            output_channels=2,
            height=2,
            width=2,
            stride_height=1,
            stride_width=1,
            border_mode="valid",
            groups=1,
            W=W,
            b=None,
            has_bias=False,
            input_name="data",
            output_name="conv_out",
        )
        builder.add_crop(
            name="crop",
            left=1,
            right=1,
            top=1,
            bottom=1,
            offset=0,
            input_names=["conv_out"],
            output_name="crop_out",
        )
        builder.add_batchnorm(
            name="bn",
            channels=2,
            gamma=np.ones(2,).astype(np.float32),
            beta=np.ones(2,).astype(np.float32),
            mean=np.ones(2,).astype(np.float32),
            variance=np.ones(2,).astype(np.float32),
            input_name="crop_out",
            output_name="out",
        )
        # Conv -> Crop -> BN
        spec = builder.spec.neuralNetwork
        np.testing.assert_equal("crop", spec.layers[1].WhichOneof("layer"))
        np.testing.assert_equal("batchnorm", spec.layers[2].WhichOneof("layer"))

        # Predict
        if _IS_MACOS:
            mlmodel = MLModel(builder.spec)
            data = np.random.rand(1, 10, 10)
            data_dict = {"data": data}
            before_pass_out = mlmodel.predict(data_dict, useCPUOnly=True)["out"]

        # transform the pattern
        transform_conv_crop(builder.spec)
        # Conv -> BN -> Crop
        np.testing.assert_equal("batchnorm", spec.layers[1].WhichOneof("layer"))
        np.testing.assert_equal("crop", spec.layers[2].WhichOneof("layer"))

        if _IS_MACOS:
            # Predict
            mlmodel = MLModel(builder.spec)
            after_pass_out = mlmodel.predict(data_dict, useCPUOnly=True)["out"]
            np.testing.assert_almost_equal(before_pass_out, after_pass_out, decimal=3)

    def test_conv_crop_bn_relu_to_conv_bn_relu_crop(self):
        input_features = [("data", datatypes.Array(1, 10, 10))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

        W = np.ones((1, 2, 2, 2), dtype=np.float32)
        builder.add_convolution(
            name="conv",
            kernel_channels=1,
            output_channels=2,
            height=2,
            width=2,
            stride_height=1,
            stride_width=1,
            border_mode="valid",
            groups=1,
            W=W,
            b=None,
            has_bias=False,
            input_name="data",
            output_name="conv_out",
        )
        builder.add_crop(
            name="crop",
            left=1,
            right=1,
            top=1,
            bottom=1,
            offset=0,
            input_names=["conv_out"],
            output_name="crop_out",
        )
        builder.add_batchnorm(
            name="bn",
            channels=2,
            gamma=np.ones(2,).astype(np.float32),
            beta=np.ones(2,).astype(np.float32),
            mean=np.ones(2,).astype(np.float32),
            variance=np.ones(2,).astype(np.float32),
            input_name="crop_out",
            output_name="bn_out",
        )
        builder.add_activation(
            name="relu", non_linearity="RELU", input_name="bn_out", output_name="out"
        )
        # Conv -> Crop -> BN -> ReLU
        spec = builder.spec.neuralNetwork
        np.testing.assert_equal("crop", spec.layers[1].WhichOneof("layer"))
        np.testing.assert_equal("batchnorm", spec.layers[2].WhichOneof("layer"))
        np.testing.assert_equal("activation", spec.layers[3].WhichOneof("layer"))

        # Predict
        if _IS_MACOS:
            mlmodel = MLModel(builder.spec)
            data = np.random.rand(1, 10, 10)
            data_dict = {"data": data}
            before_pass_out = mlmodel.predict(data_dict, useCPUOnly=True)["out"]

        # transform the pattern
        transform_conv_crop(builder.spec)
        # Conv -> BN -> ReLU -> Crop
        np.testing.assert_equal("batchnorm", spec.layers[1].WhichOneof("layer"))
        np.testing.assert_equal("activation", spec.layers[2].WhichOneof("layer"))
        np.testing.assert_equal("crop", spec.layers[3].WhichOneof("layer"))

        # Predict
        mlmodel = MLModel(builder.spec)
        if _IS_MACOS:
            after_pass_out = mlmodel.predict(data_dict, useCPUOnly=True)["out"]
            np.testing.assert_almost_equal(before_pass_out, after_pass_out, decimal=3)


@unittest.skipIf(
    platform != "darwin" or _macos_version() < (10, 15), "Requires MacOS 10.15 or later"
)
class Redundant_Transposees_Test(unittest.TestCase):
    def _test_builder(self, builder, input_shape, expected_layer_num=None):

        data = np.random.rand(*input_shape)

        # Mlmodel before
        mlmodel = MLModel(builder.spec)
        output_before = mlmodel.predict({"data": data})["out"]
        num_layers_before = len(builder.spec.neuralNetwork.layers)

        remove_redundant_transposes(builder.spec)

        layers = builder.spec.neuralNetwork.layers
        if expected_layer_num == None:
            self.assertTrue(len(layers) < num_layers_before)
        else:
            self.assertEqual(len(layers), expected_layer_num)

        # Mlmodel after
        mlmodel = MLModel(builder.spec)
        output_after = mlmodel.predict({"data": data})["out"]

        np.testing.assert_almost_equal(output_before, output_after, decimal=3)

    def test_output_edge_case(self):

        # For now for safety purpose, the node which are output should't be merged
        input_shape = (1, 10, 5)
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_transpose(
            name="first_transpose",
            axes=[2, 0, 1],
            input_name="data",
            output_name="first_transpose_out",
        )
        builder.add_transpose(
            name="second_transpose",
            axes=[1, 2, 0],
            input_name="first_transpose_out",
            output_name="out",
        )

        self._test_builder(builder, input_shape, 2)

    def test_output_edge_case_2(self):

        # For now for safety purpose, the node which are output should't be merged
        input_shape = (1, 10, 5)
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_transpose(
            name="ranspose", axes=[1, 2, 0], input_name="data", output_name="out"
        )

        self._test_builder(builder, input_shape, 1)

    def test_remove_single_identity_transpose(self):

        # A single identity transpose (like 0,1,2) should also be removed
        input_shape = (1, 10, 5)
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        builder.add_transpose(
            name="uselss_transpose",
            axes=[0, 1, 2],
            input_name="data",
            output_name="useless_transpose_out",
        )
        builder.add_activation(
            name="relu",
            non_linearity="RELU",
            input_name="useless_transpose_out",
            output_name="out",
        )

        self._test_builder(builder, input_shape, 1)

    def test_remove_three_transpose(self):

        # Three transpose layer which can be removed
        input_shape = (1, 10, 5)
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        transpose = [[2, 1, 0], [1, 0, 2], [2, 0, 1]]
        input_name = "data"
        for i, axes in enumerate(transpose):
            name = "transpose_" + str(i)
            output_name = name + "_out"
            builder.add_transpose(
                name=name, axes=axes, input_name=input_name, output_name=output_name
            )
            input_name = output_name

        builder.add_activation(
            name="relu", non_linearity="RELU", input_name=input_name, output_name="out"
        )

        self._test_builder(builder, input_shape, 1)

    def test_remove_thousands_identity_transpose(self):

        """
               INPUT
                 |
                 v
                [t1]
                 |
                 v
                [t2]
                 |
                 v
                 .
                 .
                 .
                 |
                 v
               [t1000]
                 |
                 v
                RELU
        tk are all identity
        Remove a sequence of 1000 identity transpose
        """
        input_shape = (1, 10, 5)
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )

        num_layers = 1000
        input_name = "data"
        for i in range(num_layers):
            output_name = "layer_" + str(i) + "_output"
            name = "layer_" + str(i)
            builder.add_transpose(
                name=name,
                axes=[0, 1, 2],
                input_name=input_name,
                output_name=output_name,
            )
            input_name = output_name

        builder.add_activation(
            name="relu", non_linearity="RELU", input_name=input_name, output_name="out"
        )

        self._test_builder(builder, input_shape, 1)

    def test_remove_thousands_identity_transpose_with_activation_between(self):
        """
               INPUT
                 |
                 v
                [t1]
                 |
                 v
                 .
                 .
                 .
                [t500]
                 |
                 v
                RELU_1
                 |
                 v
                 .
                 .
                 .
                 |
                 v
               [t1000]
                 |
                 v
                RELU_2
        tk are all identity
        Remove a sequence of 1000 identity transpose but with a RELU in the middle,
        the final output should be
               INPUT
                 |
                 v
                RELU_1
                 |
                 v
                RELU_2

        """
        input_shape = (1, 10, 5)
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )

        num_layers = 1000
        input_name = "data"
        for i in range(num_layers):
            output_name = "layer_" + str(i) + "_output"
            name = "layer_" + str(i)
            builder.add_transpose(
                name=name,
                axes=[0, 1, 2],
                input_name=input_name,
                output_name=output_name,
            )
            input_name = output_name
            if i == num_layers / 2:
                builder.add_activation(
                    name="relu_inter",
                    non_linearity="ReLU",
                    input_name=input_name,
                    output_name="relu_out",
                )
                input_name = "relu_out"
        builder.add_activation(
            name="relu", non_linearity="RELU", input_name=input_name, output_name="out"
        )
        self._test_builder(builder, input_shape, 2)

    def test_remove_thousands_random_transpose_layers(self):
        """
               INPUT
                 |
                 v
                [t_0]
                 |
                 v
                [t_1]
                 |
                 v
                 .
                 .
                 .
                 |
                 v
               [t_999]
                 |
                 v
                RELU
        tk are randomly generated,
        under this certain seed, the result should be
                INPUT
                 |
                 v
                [t_0]
                 |
                 v
                [t_1]
                 |
                 v
                RELU
        """

        from itertools import permutations
        import random

        random.seed(1000)
        input_shape = (3, 10, 5)
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )

        num_layers = 1000
        dim = 3
        input_name = "data"
        debug = []
        for i in range(num_layers):
            axes = list(permutations(range(dim)))
            random.shuffle(axes)
            output_name = "layer_" + str(i) + "_output"
            name = "layer_" + str(i)
            debug.append(axes[0])
            builder.add_transpose(
                name=name, axes=axes[0], input_name=input_name, output_name=output_name
            )
            input_name = output_name
        builder.add_activation(
            name="relu", non_linearity="RELU", input_name=input_name, output_name="out"
        )
        self._test_builder(builder, input_shape, None)

    def test_remove_thousands_random_transpose_layers_case_2(self):
        """
        Same test as the previous one, but add more layers and dimension.
        """
        from itertools import permutations
        import random

        random.seed(0)
        input_shape = (3, 10, 5, 2, 4)
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )

        num_layers = 5000
        dim = 5
        input_name = "data"
        for i in range(num_layers):
            axes = list(permutations(range(dim)))
            random.shuffle(axes)
            output_name = "layer_" + str(i) + "_output"
            name = "layer_" + str(i)
            builder.add_transpose(
                name=name, axes=axes[0], input_name=input_name, output_name=output_name
            )
            input_name = output_name
        builder.add_activation(
            name="relu", non_linearity="RELU", input_name=input_name, output_name="out"
        )
        self._test_builder(builder, input_shape, None)

    def test_branch_structure(self):
        """
                INPUT
                 |
                 v
                [t_0]
                 |
                 v
                [t_1]
                 |
                 v
                [t_3] --.
                 |      |
                 v      v
                [t_4]  RELU_1
                 |
                 v
                [t_5]
                 |
                 v
                RELU_2
        t_0, t_1, t_3 can be merged.
        t_4, t_5 can be merged.
        The output shuld be
                INPUT
                 |
                 .------.
                 |      |
                 v      v
               RELU_2  RELU_1

        """
        input_shape = (1, 10, 5)
        input_features = [("data", datatypes.Array(1, 10, 5))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        transpose = [[2, 1, 0], [2, 1, 0], [0, 1, 2], [2, 0, 1], [1, 2, 0]]
        input_name = "data"
        for i, axes in enumerate(transpose):
            name = "transpose_" + str(i)
            output_name = name + "_out"
            builder.add_transpose(
                name=name, axes=axes, input_name=input_name, output_name=output_name
            )
            input_name = output_name

        builder.add_activation(
            name="relu", non_linearity="RELU", input_name=input_name, output_name="out"
        )
        builder.add_activation(
            name="dumpy",
            non_linearity="RELU",
            input_name="transpose_2_out",
            output_name="dumpy",
        )
        self._test_builder(builder, input_shape, 2)

    def test_branch_case_2(self):
        """
                INPUT
                 |
                 v
                [t_0] --.
                 |      |
                 v      v
                [t_1]  RELU_1
                 |
                 v
                RELU_2
        Even though t_0, t_1 can be merged, but there is a branch from t_0,
        so we shouldn't remove anything here.

        """
        input_shape = (1, 10, 5)
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        transpose = [[2, 1, 0], [2, 1, 0]]
        input_name = "data"
        for i, axes in enumerate(transpose):
            name = "transpose_" + str(i)
            output_name = name + "_out"
            builder.add_transpose(
                name=name, axes=axes, input_name=input_name, output_name=output_name
            )
            input_name = output_name

        builder.add_activation(
            name="relu", non_linearity="RELU", input_name=input_name, output_name="out"
        )
        builder.add_activation(
            name="dumpy",
            non_linearity="RELU",
            input_name="transpose_0_out",
            output_name="dumpy",
        )
        self._test_builder(builder, input_shape, 4)

    def test_fork_structure_case_3(self):
        """
                INPUT
                 |
                 v
                [t_0]
                 |
                 v
                [t_1]--.
                 |     |
                 |     v
                 |    RELU_1
                 |
                 v
                [t_2]--.
                 |     |
                 |     v
                 |    RELU_2
                [t_3]
                 |
                 v
                [t_4]--.
                 |     |
                 |     v
                 |    RELU_3
                 v
                RELU_4

        Even though t_0, t_1 can be merged, t_2 is identity, t_3, t_4 can be merge,
        The final output should be
                   INPUT
                     |
        .------------.----------.
        |        |       |      |
        v        v       v      v
      RELU_1   RELU_2  RELU_3  RELU_4

        """
        input_shape = (1, 10, 5)
        input_features = [("data", datatypes.Array(1, 10, 5))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        transpose = [[2, 1, 0], [2, 1, 0], [0, 1, 2], [2, 1, 0], [2, 1, 0]]
        input_name = "data"
        for i, axes in enumerate(transpose):
            name = "transpose_" + str(i)
            output_name = name + "_out"
            builder.add_transpose(
                name=name, axes=axes, input_name=input_name, output_name=output_name
            )
            input_name = output_name

        builder.add_activation(
            name="relu", non_linearity="RELU", input_name=input_name, output_name="out"
        )
        builder.add_activation(
            name="dumpy_1",
            non_linearity="RELU",
            input_name="transpose_1_out",
            output_name="dumpy_1",
        )
        builder.add_activation(
            name="dumpy_2",
            non_linearity="RELU",
            input_name="transpose_2_out",
            output_name="dumpy_2",
        )
        builder.add_activation(
            name="dumpy_4",
            non_linearity="RELU",
            input_name="transpose_4_out",
            output_name="dumpy_4",
        )

        self._test_builder(builder, input_shape, 4)

    def test_fork(self):
        """
                   INPUT
                     |
              .------.------.
              |             |
              v             v
             [t_1]         [t_3]
              |             |
              v             v
             [t_2]         [t_4]
              |             |
              v             v
            RELU_1         RELU_2

            t_1,t_2 can be merged and t_3,t_4 can be merged.
            The result output would be

                   INPUT
                     |
              .------.------.
              |             |
              v             v
            RELU_1         RELU_2

        """
        input_shape = (1, 10, 5)
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        transpose = [[2, 1, 0], [2, 1, 0]]
        input_name = "data"
        for i, axes in enumerate(transpose):
            name = "transpose_" + str(i)
            output_name = name + "_out"
            builder.add_transpose(
                name=name, axes=axes, input_name=input_name, output_name=output_name
            )
            input_name = output_name

        builder.add_activation(
            name="relu", non_linearity="RELU", input_name=input_name, output_name="out"
        )

        input_name = "data"
        for i, axes in enumerate(transpose):
            name = "transpose_branch_2_" + str(i)
            output_name = name + "_out"
            builder.add_transpose(
                name=name, axes=axes, input_name=input_name, output_name=output_name
            )
            input_name = output_name

        builder.add_activation(
            name="relu_branch_2",
            non_linearity="RELU",
            input_name=input_name,
            output_name="out_branch_2",
        )
        self._test_builder(builder, input_shape, 2)

    def test_fork_and_add(self):
        """
                   INPUT
                     |
              .------.------.
              |             |
              v             v
             [t_1]         [t_3]
              |             |
              v             v
             [t_2]         [t_4]
              |             |
              .-----. .-----.
                    | |
                    v v
                    Add

            t_1,t_2 can be merged and t_3,t_4 can be merged.
            The result output would be

                   INPUT
                     |
              .------.------.
              |             |
              .-----. .-----.
                    | |
                    v v
                    Add

        """
        input_shape = (1, 10, 5)
        input_features = [("data", datatypes.Array(*input_shape))]
        output_features = [("out", None)]
        builder = neural_network.NeuralNetworkBuilder(
            input_features, output_features, disable_rank5_shape_mapping=True
        )
        transpose = [[2, 1, 0], [2, 1, 0]]
        input_name = "data"
        for i, axes in enumerate(transpose):
            name = "transpose_" + str(i)
            output_name = name + "_out"
            builder.add_transpose(
                name=name, axes=axes, input_name=input_name, output_name=output_name
            )
            input_name = output_name

        input_1 = input_name

        input_name = "data"
        for i, axes in enumerate(transpose):
            name = "transpose_branch_2_" + str(i)
            output_name = name + "_out"
            builder.add_transpose(
                name=name, axes=axes, input_name=input_name, output_name=output_name
            )
            input_name = output_name

        input_2 = input_name

        builder.add_add_broadcastable(
            name="add", input_names=[input_1, input_2], output_name="out"
        )
        self._test_builder(builder, input_shape, 1)

    def test_transpose(self):
        def _build_and_test_network(input_size, transpose_layers, expected_layers):
            """
            Helper function for testing transpose removal.

            Args:
                input_size: Size of the input network tensor.
                transpose_layers: Array of transpose axes definitions.
                expected_layers: Array of indices into transpose_layers indicating
                    which of the transpose layers should be present after the
                    graph pass.
            """
            input_features = [("data", datatypes.Array(*input_size))]
            output_features = [("out", None)]
            builder = neural_network.NeuralNetworkBuilder(
                input_features, output_features
            )
            spec = builder.spec.neuralNetwork.layers

            last_layer = "data"
            for idx, axes in enumerate(transpose_layers):
                name = "t{}".format(idx)
                if idx == len(transpose_layers) - 1:
                    output_name = "out"
                else:
                    output_name = name + "_out"
                builder.add_transpose(
                    name=name, axes=axes, input_name=last_layer, output_name=output_name
                )
                last_layer = output_name

            spec = builder.spec.neuralNetwork
            # Check the network before the graph pass.
            for idx in range(len(transpose_layers)):
                np.testing.assert_equal(
                    "transpose", spec.layers[idx].WhichOneof("layer")
                )
            # Run the removal pass.
            remove_redundant_transposes(builder.spec)
            # Verify only the expected layers remain.
            np.testing.assert_equal(len(spec.layers), len(expected_layers))
            for output_layer_idx, input_layer_idx in enumerate(expected_layers):
                np.testing.assert_equal(
                    "transpose", spec.layers[output_layer_idx].WhichOneof("layer")
                )
                np.testing.assert_array_equal(
                    transpose_layers[input_layer_idx],
                    spec.layers[output_layer_idx].transpose.axes,
                )

        _build_and_test_network(
            input_size=[1, 10, 10],
            # These transposes are not inverses.
            transpose_layers=[[2, 0, 1], [2, 0, 1]],
            expected_layers=[0, 1],
        )

        _build_and_test_network(
            input_size=[1, 1, 10, 10, 3],
            # First two are the identity, then an extra.
            transpose_layers=[[2, 4, 1, 0, 3], [3, 2, 0, 4, 1], [1, 0, 2, 3, 4]],
            expected_layers=[2],
        )

        # A slightly more complicated test case where there are two transposes
        # in topological order, but are actually in parallel in the graph.
        builder = neural_network.NeuralNetworkBuilder(
            [("data", datatypes.Array(2, 4, 8))], [("out", None)]
        )
        last_layer = "data"
        builder.add_transpose(
            name="t1", axes=[0, 2, 1], input_name="data", output_name="t1"
        )
        builder.add_transpose(
            name="t2", axes=[0, 2, 1], input_name="data", output_name="t2"
        )
        builder.add_stack(name="stack", input_names=["t1", "t2"], output_name="out")
        spec = builder.spec.neuralNetwork
        # Run the removal pass.
        remove_redundant_transposes(builder.spec)
        # Verify nothing was removed.
        np.testing.assert_equal(len(spec.layers), 3)


if __name__ == "__main__":
    RUN_ALL_TESTS = True
    if RUN_ALL_TESTS:
        unittest.main()
    else:
        suite = unittest.TestSuite()
        suite.addTest(MLModelPassesTest("test_load_constant_remove"))
        unittest.TextTestRunner().run(suite)
