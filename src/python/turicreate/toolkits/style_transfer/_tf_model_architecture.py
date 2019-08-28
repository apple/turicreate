# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import tensorflow as _tf

# TODO: change the weight dictionary key names
def define_resnet(tf_input, weights, prefix="resnet_", finetune_all_params=True):
    """ 
    This function defines the resnet network using the tensorflow nn api.
  
    Parameters
    ----------

    tf_input: tensorflow.Tensor
        The input tensor to the network. The image is expected to be in RGB
        format.

    weights: dictionary
        The dictionary of MxNet weights to the network. The naming convention
        used is that from the CoreML export of the Style Transfer Network.
    
    prefix: string
        The prefix column is used to prefix the variables of the network for
        weight export.

    finetune_all_params: boolean
        If `true` the network updates the convolutional layers as well as the
        instance norm layers of the network. If `false` only the instance norm
        layers of the network are updated.

    Returns
    -------

    out: tensorflow.Tensor
        The sigmoid output tensor to the network.

    """

    # encoding 1
    conv_1_filter = _tf.get_variable(prefix + 'conv_1_weights', initializer=weights["simplenetwork_conv0_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    conv_1 = _tf.nn.conv2d(tf_input, conv_1_filter, strides=[1, 1, 1, 1], padding='SAME')
    inst_1 = _tf.contrib.layers.instance_norm(conv_1)
    relu_1 = _tf.nn.relu(inst_1)
    
    # encoding 2
    conv_2_filter = _tf.get_variable(prefix + 'conv_2_weights', initializer=weights["simplenetwork_conv1_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    conv_2 = _tf.nn.conv2d(relu_1, conv_2_filter, strides=[1, 2, 2, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    inst_2 = _tf.contrib.layers.instance_norm(conv_2)
    relu_2 = _tf.nn.relu(inst_2)
    
    # encoding 3
    conv_3_filter = _tf.get_variable(prefix + 'conv_3_weights', initializer=weights["simplenetwork_conv2_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    conv_3 = _tf.nn.conv2d(relu_2, conv_3_filter, strides=[1, 2, 2, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    inst_3 = _tf.contrib.layers.instance_norm(conv_3)
    relu_3 = _tf.nn.relu(inst_3)
    
    # residual 1
    residual_1_conv_1_filter = _tf.get_variable(prefix + 'residual_1_conv1', initializer=weights["simplenetwork_conv3_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    residual_1_conv_1 = _tf.nn.conv2d(relu_3, residual_1_conv_1_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    residual_1_inst_1 = _tf.contrib.layers.instance_norm(residual_1_conv_1)
    residual_1_relu_1 = _tf.nn.relu(residual_1_inst_1)
    
    residual_1_conv_2_filter = _tf.get_variable(prefix + 'residual_1_conv2', initializer=weights["simplenetwork_conv4_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    residual_1_conv_2 = _tf.nn.conv2d(residual_1_relu_1, residual_1_conv_2_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    residual_1_inst_2 = _tf.contrib.layers.instance_norm(residual_1_conv_2)
    
    residual_1_add = _tf.add(relu_3, residual_1_inst_2)
    
    # residual 2
    residual_2_conv_1_filter = _tf.get_variable(prefix + 'residual_2_conv1', initializer=weights["simplenetwork_conv5_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    residual_2_conv_1 = _tf.nn.conv2d(residual_1_add, residual_2_conv_1_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    residual_2_inst_1 = _tf.contrib.layers.instance_norm(residual_2_conv_1)
    residual_2_relu_1 = _tf.nn.relu(residual_2_inst_1)
    
    residual_2_conv_2_filter = _tf.get_variable(prefix + 'residual_2_conv2', initializer=weights["simplenetwork_conv6_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    residual_2_conv_2 = _tf.nn.conv2d(residual_2_relu_1, residual_2_conv_2_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    residual_2_inst_2 = _tf.contrib.layers.instance_norm(residual_2_conv_2)
    
    residual_2_add = _tf.add(residual_1_add, residual_2_inst_2)
    
    # residual 3
    residual_3_conv_1_filter = _tf.get_variable(prefix + 'residual_3_conv1', initializer=weights["simplenetwork_conv7_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    residual_3_conv_1 = _tf.nn.conv2d(residual_2_add, residual_3_conv_1_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    residual_3_inst_1 = _tf.contrib.layers.instance_norm(residual_3_conv_1)
    residual_3_relu_1 = _tf.nn.relu(residual_3_inst_1)
    
    residual_3_conv_2_filter = _tf.get_variable(prefix + 'residual_3_conv2', initializer=weights["simplenetwork_conv8_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    residual_3_conv_2 = _tf.nn.conv2d(residual_3_relu_1, residual_3_conv_2_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    residual_3_inst_2 = _tf.contrib.layers.instance_norm(residual_3_conv_2)
    
    residual_3_add = _tf.add(residual_2_add, residual_3_inst_2)
    
    # residual 4
    residual_4_conv_1_filter = _tf.get_variable(prefix + 'residual_4_conv1', initializer=weights["simplenetwork_conv9_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    residual_4_conv_1 = _tf.nn.conv2d(residual_3_add, residual_4_conv_1_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    residual_4_inst_1 = _tf.contrib.layers.instance_norm(residual_4_conv_1)
    residual_4_relu_1 = _tf.nn.relu(residual_4_inst_1)
    
    residual_4_conv_2_filter = _tf.get_variable(prefix + 'residual_4_conv2', initializer=weights["simplenetwork_conv10_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    residual_4_conv_2 = _tf.nn.conv2d(residual_4_relu_1, residual_4_conv_2_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    residual_4_inst_2 = _tf.contrib.layers.instance_norm(residual_4_conv_2)
    
    residual_4_add = _tf.add(residual_3_add, residual_4_inst_2)
    
     # residual 5
    residual_5_conv_1_filter = _tf.get_variable(prefix + 'residual_5_conv1', initializer=weights["simplenetwork_conv11_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    residual_5_conv_1 = _tf.nn.conv2d(residual_4_add, residual_5_conv_1_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    residual_5_inst_1 = _tf.contrib.layers.instance_norm(residual_5_conv_1)
    residual_5_relu_1 = _tf.nn.relu(residual_5_inst_1)
    
    residual_5_conv_2_filter = _tf.get_variable(prefix + 'residual_5_conv2', initializer=weights["simplenetwork_conv12_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    residual_5_conv_2 = _tf.nn.conv2d(residual_5_relu_1, residual_5_conv_2_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    residual_5_inst_2 = _tf.contrib.layers.instance_norm(residual_5_conv_2)
    
    residual_5_add = _tf.add(residual_4_add, residual_5_inst_2)
    
    # decode 1
    decode_1_image_shape = _tf.shape(residual_5_add)
    decode_1_new_height = decode_1_image_shape[1] * 2
    decode_1_new_width = decode_1_image_shape[2] * 2
    
    decoding_1_upsample_1 = _tf.image.resize_images(residual_5_add, [decode_1_new_height, decode_1_new_width], method=_tf.image.ResizeMethod.NEAREST_NEIGHBOR)
    decode_1_conv_1_filter = _tf.get_variable(prefix + 'decode_1_conv_1', initializer=weights["simplenetwork_conv13_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    decode_1_conv_1 = _tf.nn.conv2d(decoding_1_upsample_1, decode_1_conv_1_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    decode_1_inst_1 = _tf.contrib.layers.instance_norm(decode_1_conv_1)
    decode_1_relu_1 = _tf.nn.relu(decode_1_inst_1)
    
    # decode 2
    decode_2_image_shape = _tf.shape(decode_1_relu_1)
    decode_2_new_height = decode_2_image_shape[1] * 2
    decode_2_new_width = decode_2_image_shape[2] * 2
    
    decoding_2_upsample_1 = _tf.image.resize_images(decode_1_relu_1, [decode_2_new_height, decode_2_new_width], method=_tf.image.ResizeMethod.NEAREST_NEIGHBOR)
    decode_2_conv_1_filter = _tf.get_variable(prefix + 'decode_2_conv_1', initializer=weights["simplenetwork_conv14_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    decode_2_conv_1 = _tf.nn.conv2d(decoding_2_upsample_1, decode_2_conv_1_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    decode_2_inst_1 = _tf.contrib.layers.instance_norm(decode_2_conv_1)
    decode_2_relu_1 = _tf.nn.relu(decode_2_inst_1)
    
    # decode 3
    decode_3_conv_1_filter = _tf.get_variable(prefix + 'decode_3_conv_1', initializer=weights["simplenetwork_conv15_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    decode_3_conv_1 = _tf.nn.conv2d(decode_2_relu_1, decode_3_conv_1_filter, strides=[1, 1, 1, 1], padding='SAME')
    decode_3_inst_1 = _tf.contrib.layers.instance_norm(decode_3_conv_1)
    decode_3_relu_1 = _tf.nn.sigmoid(decode_3_inst_1)
    
    return decode_3_relu_1

# TODO: change the weight dictionary key names
def define_vgg16(tf_input, weights, prefix="vgg16_"):
    """ 
    This function defines the vgg16 network using the tensorflow nn api.
  
    Parameters
    ----------

    tf_input: tensorflow.Tensor
        The input tensor to the network. The image is expected to be in RGB
        format

    weights: dictionary
        The dictionary of MxNet weights to the network. The naming convention
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
    conv_1_filter = _tf.get_variable(prefix + 'conv_1_weights', initializer=weights["vgg16_conv0_weight"], trainable=False, dtype=_tf.float32)
    conv_1 = __tf.nn.conv2d(tf_input, conv_1_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_1_bias = _tf.get_variable(prefix + 'conv_1_bias', initializer=weights["vgg16_conv0_bias"], trainable=False, dtype=_tf.float32)
    conv_1 = _tf.nn.bias_add(conv_1, conv_1_bias)
    relu_1 = _tf.nn.relu(conv_1)
    
    conv_2_filter = _tf.get_variable(prefix + 'conv_2_weights', initializer=weights["vgg16_conv1_weight"], trainable=False, dtype=_tf.float32)
    conv_2 = _tf.nn.conv2d(relu_1, conv_2_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_2_bias = _tf.get_variable(prefix + 'conv_2_bias', initializer=weights["vgg16_conv1_bias"], trainable=False, dtype=_tf.float32)
    conv_2 = _tf.nn.bias_add(conv_2, conv_2_bias)
    relu_2 = _tf.nn.relu(conv_2)
    
    pool_1 = _tf.nn.avg_pool(relu_2, 2, 2, 'SAME')
    
    # block 2
    conv_3_filter = _tf.get_variable(prefix + 'conv_3_weights', initializer=weights["vgg16_conv2_weight"], trainable=False, dtype=_tf.float32)
    conv_3 = _tf.nn.conv2d(pool_1, conv_3_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_3_bias = _tf.get_variable(prefix + 'conv_3_bias', initializer=weights["vgg16_conv2_bias"], trainable=False, dtype=_tf.float32)
    conv_3 = _tf.nn.bias_add(conv_3, conv_3_bias)
    relu_3 = _tf.nn.relu(conv_3)
    
    conv_4_filter = _tf.get_variable(prefix + 'conv_4_weights', initializer=weights["vgg16_conv3_weight"], trainable=False, dtype=_tf.float32)
    conv_4 = _tf.nn.conv2d(relu_3, conv_4_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_4_bias = _tf.get_variable(prefix + 'conv_4_bias', initializer=weights["vgg16_conv3_bias"], trainable=False, dtype=_tf.float32)
    conv_4 = _tf.nn.bias_add(conv_4, conv_4_bias)
    relu_4 = _tf.nn.relu(conv_4)
    
    pool_2 = _tf.nn.avg_pool(relu_4, 2, 2, 'SAME')
    
    # block 3
    conv_5_filter = _tf.get_variable(prefix + 'conv_5_weights', initializer=weights["vgg16_conv4_weight"], trainable=False, dtype=_tf.float32)
    conv_5 = _tf.nn.conv2d(pool_2, conv_5_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_5_bias = _tf.get_variable(prefix + 'conv_5_bias', initializer=weights["vgg16_conv4_bias"], trainable=False, dtype=_tf.float32)
    conv_5 = _tf.nn.bias_add(conv_5, conv_5_bias)
    relu_5 = _tf.nn.relu(conv_5)
    
    conv_6_filter = _tf.get_variable(prefix + 'conv_6_weights', initializer=weights["vgg16_conv5_weight"], trainable=False, dtype=_tf.float32)
    conv_6 = _tf.nn.conv2d(relu_5, conv_6_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_6_bias = _tf.get_variable(prefix + 'conv_6_bias', initializer=weights["vgg16_conv5_bias"], trainable=False, dtype=_tf.float32)
    conv_6 = _tf.nn.bias_add(conv_6, conv_6_bias)
    relu_6 = _tf.nn.relu(conv_6)
    
    conv_7_filter = _tf.get_variable(prefix + 'conv_7_weights', initializer=weights["vgg16_conv6_weight"], trainable=False, dtype=_tf.float32)
    conv_7 = _tf.nn.conv2d(relu_6, conv_7_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_7_bias = _tf.get_variable(prefix + 'conv_7_bias', initializer=weights["vgg16_conv6_bias"], trainable=False, dtype=_tf.float32)
    conv_7 = _tf.nn.bias_add(conv_7, conv_7_bias)
    relu_7 = _tf.nn.relu(conv_7)
    
    pool_3 = _tf.nn.avg_pool(relu_7, 2, 2, 'SAME')
    
    # block 4
    conv_8_filter = _tf.get_variable(prefix + 'conv_8_weights', initializer=weights["vgg16_conv7_weight"], trainable=False, dtype=_tf.float32)
    conv_8 = _tf.nn.conv2d(pool_3, conv_8_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_8_bias = _tf.get_variable(prefix + 'conv_8_bias', initializer=weights["vgg16_conv7_bias"], trainable=False, dtype=_tf.float32)
    conv_8 = _tf.nn.bias_add(conv_8, conv_8_bias)
    relu_8 = _tf.nn.relu(conv_8)
    
    conv_9_filter = _tf.get_variable(prefix + 'conv_9_weights', initializer=weights["vgg16_conv8_weight"], trainable=False, dtype=_tf.float32)
    conv_9 = _tf.nn.conv2d(relu_8, conv_9_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_9_bias = _tf.get_variable(prefix + 'conv_9_bias', initializer=weights["vgg16_conv8_bias"], trainable=False, dtype=_tf.float32)
    conv_9 = _tf.nn.bias_add(conv_9, conv_9_bias)
    relu_9 = _tf.nn.relu(conv_9)
    
    conv_10_filter = _tf.get_variable(prefix + 'conv_10_weights', initializer=weights["vgg16_conv9_weight"], trainable=False, dtype=_tf.float32)
    conv_10 = _tf.nn.conv2d(relu_9, conv_10_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_10_bias = _tf.get_variable(prefix + 'conv_10_bias', initializer=weights["vgg16_conv9_bias"], trainable=False, dtype=_tf.float32)
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
    
    sub_scalar = _tf.concat([red_channel_scaled, green_channel_scaled, blue_channel_scaled], 3)
    
    return sub_scalar

# TODO: Extend from TensorFlowModel
class StyleTransferTensorFlowModel():
    def __init__(self, net_params, batch_size, num_styles):
        # TODO: populate
        pass
    def train(self, feed_dict):
        # TODO: populate
        pass
    def predict(self, feed_dict):
        # TODO: populate
        pass
    def export_weights(self):
        # TODO: populate
        pass
    def set_learning_rate(self, lr):
        # TODO: populate
        pass