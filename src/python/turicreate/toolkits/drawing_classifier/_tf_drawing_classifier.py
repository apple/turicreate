# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import numpy as _np
from .._tf_model import TensorFlowModel
import turicreate.toolkits._tf_utils as _utils
import tensorflow.compat.v1 as _tf

_tf.disable_v2_behavior()


class DrawingClassifierTensorFlowModel(TensorFlowModel):
    def __init__(self, net_params, batch_size, num_classes):
        """
        Defines the TensorFlow model, loss, optimisation and accuracy. Then
        loads the weights into the model.

        """
        self.gpu_policy = _utils.TensorFlowGPUPolicy()
        self.gpu_policy.start()

        for key in net_params.keys():
            net_params[key] = _utils.convert_shared_float_array_to_numpy(
                net_params[key]
            )

        self.dc_graph = _tf.Graph()
        self.num_classes = num_classes
        self.batch_size = batch_size
        self.sess = _tf.Session(graph=self.dc_graph)
        with self.dc_graph.as_default():
            self.init_drawing_classifier_graph(net_params)

    def init_drawing_classifier_graph(self, net_params):

        self.input = _tf.placeholder(_tf.float32, [self.batch_size, 28, 28, 1])
        self.weights = _tf.placeholder(_tf.float32, [self.batch_size, 1])
        self.labels = _tf.placeholder(_tf.int64, [self.batch_size, 1])

        # One hot encoding target
        reshaped_labels = _tf.reshape(self.labels, [self.batch_size])
        one_hot_labels = _tf.one_hot(reshaped_labels, depth=self.num_classes, axis=-1)

        # Reshaping weights
        reshaped_weights = _tf.reshape(self.weights, [self.batch_size])

        self.one_hot_labels = _tf.placeholder(_tf.int32, [None, self.num_classes])

        # Weights
        weights = {
            "drawing_conv0_weight": _tf.Variable(
                _tf.zeros([3, 3, 1, 16]), name="drawing_conv0_weight"
            ),
            "drawing_conv1_weight": _tf.Variable(
                _tf.zeros([3, 3, 16, 32]), name="drawing_conv1_weight"
            ),
            "drawing_conv2_weight": _tf.Variable(
                _tf.zeros([3, 3, 32, 64]), name="drawing_conv2_weight"
            ),
            "drawing_dense0_weight": _tf.Variable(
                _tf.zeros([576, 128]), name="drawing_dense0_weight"
            ),
            "drawing_dense1_weight": _tf.Variable(
                _tf.zeros([128, self.num_classes]), name="drawing_dense1_weight"
            ),
        }

        # Biases
        biases = {
            "drawing_conv0_bias": _tf.Variable(
                _tf.zeros([16]), name="drawing_conv0_bias"
            ),
            "drawing_conv1_bias": _tf.Variable(
                _tf.zeros([32]), name="drawing_conv1_bias"
            ),
            "drawing_conv2_bias": _tf.Variable(
                _tf.zeros([64]), name="drawing_conv2_bias"
            ),
            "drawing_dense0_bias": _tf.Variable(
                _tf.zeros([128]), name="drawing_dense0_bias"
            ),
            "drawing_dense1_bias": _tf.Variable(
                _tf.zeros([self.num_classes]), name="drawing_dense1_bias"
            ),
        }

        conv_1 = _tf.nn.conv2d(
            self.input, weights["drawing_conv0_weight"], strides=1, padding="SAME"
        )
        conv_1 = _tf.nn.bias_add(conv_1, biases["drawing_conv0_bias"])
        relu_1 = _tf.nn.relu(conv_1)
        pool_1 = _tf.nn.max_pool2d(
            relu_1, ksize=[1, 2, 2, 1], strides=[1, 2, 2, 1], padding="VALID"
        )

        conv_2 = _tf.nn.conv2d(
            pool_1, weights["drawing_conv1_weight"], strides=1, padding="SAME"
        )
        conv_2 = _tf.nn.bias_add(conv_2, biases["drawing_conv1_bias"])
        relu_2 = _tf.nn.relu(conv_2)
        pool_2 = _tf.nn.max_pool2d(
            relu_2, ksize=[1, 2, 2, 1], strides=[1, 2, 2, 1], padding="VALID"
        )

        conv_3 = _tf.nn.conv2d(
            pool_2, weights["drawing_conv2_weight"], strides=1, padding="SAME"
        )
        conv_3 = _tf.nn.bias_add(conv_3, biases["drawing_conv2_bias"])
        relu_3 = _tf.nn.relu(conv_3)
        pool_3 = _tf.nn.max_pool2d(
            relu_3, ksize=[1, 2, 2, 1], strides=[1, 2, 2, 1], padding="VALID"
        )

        # Flatten the data to a 1-D vector for the fully connected layer
        fc1 = _tf.reshape(pool_3, (-1, 576))

        fc1 = _tf.nn.xw_plus_b(
            fc1,
            weights=weights["drawing_dense0_weight"],
            biases=biases["drawing_dense0_bias"],
        )

        fc1 = _tf.nn.relu(fc1)

        out = _tf.nn.xw_plus_b(
            fc1,
            weights=weights["drawing_dense1_weight"],
            biases=biases["drawing_dense1_bias"],
        )
        softmax_out = _tf.nn.softmax(out)

        self.predictions = softmax_out

        # Loss
        self.cost = _tf.losses.softmax_cross_entropy(
            logits=out,
            onehot_labels=one_hot_labels,
            weights=reshaped_weights,
            reduction=_tf.losses.Reduction.NONE,
        )

        # Optimizer
        self.optimizer = _tf.train.AdamOptimizer(learning_rate=0.001).minimize(
            self.cost
        )

        self.sess = _tf.Session()
        self.sess.run(_tf.global_variables_initializer())

        # Assign the initialised weights from C++ to tensorflow
        layers = [
            "drawing_conv0_weight",
            "drawing_conv0_bias",
            "drawing_conv1_weight",
            "drawing_conv1_bias",
            "drawing_conv2_weight",
            "drawing_conv2_bias",
            "drawing_dense0_weight",
            "drawing_dense0_bias",
            "drawing_dense1_weight",
            "drawing_dense1_bias",
        ]

        for key in layers:
            if "bias" in key:
                self.sess.run(
                    _tf.assign(
                        _tf.get_default_graph().get_tensor_by_name(key + ":0"),
                        net_params[key],
                    )
                )
            else:
                if "drawing_dense0_weight" in key:
                    """
                    To make output of CoreML pool3 (NCHW) compatible with TF (NHWC).
                    Decompose FC weights to NCHW. Transpose to NHWC. Reshape back to FC.
                    """
                    coreml_128_576 = net_params[key]
                    coreml_128_576 = _np.reshape(coreml_128_576, (128, 64, 3, 3))
                    coreml_128_576 = _np.transpose(coreml_128_576, (0, 2, 3, 1))
                    coreml_128_576 = _np.reshape(coreml_128_576, (128, 576))
                    self.sess.run(
                        _tf.assign(
                            _tf.get_default_graph().get_tensor_by_name(key + ":0"),
                            _np.transpose(coreml_128_576, (1, 0)),
                        )
                    )
                elif "dense" in key:
                    dense_weights = _utils.convert_dense_coreml_to_tf(net_params[key])
                    self.sess.run(
                        _tf.assign(
                            _tf.get_default_graph().get_tensor_by_name(key + ":0"),
                            dense_weights,
                        )
                    )
                else:
                    self.sess.run(
                        _tf.assign(
                            _tf.get_default_graph().get_tensor_by_name(key + ":0"),
                            _utils.convert_conv2d_coreml_to_tf(net_params[key]),
                        )
                    )

    def __del__(self):
        self.sess.close()
        self.gpu_policy.stop()

    def train(self, feed_dict):

        for key in feed_dict.keys():
            feed_dict[key] = _utils.convert_shared_float_array_to_numpy(feed_dict[key])

        _, final_train_loss, final_train_output = self.sess.run(
            [self.optimizer, self.cost, self.predictions],
            feed_dict={
                self.input: feed_dict["input"],
                self.labels: feed_dict["labels"],
                self.weights: feed_dict["weights"],
            },
        )

        result = {
            "loss": _np.array(final_train_loss),
            "output": _np.array(final_train_output),
        }

        return result

    def predict(self, feed_dict):

        is_train = "labels" in feed_dict

        for key in feed_dict.keys():
            feed_dict[key] = _utils.convert_shared_float_array_to_numpy(feed_dict[key])

        one_hot_labels = _np.zeros((int(self.batch_size), self.num_classes))

        feed_dict_for_session = {self.input: feed_dict["input"]}

        if is_train:

            feed_dict_for_session[self.labels] = feed_dict["labels"]
            feed_dict_for_session[self.weights] = feed_dict["weights"]

            pred_probs, loss = self.sess.run(
                [self.predictions, self.cost], feed_dict=feed_dict_for_session
            )

            result = {"loss": _np.array(loss), "output": _np.array(pred_probs)}
        else:
            pred_probs = self.sess.run(
                [self.predictions], feed_dict=feed_dict_for_session
            )
            result = {"output": _np.array(pred_probs)}

        return result

    def export_weights(self):
        """
        Retrieve weights from the TF model, convert to the format Core ML
        expects and store in a dictionary.

        Returns
        -------
        net_params : dict
            Dictionary of weights, where the key is the name of the
            layer (e.g. `drawing_conv0_weight`) and the value is the
            respective weight of type `numpy.ndarray`.
        """

        net_params = {}
        with self.dc_graph.as_default():
            layer_names = _tf.trainable_variables()
            layer_weights = self.sess.run(layer_names)

        for var, val in zip(layer_names, layer_weights):
            if "bias" in var.name:
                net_params.update({var.name.replace(":0", ""): val})
            else:
                if "dense" in var.name:
                    if "drawing_dense0_weight" in var.name:
                        """
                         To make output of TF pool3 (NHWC) compatible with CoreML (NCHW).
                         Decompose FC weights to NHWC. Transpose to NCHW. Reshape back to FC.
                         """
                        tf_576_128 = val
                        tf_576_128 = _np.reshape(tf_576_128, (3, 3, 64, 128))
                        tf_576_128 = _np.transpose(tf_576_128, (2, 0, 1, 3))
                        tf_576_128 = _np.reshape(tf_576_128, (576, 128))
                        net_params.update(
                            {
                                var.name.replace(":0", ""): _np.transpose(
                                    tf_576_128, (1, 0)
                                )
                            }
                        )
                    else:
                        net_params.update(
                            {var.name.replace(":0", ""): val.transpose(1, 0)}
                        )
                else:
                    # np.transpose won't change the underlying memory layout
                    # but in turicreate we will force it.
                    net_params.update(
                        {
                            var.name.replace(
                                ":0", ""
                            ): _utils.convert_conv2d_tf_to_coreml(val)
                        }
                    )

        return net_params
