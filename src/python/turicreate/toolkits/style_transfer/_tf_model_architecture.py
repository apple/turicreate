# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import tensorflow as _tf

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
        The dictionary of MxNet weights to the network. The naming convention
        used is that from the CoreML export of the Style Transfer Network.

    Returns
    -------

    out: tensorflow.Tensor
        The instance norm output tensor to the network.
    """
    inputs_shape = tf_input.shape
    inputs_rank = tf_input.shape.ndims
    epsilon=1e-5
    
    reduction_axis = inputs_rank - 1
    params_shape = inputs_shape[reduction_axis:reduction_axis + 1]
    
    moments_axes = list(range(inputs_rank))
    del moments_axes[reduction_axis]
    del moments_axes[0]
    
    mean, variance = _tf.nn.moments(tf_input, moments_axes, keep_dims=True)

    gamma = _tf.compat.v1.get_variable(prefix + 'gamma', initializer=weights[prefix + "gamma"], dtype=_tf.float32)
    beta = _tf.compat.v1.get_variable(prefix + 'beta', initializer=weights[prefix + "beta"], dtype=_tf.float32)
    
    gamma_sliced = _tf.split(gamma, gamma.shape[0], axis=0)
    beta_sliced = _tf.split(beta, beta.shape[0], axis=0)
    
    batch_norm_array = []
    for b, g in zip(beta_sliced, gamma_sliced):
        style_bn = _tf.nn.batch_normalization(tf_input, mean, variance, b, g, epsilon)
        style_bn = _tf.expand_dims(style_bn, 0)
        batch_norm_array.append(style_bn)
        
    batch_norm_concat = _tf.concat(batch_norm_array, 0)
    picked_index = _tf.gather_nd(batch_norm_concat, tf_index)
    picked_index = _tf.reshape(picked_index, [-1] + picked_index.get_shape().as_list()[2:])
    
    return picked_index

def define_residual(tf_input, tf_index, weights, prefix, finetune_all_params=True):
    """ 
    This function defines the residual network using the tensorflow nn api.
  
    Parameters
    ----------

    tf_input: tensorflow.Tensor
        The input tensor to the residual network.

    tf_index: tensorflow.Tensor
        The index tensor to the residual network.

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
    # TODO: Refactor Instance Norm
    conv_1_filter = _tf.get_variable(prefix + 'conv0_weight', initializer=weights[prefix + "conv0_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    conv_1 = _tf.nn.conv2d(tf_input, conv_1_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    inst_1 = define_instance_norm(conv_1, tf_index, weights, prefix + "instancenorm0_")
    relu_1 = _tf.nn.relu(inst_1)
    
    conv_2_filter = _tf.get_variable(prefix + 'conv1_weight', initializer=weights[prefix + "conv1_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    conv_2 = _tf.nn.conv2d(relu_1, conv_2_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    inst_2 = define_instance_norm(conv_2, tf_index, weights, prefix + "instancenorm1_")
    
    return _tf.add(tf_input, inst_2)

def define_resnet(tf_input,
                 tf_index,
                 weights,
                 prefix="transformer_",
                 finetune_all_params=True):
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
    conv_1_filter = _tf.get_variable(prefix + 'conv0_weight', initializer=weights[prefix + "conv0_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    conv_1 = _tf.nn.conv2d(tf_input, conv_1_filter, strides=[1, 1, 1, 1], padding='SAME')
    inst_1 = define_instance_norm(conv_1, tf_index, weights, prefix + "instancenorm0_")
    relu_1 = _tf.nn.relu(inst_1)
    
    # encoding 2
    conv_2_filter = _tf.get_variable(prefix + 'conv1_weight', initializer=weights[prefix + "conv1_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    conv_2 = _tf.nn.conv2d(relu_1, conv_2_filter, strides=[1, 2, 2, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    inst_2 = define_instance_norm(conv_2, tf_index, weights, prefix + "instancenorm1_")
    relu_2 = _tf.nn.relu(inst_2)
    
    # encoding 3
    conv_3_filter = _tf.get_variable(prefix + 'conv2_weight', initializer=weights[prefix + "conv2_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    conv_3 = _tf.nn.conv2d(relu_2, conv_3_filter, strides=[1, 2, 2, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    inst_3 = define_instance_norm(conv_3, tf_index, weights, prefix + "instancenorm2_")
    relu_3 = _tf.nn.relu(inst_3)
    
    # Residual Blocks
    residual_1 = define_residual(relu_3, tf_index, weights, prefix + "residualblock0_", finetune_all_params=finetune_all_params)
    residual_2 = define_residual(residual_1, tf_index, weights, prefix + "residualblock1_", finetune_all_params=finetune_all_params)
    residual_3 = define_residual(residual_2, tf_index, weights, prefix + "residualblock2_", finetune_all_params=finetune_all_params)
    residual_4 = define_residual(residual_3, tf_index, weights, prefix + "residualblock3_", finetune_all_params=finetune_all_params)
    residual_5 = define_residual(residual_4, tf_index, weights, prefix + "residualblock4_", finetune_all_params=finetune_all_params)
    
    # decode 1
    decode_1_image_shape = _tf.shape(residual_5)
    decode_1_new_height = decode_1_image_shape[1] * 2
    decode_1_new_width = decode_1_image_shape[2] * 2
    
    decoding_1_upsample_1 = _tf.image.resize_images(residual_5, [decode_1_new_height, decode_1_new_width], method=_tf.image.ResizeMethod.NEAREST_NEIGHBOR)
    decode_1_conv_1_filter = _tf.get_variable(prefix + 'conv3_weight', initializer=weights[prefix + "conv3_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    decode_1_conv_1 = _tf.nn.conv2d(decoding_1_upsample_1, decode_1_conv_1_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    decode_1_inst_1 = define_instance_norm(decode_1_conv_1, tf_index, weights, prefix + "instancenorm3_")
    decode_1_relu_1 = _tf.nn.relu(decode_1_inst_1)
    
    # decode 2
    decode_2_image_shape = _tf.shape(decode_1_relu_1)
    decode_2_new_height = decode_2_image_shape[1] * 2
    decode_2_new_width = decode_2_image_shape[2] * 2
    
    decoding_2_upsample_1 = _tf.image.resize_images(decode_1_relu_1, [decode_2_new_height, decode_2_new_width], method=_tf.image.ResizeMethod.NEAREST_NEIGHBOR)
    decode_2_conv_1_filter = _tf.get_variable(prefix + 'conv4_weight', initializer=weights[prefix + "conv4_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    decode_2_conv_1 = _tf.nn.conv2d(decoding_2_upsample_1, decode_2_conv_1_filter, strides=[1, 1, 1, 1], padding=[[0, 0], [1, 1], [1, 1], [0, 0]])
    decode_2_inst_1 = define_instance_norm(decode_2_conv_1, tf_index, weights, prefix + "instancenorm4_")
    decode_2_relu_1 = _tf.nn.relu(decode_2_inst_1)
    
    # decode 3
    decode_3_conv_1_filter = _tf.get_variable(prefix + 'conv5_weight', initializer=weights[prefix + "conv5_weight"], trainable=finetune_all_params, dtype=_tf.float32)
    decode_3_conv_1 = _tf.nn.conv2d(decode_2_relu_1, decode_3_conv_1_filter, strides=[1, 1, 1, 1], padding='SAME')    
    decode_3_inst_1 = define_instance_norm(decode_3_conv_1, tf_index, weights, prefix + "instancenorm5_")
    decode_3_relu_1 = _tf.nn.sigmoid(decode_3_inst_1)
    
    return decode_3_relu_1

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
    conv_1_filter = _tf.get_variable(prefix + "conv0_weight", initializer=weights["vgg16_conv0_weight"], trainable=False, dtype=_tf.float32)
    conv_1 = __tf.nn.conv2d(tf_input, conv_1_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_1_bias = _tf.get_variable(prefix + 'conv0_bias', initializer=weights["vgg16_conv0_bias"], trainable=False, dtype=_tf.float32)
    conv_1 = _tf.nn.bias_add(conv_1, conv_1_bias)
    relu_1 = _tf.nn.relu(conv_1)
    
    conv_2_filter = _tf.get_variable(prefix + "conv1_weight", initializer=weights["vgg16_conv1_weight"], trainable=False, dtype=_tf.float32)
    conv_2 = _tf.nn.conv2d(relu_1, conv_2_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_2_bias = _tf.get_variable(prefix + "conv1_bias", initializer=weights["vgg16_conv1_bias"], trainable=False, dtype=_tf.float32)
    conv_2 = _tf.nn.bias_add(conv_2, conv_2_bias)
    relu_2 = _tf.nn.relu(conv_2)
    
    pool_1 = _tf.nn.avg_pool(relu_2, 2, 2, 'SAME')
    
    # block 2
    conv_3_filter = _tf.get_variable(prefix + "conv2_weight", initializer=weights["vgg16_conv2_weight"], trainable=False, dtype=_tf.float32)
    conv_3 = _tf.nn.conv2d(pool_1, conv_3_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_3_bias = _tf.get_variable(prefix + "conv2_bias", initializer=weights["vgg16_conv2_bias"], trainable=False, dtype=_tf.float32)
    conv_3 = _tf.nn.bias_add(conv_3, conv_3_bias)
    relu_3 = _tf.nn.relu(conv_3)
    
    conv_4_filter = _tf.get_variable(prefix + "conv3_weight", initializer=weights["vgg16_conv3_weight"], trainable=False, dtype=_tf.float32)
    conv_4 = _tf.nn.conv2d(relu_3, conv_4_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_4_bias = _tf.get_variable(prefix + "conv3_bias", initializer=weights["vgg16_conv3_bias"], trainable=False, dtype=_tf.float32)
    conv_4 = _tf.nn.bias_add(conv_4, conv_4_bias)
    relu_4 = _tf.nn.relu(conv_4)
    
    pool_2 = _tf.nn.avg_pool(relu_4, 2, 2, 'SAME')
    
    # block 3
    conv_5_filter = _tf.get_variable(prefix + "conv4_weight", initializer=weights["vgg16_conv4_weight"], trainable=False, dtype=_tf.float32)
    conv_5 = _tf.nn.conv2d(pool_2, conv_5_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_5_bias = _tf.get_variable(prefix + "conv4_bias", initializer=weights["vgg16_conv4_bias"], trainable=False, dtype=_tf.float32)
    conv_5 = _tf.nn.bias_add(conv_5, conv_5_bias)
    relu_5 = _tf.nn.relu(conv_5)
    
    conv_6_filter = _tf.get_variable(prefix + "conv5_weight", initializer=weights["vgg16_conv5_weight"], trainable=False, dtype=_tf.float32)
    conv_6 = _tf.nn.conv2d(relu_5, conv_6_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_6_bias = _tf.get_variable(prefix + "conv5_bias", initializer=weights["vgg16_conv5_bias"], trainable=False, dtype=_tf.float32)
    conv_6 = _tf.nn.bias_add(conv_6, conv_6_bias)
    relu_6 = _tf.nn.relu(conv_6)
    
    conv_7_filter = _tf.get_variable(prefix + "conv6_weight", initializer=weights["vgg16_conv6_weight"], trainable=False, dtype=_tf.float32)
    conv_7 = _tf.nn.conv2d(relu_6, conv_7_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_7_bias = _tf.get_variable(prefix + "conv6_bias", initializer=weights["vgg16_conv6_bias"], trainable=False, dtype=_tf.float32)
    conv_7 = _tf.nn.bias_add(conv_7, conv_7_bias)
    relu_7 = _tf.nn.relu(conv_7)
    
    pool_3 = _tf.nn.avg_pool(relu_7, 2, 2, 'SAME')
    
    # block 4
    conv_8_filter = _tf.get_variable(prefix + "conv7_weight", initializer=weights["vgg16_conv7_weight"], trainable=False, dtype=_tf.float32)
    conv_8 = _tf.nn.conv2d(pool_3, conv_8_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_8_bias = _tf.get_variable(prefix + "conv7_bias", initializer=weights["vgg16_conv7_bias"], trainable=False, dtype=_tf.float32)
    conv_8 = _tf.nn.bias_add(conv_8, conv_8_bias)
    relu_8 = _tf.nn.relu(conv_8)
    
    conv_9_filter = _tf.get_variable(prefix + "conv8_weight", initializer=weights["vgg16_conv8_weight"], trainable=False, dtype=_tf.float32)
    conv_9 = _tf.nn.conv2d(relu_8, conv_9_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_9_bias = _tf.get_variable(prefix + "conv8_bias", initializer=weights["vgg16_conv8_bias"], trainable=False, dtype=_tf.float32)
    conv_9 = _tf.nn.bias_add(conv_9, conv_9_bias)
    relu_9 = _tf.nn.relu(conv_9)
    
    conv_10_filter = _tf.get_variable(prefix + "conv9_weight", initializer=weights["vgg16_conv9_weight"], trainable=False, dtype=_tf.float32)
    conv_10 = _tf.nn.conv2d(relu_9, conv_10_filter, strides=[1, 1, 1, 1], padding='SAME')
    conv_10_bias = _tf.get_variable(prefix + "conv9_bias", initializer=weights["vgg16_conv9_bias"], trainable=False, dtype=_tf.float32)
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
    defined_shape= _tf.shape(tf_input)
    reshaped_output = _tf.reshape(tf_input, [-1, defined_shape[1] * defined_shape[2], defined_shape[3]])
    reshaped_output_transposed = _tf.transpose(reshaped_output, perm=[0, 2, 1])
    multiplied_out = _tf.matmul(reshaped_output_transposed, reshaped_output)
    normalized_out =  multiplied_out / _tf.cast(defined_shape[1] * defined_shape[2], dtype=_tf.float32)

    return normalized_out

def define_style_transfer_network(content_image,
                                  tf_index,
                                  style_image,
                                  weight_dict,
                                  finetune_all_params=False,
                                  define_training_graph=False):
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
        The dictionary of MxNet weights to the network. The naming convention
        used is that from the CoreML export of the Style Transfer Network.
    
    
    finetune_all_params: boolean
        If `true` the network updates the convolutional layers as well as the
        instance norm layers of the network. If `false` only the instance norm
        layers of the network are updated.

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

    content_output = define_transformer(content_image, tf_index, weight_dict, finetune_all_params=finetune_all_params)
    
    optimizer = None
    
    if define_training_graph:
        pre_processing_output = define_vgg_pre_processing(content_output, prefix="output_pre_processing_")
        output_relu_1, output_relu_2, output_relu_3, output_relu_4 = define_vgg16(pre_processing_output, weight_dict, prefix="output_vgg_")

        pre_processing_style = define_vgg_pre_processing(style_image, prefix="style_pre_processing")
        style_relu_1, style_relu_2, style_relu_3, style_relu_4 = define_vgg16(pre_processing_style, weight_dict, prefix="style_vgg_")

        pre_processing_content = define_vgg_pre_processing(content_image, prefix="content_pre_processing_")
        _, _, content_relu_3, _ = define_vgg16(pre_processing_content, weight_dict, prefix="content_vgg_")

        gram_output_relu_1 = define_gram_matrix(output_relu_1)
        gram_output_relu_2 = define_gram_matrix(output_relu_2)
        gram_output_relu_3 = define_gram_matrix(output_relu_3)
        gram_output_relu_4 = define_gram_matrix(output_relu_4)

        gram_style_relu_1 = define_gram_matrix(style_relu_1)
        gram_style_relu_2 = define_gram_matrix(style_relu_2)
        gram_style_relu_3 = define_gram_matrix(style_relu_3)
        gram_style_relu_4 = define_gram_matrix(style_relu_4)

        # L2 Loss Between the Nodes
        style_loss_1 = tf.losses.mean_squared_error(gram_style_relu_1, gram_output_relu_1, weights=1e-4)
        style_loss_2 = tf.losses.mean_squared_error(gram_style_relu_2, gram_output_relu_2, weights=1e-4)
        style_loss_3 = tf.losses.mean_squared_error(gram_style_relu_3, gram_output_relu_3, weights=1e-4)
        style_loss_4 = tf.losses.mean_squared_error(gram_style_relu_4, gram_output_relu_4, weights=1e-4)

        content_loss = tf.losses.mean_squared_error(content_relu_3, output_relu_3)
        style_loss = style_loss_1 + style_loss_2 + style_loss_3 + style_loss_4


        total_loss = ((content_loss + style_loss)/10000.0) * 0.5
        
        optimizer = tf.train.AdamOptimizer().minimize(total_loss)
    
    return optimizer, total_loss, content_output

# TODO: Extend from TensorFlowModel
class StyleTransferTensorFlowModel():
    def __init__(self, net_params, batch_size, num_styles):
        _tf.reset_default_graph()

        self.batch_size = batch_size

        # TODO: change to take any size input
        self.tf_input = _tf.placeholder(dtype = _tf.float32, shape = [None, 256, 256, 3])
        self.tf_style = _tf.placeholder(dtype = _tf.float32, shape = [None, 256, 256, 3])
        self.tf_index = _tf.placeholder(dtype = _tf.float32, shape = [None, 1])

        self.optimizer, self.loss, self.output = define_style_transfer_network(self.tf_input,
                                                                               self.tf_index,
                                                                               self.tf_style,
                                                                               net_params,
                                                                               finetune_all_params=False,
                                                                               define_training_graph=False)
        
        self.sess = _tf.compat.v1.Session()
        init = _tf.compat.v1.global_variables_initializer()
        self.sess.run(init)

    def train(self, feed_dict):
        _, loss_value = self.sess.run([self.optimizer, self.loss], feed_dict=feed_dict)
        return { "loss": loss_value }

    def predict(self, feed_dict):
        stylized_image = self.sess.run([self.output], feed_dict=feed_dict)
        return { "stylized_image": stylized_image }

    def export_weights(self):
        tf_keys = _tf.trainable_variables()
        tf_weights = self.sess.run(tf_keys)

        weight_dictionary = dict()
        for key, weight in zip(tf_keys, tf_weights):
            weight_dictionary[key.name] = weight

        return weight_dictionary

    def set_learning_rate(self, lr):
        pass