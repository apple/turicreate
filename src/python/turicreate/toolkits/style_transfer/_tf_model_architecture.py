# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from turicreate.toolkits._tf_model import TensorFlowModel

import numpy as _np
import turicreate.toolkits._tf_utils as _utils
import tensorflow.compat.v1 as _tf

_tf.disable_v2_behavior()


def define_tensorflow_variables(net_params, trainable=True):
    """
    This function defines TF Variables from the C++ initialization.

    Parameters
    ----------
    trainable: boolean
        If `True` the transformer network updates the convolutional layers as
        well as the instance norm layers of the network. If `False` only the
        instance norm layers of the network are updated.

        Note the VGG network's parameters aren't updated
    Returns
    -------
    out: dict
        The TF Variable dictionary.
    """
    tensorflow_variables = dict()
    for key in net_params.keys():
        if "weight" in key:
            # only set the parameter to train if in the transformer network
            train_param = trainable and "transformer_" in key
            if "conv" in key:
                tensorflow_variables[key] = _tf.Variable(
                    initial_value=_utils.convert_conv2d_coreml_to_tf(net_params[key]),
                    name=key,
                    trainable=train_param,
                )
            else:
                # This is the path that the instance norm takes
                tensorflow_variables[key] = _tf.Variable(
                    initial_value=_utils.convert_dense_coreml_to_tf(net_params[key]),
                    name=key,
                    trainable=True,
                )
        else:
            tensorflow_variables[key] = _tf.Variable(
                initial_value=net_params[key], name=key, trainable=False
            )
    return tensorflow_variables


def define_instance_norm(tf_input, tf_index, weights, prefix):
    """
    This function defines the indexed instance norm node the tensorflow nn api.

    Parameters
    ----------
    tf_input: tensorflow.Tensor
        The input tensor to the instance norm node. Data format expected to be
        in `NHWC`

    tf_index: tensorflow.Tensor
        The index tensor to the instance norm node.
    prefix: string
        The prefix column is used to prefix the variables of the instance norm
        node for weight export.
    weights: dictionary
        The dictionary of weights to the network. The naming convention
        used is that from the CoreML export of the Style Transfer Network.
    Returns
    -------
    out: tensorflow.Tensor
        The instance norm output tensor to the network.
    """
    epsilon = 1e-5

    gamma = weights[prefix + 'gamma_weight']
    beta = weights[prefix + 'beta_weight']

    inputs_rank = tf_input.shape.ndims
    reduction_axis = inputs_rank - 1
    moments_axes = list(range(inputs_rank))

    del moments_axes[reduction_axis]
    del moments_axes[0]

    indexed_gamma = _tf.gather(gamma, tf_index)
    indexed_beta = _tf.gather(beta, tf_index)

    expanded_gamma = _tf.expand_dims(_tf.expand_dims(indexed_gamma, 1), 1)
    expanded_beta = _tf.expand_dims(_tf.expand_dims(indexed_beta, 1), 1)

    mean, variance = _tf.nn.moments(tf_input, moments_axes, keep_dims=True)
    return _tf.nn.batch_normalization(
        tf_input,
        mean,
        variance,
        expanded_beta,
        expanded_gamma,
        epsilon)
    
def define_residual(tf_input, tf_index, weights, prefix):
    """
    This function defines the residual network using the tensorflow nn api.

    Parameters
    ----------
    tf_input: tensorflow.Tensor
        The input tensor to the residual network.
    tf_index: tensorflow.Tensor
        The index tensor to the residual network.
    weights: dictionary
        The dictionary of weights to the network. The naming convention
        used is that from the CoreML export of the Style Transfer Network.

    prefix: string
        The prefix column is used to prefix the variables of the network for
        weight export.
    Returns
    -------
    out: tensorflow.Tensor
        The sigmoid output tensor to the network.
    """
    # TODO: Refactor Instance Norm
    conv_1_paddings = _tf.constant([[0, 0], [1, 1], [1, 1], [0, 0]])
    conv_1_pad = _tf.pad(tf_input, conv_1_paddings, "REFLECT")
    conv_1_filter = weights[prefix + "conv_1_weight"]
    conv_1 = _tf.nn.conv2d(
        conv_1_pad, conv_1_filter, strides=[1, 1, 1, 1], padding="VALID"
    )
    inst_1 = define_instance_norm(conv_1, tf_index, weights, prefix + "inst_1_")
    relu_1 = _tf.nn.relu(inst_1)

    conv_2_paddings = _tf.constant([[0, 0], [1, 1], [1, 1], [0, 0]])
    conv_2_pad = _tf.pad(relu_1, conv_2_paddings, "REFLECT")
    conv_2_filter = weights[prefix + "conv_2_weight"]
    conv_2 = _tf.nn.conv2d(
        conv_2_pad, conv_2_filter, strides=[1, 1, 1, 1], padding="VALID"
    )
    inst_2 = define_instance_norm(conv_2, tf_index, weights, prefix + "inst_2_")

    return _tf.add(tf_input, inst_2)


def define_resnet(tf_input, tf_index, weights, prefix="transformer_"):
    """
    This function defines the resnet network using the tensorflow nn api.

    Parameters
    ----------
    tf_input: tensorflow.Tensor
        The input tensor to the network. The image is expected to be in RGB
        format.
    tf_index: tensorflow.Tensor
        The index tensor to the network.
    weights: dictionary
        The dictionary of weights to the network. The naming convention
        used is that from the CoreML export of the Style Transfer Network.

    prefix: string
        The prefix column is used to prefix the variables of the network for
        weight export.
    Returns
    -------
    out: tensorflow.Tensor
        The sigmoid output tensor to the network.
    """

    # encoding 1
    conv_1_paddings = _tf.constant([[0, 0], [4, 4], [4, 4], [0, 0]])
    conv_1_pad = _tf.pad(tf_input, conv_1_paddings, "REFLECT")
    conv_1_filter = weights[prefix + "encode_1_conv_weight"]
    conv_1 = _tf.nn.conv2d(
        conv_1_pad, conv_1_filter, strides=[1, 1, 1, 1], padding="VALID"
    )
    inst_1 = define_instance_norm(conv_1, tf_index, weights, prefix + "encode_1_inst_")
    relu_1 = _tf.nn.relu(inst_1)

    # encoding 2
    conv_2_paddings = _tf.constant([[0, 0], [1, 1], [1, 1], [0, 0]])
    conv_2_pad = _tf.pad(relu_1, conv_2_paddings, "REFLECT")
    conv_2_filter = weights[prefix + "encode_2_conv_weight"]
    conv_2 = _tf.nn.conv2d(
        conv_2_pad, conv_2_filter, strides=[1, 2, 2, 1], padding="VALID"
    )
    inst_2 = define_instance_norm(conv_2, tf_index, weights, prefix + "encode_2_inst_")
    relu_2 = _tf.nn.relu(inst_2)

    # encoding 3
    conv_3_paddings = _tf.constant([[0, 0], [1, 1], [1, 1], [0, 0]])
    conv_3_pad = _tf.pad(relu_2, conv_3_paddings, "REFLECT")
    conv_3_filter = weights[prefix + "encode_3_conv_weight"]
    conv_3 = _tf.nn.conv2d(
        conv_3_pad, conv_3_filter, strides=[1, 2, 2, 1], padding="VALID"
    )
    inst_3 = define_instance_norm(conv_3, tf_index, weights, prefix + "encode_3_inst_")
    relu_3 = _tf.nn.relu(inst_3)

    # Residual Blocks
    residual_1 = define_residual(relu_3, tf_index, weights, prefix + "residual_1_")
    residual_2 = define_residual(residual_1, tf_index, weights, prefix + "residual_2_")
    residual_3 = define_residual(residual_2, tf_index, weights, prefix + "residual_3_")
    residual_4 = define_residual(residual_3, tf_index, weights, prefix + "residual_4_")
    residual_5 = define_residual(residual_4, tf_index, weights, prefix + "residual_5_")

    # decode 1
    decode_1_image_shape = _tf.shape(residual_5)
    decode_1_new_height = decode_1_image_shape[1] * 2
    decode_1_new_width = decode_1_image_shape[2] * 2

    decoding_1_upsample_1 = _tf.image.resize_images(
        residual_5,
        [decode_1_new_height, decode_1_new_width],
        method=_tf.image.ResizeMethod.NEAREST_NEIGHBOR,
    )

    decode_1_conv_1_paddings = _tf.constant([[0, 0], [1, 1], [1, 1], [0, 0]])

    decode_1_conv_1_pad = _tf.pad(
        decoding_1_upsample_1, decode_1_conv_1_paddings, "REFLECT"
    )

    decode_1_conv_1_filter = weights[prefix + "decoding_1_conv_weight"]

    decode_1_conv_1 = _tf.nn.conv2d(
        decode_1_conv_1_pad,
        decode_1_conv_1_filter,
        strides=[1, 1, 1, 1],
        padding="VALID",
    )

    decode_1_inst_1 = define_instance_norm(
        decode_1_conv_1, tf_index, weights, prefix + "decoding_1_inst_"
    )

    decode_1_relu_1 = _tf.nn.relu(decode_1_inst_1)

    # decode 2
    decode_2_image_shape = _tf.shape(decode_1_relu_1)
    decode_2_new_height = decode_2_image_shape[1] * 2
    decode_2_new_width = decode_2_image_shape[2] * 2

    decoding_2_upsample_1 = _tf.image.resize_images(
        decode_1_relu_1,
        [decode_2_new_height, decode_2_new_width],
        method=_tf.image.ResizeMethod.NEAREST_NEIGHBOR,
    )

    decode_2_conv_1_paddings = _tf.constant([[0, 0], [1, 1], [1, 1], [0, 0]])

    decode_2_conv_1_pad = _tf.pad(
        decoding_2_upsample_1, decode_2_conv_1_paddings, "REFLECT"
    )

    decode_2_conv_1_filter = weights[prefix + "decoding_2_conv_weight"]

    decode_2_conv_1 = _tf.nn.conv2d(
        decode_2_conv_1_pad,
        decode_2_conv_1_filter,
        strides=[1, 1, 1, 1],
        padding="VALID",
    )

    decode_2_inst_1 = define_instance_norm(
        decode_2_conv_1, tf_index, weights, prefix + "decoding_2_inst_"
    )

    decode_2_relu_1 = _tf.nn.relu(decode_2_inst_1)

    # decode 3
    decode_3_conv_1_paddings = _tf.constant([[0, 0], [4, 4], [4, 4], [0, 0]])

    decode_3_conv_1_pad = _tf.pad(decode_2_relu_1, decode_3_conv_1_paddings, "REFLECT")

    decode_3_conv_1_filter = weights[prefix + "conv5_weight"]

    decode_3_conv_1 = _tf.nn.conv2d(
        decode_3_conv_1_pad,
        decode_3_conv_1_filter,
        strides=[1, 1, 1, 1],
        padding="VALID",
    )

    decode_3_inst_1 = define_instance_norm(
        decode_3_conv_1, tf_index, weights, prefix + "instancenorm5_"
    )

    decode_3_relu_1 = _tf.nn.sigmoid(decode_3_inst_1)

    return decode_3_relu_1


def define_vgg16(tf_input, weights, prefix="vgg_"):
    """
    This function defines the vgg16 network using the tensorflow nn api.

    Parameters
    ----------
    tf_input: tensorflow.Tensor
        The input tensor to the network. The image is expected to be in RGB
        format
    weights: dictionary
        The dictionary of weights to the network. The naming convention
        used is that from the CoreML export of the Style Transfer Network.

    prefix: string
        The prefix column is used to prefix the variables of the network for
        weight export.

    Returns
    -------

    The function returns four tensorflow.Tensor objects used in the
    featurization of the image passed into the network.
    relu_2:  tensorflow.Tensor
        `relu` from the first block of `conv` and `relu`

    relu_4:  tensorflow.Tensor
        `relu` from the second block of `conv` and `relu`

    relu_7:  tensorflow.Tensor
        `relu` from the third block of `conv` and `relu`
    relu_10: tensorflow.Tensor
        `relu` from the fourth block of `conv` and `relu`

    """
    # block 1
    conv_1_filter = weights[prefix + "block_1_conv_1_weight"]
    conv_1 = _tf.nn.conv2d(
        tf_input, conv_1_filter, strides=[1, 1, 1, 1], padding="SAME"
    )
    conv_1_bias = weights[prefix + "block_1_conv_1_bias"]
    conv_1 = _tf.nn.bias_add(conv_1, conv_1_bias)
    relu_1 = _tf.nn.relu(conv_1)

    conv_2_filter = weights[prefix + "block_1_conv_2_weight"]
    conv_2 = _tf.nn.conv2d(relu_1, conv_2_filter, strides=[1, 1, 1, 1], padding="SAME")
    conv_2_bias = weights[prefix + "block_1_conv_2_bias"]
    conv_2 = _tf.nn.bias_add(conv_2, conv_2_bias)
    relu_2 = _tf.nn.relu(conv_2)

    pool_1 = _tf.nn.avg_pool(relu_2, 2, 2, "SAME")

    # block 2
    conv_3_filter = weights[prefix + "block_2_conv_1_weight"]
    conv_3 = _tf.nn.conv2d(pool_1, conv_3_filter, strides=[1, 1, 1, 1], padding="SAME")
    conv_3_bias = weights[prefix + "block_2_conv_1_bias"]
    conv_3 = _tf.nn.bias_add(conv_3, conv_3_bias)
    relu_3 = _tf.nn.relu(conv_3)

    conv_4_filter = weights[prefix + "block_2_conv_2_weight"]
    conv_4 = _tf.nn.conv2d(relu_3, conv_4_filter, strides=[1, 1, 1, 1], padding="SAME")
    conv_4_bias = weights[prefix + "block_2_conv_2_bias"]
    conv_4 = _tf.nn.bias_add(conv_4, conv_4_bias)
    relu_4 = _tf.nn.relu(conv_4)

    pool_2 = _tf.nn.avg_pool(relu_4, 2, 2, "SAME")

    # block 3
    conv_5_filter = weights[prefix + "block_3_conv_1_weight"]
    conv_5 = _tf.nn.conv2d(pool_2, conv_5_filter, strides=[1, 1, 1, 1], padding="SAME")
    conv_5_bias = weights[prefix + "block_3_conv_1_bias"]
    conv_5 = _tf.nn.bias_add(conv_5, conv_5_bias)
    relu_5 = _tf.nn.relu(conv_5)

    conv_6_filter = weights[prefix + "block_3_conv_2_weight"]
    conv_6 = _tf.nn.conv2d(relu_5, conv_6_filter, strides=[1, 1, 1, 1], padding="SAME")
    conv_6_bias = weights[prefix + "block_3_conv_2_bias"]
    conv_6 = _tf.nn.bias_add(conv_6, conv_6_bias)
    relu_6 = _tf.nn.relu(conv_6)

    conv_7_filter = weights[prefix + "block_3_conv_3_weight"]
    conv_7 = _tf.nn.conv2d(relu_6, conv_7_filter, strides=[1, 1, 1, 1], padding="SAME")
    conv_7_bias = weights[prefix + "block_3_conv_3_bias"]
    conv_7 = _tf.nn.bias_add(conv_7, conv_7_bias)
    relu_7 = _tf.nn.relu(conv_7)

    pool_3 = _tf.nn.avg_pool(relu_7, 2, 2, "SAME")

    # block 4
    conv_8_filter = weights[prefix + "block_4_conv_1_weight"]
    conv_8 = _tf.nn.conv2d(pool_3, conv_8_filter, strides=[1, 1, 1, 1], padding="SAME")
    conv_8_bias = weights[prefix + "block_4_conv_1_bias"]
    conv_8 = _tf.nn.bias_add(conv_8, conv_8_bias)
    relu_8 = _tf.nn.relu(conv_8)

    conv_9_filter = weights[prefix + "block_4_conv_2_weight"]
    conv_9 = _tf.nn.conv2d(relu_8, conv_9_filter, strides=[1, 1, 1, 1], padding="SAME")
    conv_9_bias = weights[prefix + "block_4_conv_2_bias"]
    conv_9 = _tf.nn.bias_add(conv_9, conv_9_bias)
    relu_9 = _tf.nn.relu(conv_9)

    conv_10_filter = weights[prefix + "block_4_conv_3_weight"]
    conv_10 = _tf.nn.conv2d(
        relu_9, conv_10_filter, strides=[1, 1, 1, 1], padding="SAME"
    )
    conv_10_bias = weights[prefix + "block_4_conv_3_bias"]
    conv_10 = _tf.nn.bias_add(conv_10, conv_10_bias)
    relu_10 = _tf.nn.relu(conv_10)

    return relu_2, relu_4, relu_7, relu_10


def define_vgg_pre_processing(tf_input):
    """
    This function defines the vgg_pre_processing network using the tensorflow nn
    api.

    Parameters
    ----------
    tf_input: tensorflow.Tensor
        The input tensor to the network. The image is expected to be in RGB
        format.
    Returns
    -------
    out: tensorflow.Tensor
        The scaled output tensor of the network.
    """
    scaled_input = tf_input * 255.00
    red_channel, green_channel, blue_channel = _tf.split(scaled_input, 3, axis=3)

    red_channel_scaled = red_channel - 123.68
    green_channel_scaled = green_channel - 116.779
    blue_channel_scaled = blue_channel - 103.939

    sub_scalar = _tf.concat(
        [red_channel_scaled, green_channel_scaled, blue_channel_scaled], 3
    )

    return sub_scalar


def define_gram_matrix(tf_input):
    """
    This function defines the gram_matrix computation the tensorflow nn api.

    Parameters
    ----------
    tf_input: tensorflow.Tensor
        The input tensor to the network. The image is expected to be in RGB
        format.
    Returns
    -------
    out: tensorflow.Tensor
        The gram matrix output of the network.
    """
    defined_shape = _tf.shape(tf_input)
    reshaped_output = _tf.reshape(
        tf_input, [-1, defined_shape[1] * defined_shape[2], defined_shape[3]]
    )
    reshaped_output_transposed = _tf.transpose(reshaped_output, perm=[0, 2, 1])
    multiplied_out = _tf.matmul(reshaped_output_transposed, reshaped_output)
    normalized_out = multiplied_out / _tf.cast(
        defined_shape[1] * defined_shape[2], dtype=_tf.float32
    )

    return normalized_out


def define_style_transfer_network(
    content_image, tf_index, style_image, weight_dict, define_training_graph=False
):
    """
    This function defines the style transfer network using the tensorflow nn api.

    Parameters
    ----------
    content_image: tensorflow.Tensor
        The content image to the network. The image is expected to be in RGB
        format.
    tf_index: tensorflow.Tensor
        The index tensor to the network.
    style_image: tensorflow.Tensor
        The content image to the network. The image is expected to be in RGB
        format.
    weights: dictionary
        The dictionary of weights to the network. The naming convention
        used is that from the CoreML export of the Style Transfer Network.
    define_training_graph: boolean
        If `true` both the training graph and the inference graph are returned.
        If `false` only the inference graph is returned.
    Returns
    -------
    optimizer: tensorflow.Tensor | None
        The training output tensor to the network.
    total_loss: tensorflow.Tensor | None
        The loss output tensor to the network.
    content_output: tensorflow.Tensor
        The content image output tensor to the network.
    """

    content_output = define_resnet(content_image, tf_index, weight_dict)

    optimizer = None

    if define_training_graph:
        pre_processing_output = define_vgg_pre_processing(content_output)
        output_relu_1, output_relu_2, output_relu_3, output_relu_4 = define_vgg16(
            pre_processing_output, weight_dict
        )

        pre_processing_style = define_vgg_pre_processing(style_image)
        style_relu_1, style_relu_2, style_relu_3, style_relu_4 = define_vgg16(
            pre_processing_style, weight_dict
        )

        pre_processing_content = define_vgg_pre_processing(content_image)
        _, _, content_relu_3, _ = define_vgg16(pre_processing_content, weight_dict)

        gram_output_relu_1 = define_gram_matrix(output_relu_1)
        gram_output_relu_2 = define_gram_matrix(output_relu_2)
        gram_output_relu_3 = define_gram_matrix(output_relu_3)
        gram_output_relu_4 = define_gram_matrix(output_relu_4)

        gram_style_relu_1 = define_gram_matrix(style_relu_1)
        gram_style_relu_2 = define_gram_matrix(style_relu_2)
        gram_style_relu_3 = define_gram_matrix(style_relu_3)
        gram_style_relu_4 = define_gram_matrix(style_relu_4)

        # L2 Loss Between the Nodes
        style_loss_1 = _tf.losses.mean_squared_error(
            gram_style_relu_1, gram_output_relu_1, weights=1e-4
        )
        style_loss_2 = _tf.losses.mean_squared_error(
            gram_style_relu_2, gram_output_relu_2, weights=1e-4
        )
        style_loss_3 = _tf.losses.mean_squared_error(
            gram_style_relu_3, gram_output_relu_3, weights=1e-4
        )
        style_loss_4 = _tf.losses.mean_squared_error(
            gram_style_relu_4, gram_output_relu_4, weights=1e-4
        )

        content_loss = _tf.losses.mean_squared_error(content_relu_3, output_relu_3)
        style_loss = style_loss_1 + style_loss_2 + style_loss_3 + style_loss_4

        total_loss = ((content_loss + style_loss) / 10000.0) * 0.5

        optimizer = _tf.train.AdamOptimizer().minimize(total_loss)

        return optimizer, total_loss, content_output
    else:
        return None, None, content_output


class StyleTransferTensorFlowModel(TensorFlowModel):
    def __init__(self, config, net_params):

        self.gpu_policy = _utils.TensorFlowGPUPolicy()
        self.gpu_policy.start()

        for key in net_params.keys():
            net_params[key] = _utils.convert_shared_float_array_to_numpy(
                net_params[key]
            )

        for key in config.keys():
            config[key] = _utils.convert_shared_float_array_to_numpy(config[key])

        self.st_graph = _tf.Graph()
        self._batch_size = 1
        self._finetune_all_params = True
        self._define_training_graph = bool(config["st_training"])

        self.sess = _tf.Session(graph=self.st_graph)
        with self.st_graph.as_default():
            self.init_style_transfer_graph(net_params)

    def init_style_transfer_graph(self, net_params):
        self._tf_variables = define_tensorflow_variables(net_params)

        # TODO: take care of batch size
        self.tf_input = _tf.placeholder(dtype=_tf.float32, shape=[None, None, None, 3])
        self.tf_style = _tf.placeholder(dtype=_tf.float32, shape=[None, 256, 256, 3])
        self.tf_index = _tf.placeholder(dtype=_tf.int64, shape=[self.batch_size])

        self.__define_graph()

        init = _tf.global_variables_initializer()
        self.sess.run(init)

    def __del__(self):
        # If a base class has a __del__() method, the derived class’s __del__() method,
        # if any, must explicitly call it to ensure proper deletion of # the base class
        # part of the instance.
        # That being said, no __del__() method is in base classes, we should be good.
        self.sess.close()
        self.gpu_policy.stop()

    def __define_graph(self):
        self.optimizer, self.loss, self.output = define_style_transfer_network(
            self.tf_input,
            self.tf_index,
            self.tf_style,
            self._tf_variables,
            self._define_training_graph,
        )

    @property
    def batch_size(self):
        return self._batch_size

    @batch_size.setter
    def batch_size(self, batch_size):
        self._batch_size = batch_size
        with self.st_graph.as_default():
            self.tf_index = _tf.placeholder(dtype=_tf.int32, shape=[batch_size])
            self.__define_graph()

    def train(self, feed_dict):
        for key in feed_dict.keys():
            feed_dict[key] = _utils.convert_shared_float_array_to_numpy(feed_dict[key])

        _, loss_value = self.sess.run(
            fetches=[self.optimizer, self.loss],
            feed_dict={
                self.tf_input: feed_dict["input"],
                self.tf_index: feed_dict["index"],
                self.tf_style: feed_dict["labels"],
            },
        )

        return {"loss": _np.array(loss_value)}

    def predict(self, feed_dict):
        for key in feed_dict.keys():
            feed_dict[key] = _utils.convert_shared_float_array_to_numpy(feed_dict[key])

        with self.st_graph.as_default():
            stylized_image = self.sess.run(
                fetches=[self.output],
                feed_dict={
                    self.tf_input: feed_dict["input"],
                    self.tf_index: feed_dict["index"],
                },
            )

        stylized_raw = _np.array(stylized_image)

        expected_height = feed_dict["input"].shape[1]
        expected_width = feed_dict["input"].shape[2]

        # Crop to remove added padding
        stylized_cropped = stylized_raw[:, :, 0:expected_height, 0:expected_width, :][0]

        return {"output": _np.array(stylized_cropped)}

    def export_weights(self):
        tf_export_params = {}

        with self.st_graph.as_default():
            tvars = _tf.trainable_variables()
            tvars_vals = self.sess.run(tvars)

        for var, val in zip(tvars, tvars_vals):
            if "weight" in var.name:
                if "conv" in var.name:
                    tf_export_params[
                        var.name.split(":")[0]
                    ] = _utils.convert_conv2d_tf_to_coreml(val)
                else:
                    tf_export_params[
                        var.name.split(":")[0]
                    ] = _utils.convert_dense_tf_to_coreml(val)
            else:
                tf_export_params[var.name.split(":")[0]] = _np.array(val)
        for layer_name in tf_export_params.keys():
            tf_export_params[layer_name] = _np.ascontiguousarray(
                tf_export_params[layer_name]
            )
        return tf_export_params

    def set_learning_rate(self, lr):
        pass
