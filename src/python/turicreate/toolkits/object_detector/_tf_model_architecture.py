# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import tensorflow as _tf
import numpy as _np
import turicreate.toolkits._tf_utils as _utils

class ODTensorFlowModel(object):

    def __init__(self, input_h, input_w, batch_size, output_size, init_weights, config, is_train=True):

        #_tf.compat.v1.get_default_graph()

        # Converting incoming weights from shared_float_array to numpy
        for key in init_weights.keys():
            init_weights[key] = _utils.convert_shared_float_array_to_numpy(init_weights[key])

        self.config = config
        self.batch_size = batch_size
        self.grid_shape = [13,13]
        self.num_classes = int(_utils.convert_shared_float_array_to_numpy(config['num_classes']))
        self.anchors = [
            (1.0, 2.0), (1.0, 1.0), (2.0, 1.0),
            (2.0, 4.0), (2.0, 2.0), (4.0, 2.0),
            (4.0, 8.0), (4.0, 4.0), (8.0, 4.0),
            (8.0, 16.0), (8.0, 8.0), (16.0, 8.0),
            (16.0, 32.0), (16.0, 16.0), (32.0, 16.0),
        ]
        self.num_anchors = len(self.anchors)
        self.output_size = output_size
        self.is_train = is_train  # Set flag for training or val

        # Create placeholders for image and labels
        self.images = _tf.compat.v1.placeholder(_tf.float32, [self.batch_size, input_h,
                                                              input_w, 3], name='images')
        self.labels = _tf.compat.v1.placeholder(_tf.float32,
                                [self.batch_size, self.grid_shape[0], self.grid_shape[1],
                                 self.num_anchors, self.num_classes + 5],
                                name='labels')
        self.init_weights = init_weights
        self.tf_model = self.tiny_yolo(inputs=self.images, output_size=self.output_size)
        self.global_step = _tf.Variable(0, trainable=False,
                                        name="global_step")

        self.loss = self.loss_layer(self.tf_model, self.labels)
        self.base_lr = _utils.convert_shared_float_array_to_numpy(config['learning_rate'])
        self.num_iterations = int(_utils.convert_shared_float_array_to_numpy(config['num_iterations']))
        self.init_steps = [self.num_iterations // 2, 3 * self.num_iterations // 4, self.num_iterations]
        self.lrs = [_np.float32(self.base_lr * 10 ** (-i)) for i, step in enumerate(self.init_steps)]
        self.steps_tf = self.init_steps[:-1]
        self.lr = _tf.compat.v1.train.piecewise_constant(self.global_step, self.steps_tf, self.lrs)
        # TODO: Evaluate method to update lr in set_learning_rate()

        self.opt = _tf.compat.v1.train.MomentumOptimizer(self.lr, momentum=0.9)

        self.clip_value = _utils.convert_shared_float_array_to_numpy(self.config.get('gradient_clipping'))

        grads_and_vars = self.opt.compute_gradients(self.loss)
        clipped_gradients = [(self.ClipIfNotNone(g, self.clip_value), v) for g, v in grads_and_vars]
        self.train_op = self.opt.apply_gradients(clipped_gradients, global_step=self.global_step)


        self.sess = _tf.compat.v1.Session()
        self.sess.run(_tf.compat.v1.global_variables_initializer())
        self.sess.run(_tf.compat.v1.local_variables_initializer())

        self.load_weights(self.init_weights)

    def load_weights(self, tf_net_params):
        """
        Function to load MXNet weights into TensorFlow

        Parameters
        ----------
        tf_net_params: Dictionary
            Dict with MXNet weights and names

        """
        for keys in tf_net_params:
            if tf_net_params[keys].ndim == 1:
                self.sess.run(_tf.compat.v1.assign(_tf.compat.v1.get_default_graph().get_tensor_by_name(keys+":0"),
                                                   tf_net_params[keys]))
            elif tf_net_params[keys].ndim == 4:
                # Converting from [output_channels, input_channels, filter_height, filter_width] to
                # [filter_height, filter_width, input_channels, output_channels]
                self.sess.run(_tf.compat.v1.assign(_tf.compat.v1.get_default_graph().get_tensor_by_name(keys+":0"),
                                                   tf_net_params[keys].transpose(2, 3, 1, 0)))
            else:
                continue

    def ClipIfNotNone(self, grad, clip_value):
        """
        Function to check if grad value is None. If not, continue with clipping.
        """
        if grad is None:
            return grad
        return _tf.clip_by_value(grad, -clip_value, clip_value)

    def batch_norm_wrapper(self,
                           inputs,
                           batch_name,
                           is_training=True,
                           epsilon=1e-05,
                           decay=0.9):
        """
        Layer to handle batch norm training and inference

        Parameters
        ----------
        inputs: TensorFlow Tensor
            4d tensor of NHWC format
        batch_name: string
            Name for the batch norm layer
        is_training: bool
            True if training and False if running validation; updates based on is_train from params
        epsilon: float
            Small, non-zero value added to variance to avoid divide-by-zero error
        decay: float
            Decay for the moving average


        Returns
        -------
        return: TensorFlow Tensor
            Result of batch norm layer
        """
        dim_of_x = inputs.get_shape()[-1]

        shadow_mean = _tf.Variable(_tf.zeros(shape=[dim_of_x], dtype='float32'),
                                   name=batch_name + 'running_mean', trainable=False)

        shadow_var = _tf.Variable(_tf.ones(shape=[dim_of_x], dtype='float32'),
                                  name=batch_name + 'running_var', trainable=False)
        axes = list(range(len(inputs.get_shape()) - 1))

        # Calculate mean and variance for a batch
        batch_mean, batch_var = _tf.nn.moments(inputs, axes, name='moments')

        def mean_var_update():
            with _tf.control_dependencies([_tf.compat.v1.assign(shadow_mean, _tf.multiply(shadow_mean, decay)
                                                                             + _tf.multiply(batch_mean, 1. - decay)),
                                           _tf.compat.v1.assign(shadow_var, _tf.multiply(shadow_var, decay)
                                                                            + _tf.multiply(batch_var, 1. - decay))]):
                return _tf.identity(batch_mean), _tf.identity(batch_var)

        mean, variance = _tf.cond(_tf.cast(is_training, _tf.bool), mean_var_update,
                                  lambda:(_tf.identity(shadow_mean), _tf.identity(shadow_var)))
        beta = _tf.Variable(_tf.zeros(shape=dim_of_x, dtype='float32'),
                            name=batch_name + 'beta', trainable=True)  #Offset/Shift
        gamma = _tf.Variable(_tf.ones(shape=dim_of_x, dtype='float32'),
                             name=batch_name + 'gamma', trainable=True)  #Scale

        return _tf.nn.batch_normalization(inputs, mean, variance, beta, gamma, epsilon)

    def conv_layer(self, inputs, shape, name, batch_name, batch_norm=True):
        """
        Defines conv layer, batch norm and leaky ReLU

        Parameters
        ----------
        inputs: TensorFlow Tensor
            4d tensor of NHWC format
        shape: TensorFlow Tensor
            Shape of the conv layer
        batch_norm: Bool
            (True or False) to add batch norm layer. This is used to add batch norm to all conv layers but the last.
        name: string
            Name for the conv layer
        batch_name: string
            Name for the batch norm layer

        Returns
        -------
        conv: TensorFlow Tensor
            Return result from combining conv, batch norm and leaky ReLU or conv and bias as needed
        """
        weight = _tf.Variable(_tf.random.truncated_normal(shape, stddev=0.1), trainable=True, name=name + 'weight')

        conv = _tf.nn.conv2d(inputs, weight, strides=[1, 1, 1, 1], padding='SAME', name=name)

        if batch_norm:

            conv = self.batch_norm_wrapper(conv,
                                           batch_name,
                                           is_training = self.is_train)
            alpha = 0.1
            conv = _tf.maximum(alpha * conv, conv)
        else:
            bias = _tf.Variable(_tf.constant(0.1, shape=[shape[3]]), name=name + 'bias')
            conv = _tf.add(conv, bias)

        return conv

    def pooling_layer(self, inputs, pool_size, strides, name='1_pool'):
        """
        Define pooling layer

        Parameters
        ----------
        inputs: TensorFlow Tensor
            4d tensor of NHWC format
        pool_size: List of ints
            Size of window for each dimension of input tensor
        strides: List of ints
            Stride of sliding window for each dimension of input tensor
        name: string
            Name of the pooling layer

        Returns
        -------
        pool: TensorFlow Tensor
            Return pooling layer
        """

        pool = _tf.nn.max_pool2d(inputs, ksize=pool_size, strides=strides, padding='SAME', name=name)
        return pool

    def tiny_yolo(self, inputs, output_size=125):
        """
        Building the Tiny yolov2 network

        Parameters
        ----------
        inputs: TensorFlow Tensor
            Images sent as input for the network.
            This is an input tensor of shape [batch, in_height, in_width, in_channels]
        output_size: int
            Result of (num_classes + 5) * num_boxes

        Returns
        -------
        net: TensorFlow Tensor
            output of the TinyYOLOv2 network stored in net
        """

        filter_sizes = [16, 32, 64, 128, 256, 512, 1024, 1024]

        for idx, f in enumerate(filter_sizes, 1):
            batch_name = 'batchnorm%d_' % (idx - 1)
            if idx == 1:
                net = self.conv_layer(inputs, [3, 3, 3, f], name='conv%d_' % (idx - 1),
                                      batch_name=batch_name, batch_norm=True)
            else:
                net = self.conv_layer(net, [3, 3, filter_sizes[idx - 2], filter_sizes[idx - 1]],
                                      name='conv%d_' % (idx - 1), batch_name=batch_name, batch_norm=True)

            if idx < 7:
                if idx < 6:
                    strides = [1, 2, 2, 1]
                else:
                    strides = [1, 1, 1, 1]

                net = self.pooling_layer(net, pool_size=[1, 2, 2, 1], strides=strides, name='pool%d_' % idx)

        if output_size is not None:
            net = self.conv_layer(net, [1, 1, filter_sizes[idx - 1], output_size],
                                  name='conv8_', batch_name=None, batch_norm=False)

        return net

    def loss_layer(self, predict, labels):
        """
        Define loss layer

        Parameters
        ----------
        predict: TensorFlow Tensor
            The predicted values for the batch of data
        labels: TensorFlow Tensor
            Ground truth labels for the batch of data

        Returns
        -------
        loss: TensorFlow Tensor
            Loss (combination of regression and classification losses)
        """
        rescore = int(_utils.convert_shared_float_array_to_numpy(self.config.get('od_rescore')))
        lmb_coord_xy = _utils.convert_shared_float_array_to_numpy(self.config.get('lmb_coord_xy'))
        lmb_coord_wh = _utils.convert_shared_float_array_to_numpy(self.config.get('lmb_coord_wh'))
        lmb_obj = _utils.convert_shared_float_array_to_numpy(self.config.get('lmb_obj'))
        lmb_noobj = _utils.convert_shared_float_array_to_numpy(self.config.get('lmb_noobj'))
        lmb_class = _utils.convert_shared_float_array_to_numpy(self.config.get('lmb_class'))

        # Prediction values from model on the images
        ypred = _tf.reshape(predict, [-1] + list(self.grid_shape) + [self.num_anchors, 5 + self.num_classes])
        raw_xy = ypred[..., 0:2]
        raw_wh = ypred[..., 2:4]
        raw_conf = ypred[..., 4]
        class_scores = ypred[..., 5:]

        tf_anchors = _tf.constant(self.anchors)

        # Ground Truth info derived from ymap/labels
        gt_xy = labels[..., 0:2]
        gt_wh = labels[..., 2:4]
        gt_raw_wh = _tf.math.log(gt_wh / tf_anchors + 1e-5)
        gt_conf = labels[..., 4]
        gt_conf0 = labels[..., 0:1, 4]
        gt_class = labels[..., 5:]

        # Calculations on predicted confidences
        xy = _tf.sigmoid(raw_xy)
        wh = _tf.exp(raw_wh) * tf_anchors
        wh_anchors = _tf.exp(raw_wh * 0.0) * tf_anchors
        lo = xy - wh / 2
        hi = xy + wh / 2

        gt_area = gt_wh[..., 0] * gt_wh[..., 1]
        gt_lo = gt_xy - gt_wh / 2
        gt_hi = gt_xy + gt_wh / 2

        c_inter = _tf.maximum(2 * _tf.minimum(wh_anchors / 2, gt_wh / 2), 0)
        c_area = wh_anchors[..., 0] * wh_anchors[..., 1]
        c_inter_area = c_inter[..., 0] * c_inter[..., 1]
        c_iou = c_inter_area / (c_area + gt_area - c_inter_area)

        inter = _tf.maximum(_tf.minimum(hi, gt_hi) - _tf.maximum(lo, gt_lo), 0)
        area = wh[..., 0] * wh[..., 1]
        inter_area = inter[..., 0] * inter[..., 1]
        iou = inter_area / (area + gt_area - inter_area)
        active_iou = c_iou

        max_iou = _tf.reduce_max(active_iou, 3, keepdims=True)
        resp_box = _tf.cast(_tf.equal(active_iou, max_iou), dtype=_tf.float32)
        count = _tf.reduce_sum(gt_conf0)

        kr_obj_ij = _tf.stop_gradient(resp_box * gt_conf)

        kr_noobj_ij = 1 - kr_obj_ij
        s = 1 / (self.batch_size * self.grid_shape[0] * self.grid_shape[1])
        kr_obj_ij_plus1 = _tf.expand_dims(kr_obj_ij, -1)

        if rescore:
            obj_gt_conf = kr_obj_ij * _tf.stop_gradient(iou)
        else:
            obj_gt_conf = kr_obj_ij
        kr_box = kr_obj_ij_plus1
        obj_w = (kr_obj_ij * lmb_obj + kr_noobj_ij * lmb_noobj)

        loss_xy = lmb_coord_xy * _tf.reduce_sum(kr_box * _tf.square(gt_xy - xy)) / (count + 0.01)

        loss_wh = _tf.compat.v1.losses.huber_loss (labels=gt_raw_wh, predictions=raw_wh, weights=lmb_coord_wh * kr_box,
                                                   delta= 1.0)
        # Confidence loss
        loss_conf = s * _tf.reduce_sum(
            obj_w * _tf.nn.sigmoid_cross_entropy_with_logits(labels=obj_gt_conf, logits=raw_conf))

        # TODO: tf.nn.softmax_cross_entropy_with_logits_v2 instead of tf.nn.softmax_cross_entropy_with_logits
        loss_cls = lmb_class * _tf.reduce_sum(
            kr_obj_ij * _tf.nn.softmax_cross_entropy_with_logits_v2(labels=gt_class, logits=class_scores)) / (
                           count + 0.01)
        losses = [loss_xy, loss_wh, loss_conf, loss_cls]
        loss = _tf.add_n(losses)
        return loss

    def train(self, feed_dict):
        """
        Run session for training with new batch of data(Input and Label)

        Parameters
        ----------
        feed_dict: Dictionary
            Dictionary to store a batch of input data, corresponding labels and iteration number. This is currently
            passed from the object_detector.py file when a new batch of data is sent.

        Returns
        -------
        loss_batch: TensorFlow Tensor
            Loss per batch
        """
        for key in feed_dict.keys():
            feed_dict[key] = _utils.convert_shared_float_array_to_numpy(feed_dict[key])
        feed_dict['labels'] = feed_dict['labels'].reshape(self.batch_size, self.grid_shape[0], self.grid_shape[1],self.num_anchors, self.num_classes + 5)

        _, loss_batch = self.sess.run([self.train_op, self.loss], feed_dict={self.images: feed_dict['input'],
                                                                             self.labels: feed_dict['labels']})
        result = {}
        result['loss'] = _np.array([loss_batch])
        return result

    def predict(self, feed_dict):
        """
        Run session for predicting with new batch of data(Input)

        Parameters
        ----------
        feed_dict: Dictionary
            Dictionary to store a batch of input data

        Returns
        -------
        output: TensorFlow Tensor
            Feature map from building the network. This will be used in MXNet for further processing
        """
        for key in feed_dict.keys():
            feed_dict[key] = _utils.convert_shared_float_array_to_numpy(feed_dict[key])

        output = self.sess.run([self.tf_model], feed_dict={self.images: feed_dict['input']})

        # TODO: Include self.labels: feed_dict['label'] to handle labels from validation set
        result = {}
        result['output'] = _np.array(output[0])
        return result

    def export_weights(self):
        """
        Function to store TensorFlow weights back to into a dict for use with MXNet

        Returns
        -------
        tf_export_params: Dictionary
            Dictionary of weights from TensorFlow stored as {weight_name: weight_value}
        """
        tf_export_params = {}

        # collect all TF variables to include running_mean and running_variance
        tvars = _tf.compat.v1.global_variables()
        tvars_vals = self.sess.run(tvars)
        for var, val in zip(tvars, tvars_vals):
            if val.ndim == 1:
                tf_export_params.update(
                    {var.name.replace(":0", ""): val})
            elif val.ndim == 4:
                # Converting from [filter_height, filter_width, input_channels, output_channels] to
                # [output_channels, input_channels, filter_height, filter_width]
                tf_export_params.update(
                    {var.name.replace(":0", ""): val.transpose(3,2,0,1)})
        for layer_name in tf_export_params.keys():
            tf_export_params[layer_name] = _np.ascontiguousarray(tf_export_params[layer_name])

        return tf_export_params

    def set_learning_rate(self, lr):
        """
        Function to update learning rate

        Parameters
        ----------
        lr: float32
            Old learning rate

        Returns
        -------
        lr: float32
            New learning rate
        """
        return lr
