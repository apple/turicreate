import unittest
import tensorflow.compat.v1 as tf
import numpy as np
from coremltools._deps import HAS_TF_1_14

from test_base import TFNetworkTest
import itertools

# IMPORTANT NOTE TO ADD NEW TESTS:
# For each test function you should set up your own graph and session.
# Otherwise TF will carry all ops and tensors from previously run tests.

def conv_cell(inp, conv_weights, bias=None, activation=None, pooling=None, has_batchnorm=False, conv_config=None, data_format='NHWC'):
    if conv_config is None:
        conv_config = {'strides': [1, 1, 1, 1], 'padding': 'SAME'}
    x = tf.nn.conv2d(inp, conv_weights, conv_config['strides'], conv_config['padding'], data_format=data_format)
    return x


class TFConvNetTest(TFNetworkTest):
    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading common utilities.
        """
        pass

    # Backend - set use_cpu_only to be True when working on Intel GPU macs
    def _test_tf_model(
            self,
            graph,
            input_tensor_shapes,
            output_node_names,
            data_mode='random',
            delta=1e-2,
            use_cpu_only=True,
            graph_optimizations="freeze",  # one of ["freeze", "convert_variables_to_constants", None]
            quantize_tf_model=False):
        super(TFConvNetTest, self)._test_tf_model(
            graph,
            input_tensor_shapes,
            output_node_names,
            data_mode=data_mode,
            input_refs=None,
            delta=delta,
            use_cpu_only=use_cpu_only,
            graph_optimizations=graph_optimizations,
            quantize_tf_model=quantize_tf_model)

    def test_toy(self):
        graph = tf.Graph()
        with graph.as_default():
            matrix1 = tf.placeholder(tf.float32, shape=[1, 2])
            matrix2 = tf.Variable(tf.truncated_normal([2, 1]))
            product = tf.matmul(matrix1, matrix2)
        self._test_tf_model(graph, {matrix1.op.name: [1, 2]}, [product.op.name])

    def test_linear(self):
        graph = tf.Graph()
        with graph.as_default():
            # placeholder constructor returns a tensor not an op
            x = tf.placeholder(tf.float32, shape=[None, 20])
            # Make a redundant tensor. It should get trimmed
            gt = tf.placeholder(tf.float32, shape=[None, 10])

            W = tf.Variable(tf.ones([20, 10]))
            b = tf.Variable(tf.ones([10]))

            y = tf.matmul(x, W) + b
            output_name = [y.op.name]
        self._test_tf_model(graph, {x.op.name: [1, 20]}, output_name)
        self._test_tf_model(graph, {x.op.name: [8, 20]}, output_name)

    def test_convnet(self):
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            W1 = tf.Variable(tf.truncated_normal([3, 3, 3, 4], stddev=0.3))
            x = conv_cell(a, W1)
            W2 = tf.Variable(tf.truncated_normal([3, 3, 4, 2], stddev=0.3))
            x = conv_cell(x, W2)
        self._test_tf_model(graph, {a.op.name: [1, 8, 8, 3]}, [x.op.name])
        self._test_tf_model(graph, {a.op.name: [10, 8, 8, 3]}, [x.op.name])

    def test_convnet_batchnorm(self):
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            W1 = tf.Variable(tf.truncated_normal([3, 3, 3, 4], stddev=0.3))
            x = conv_cell(a, W1, has_batchnorm=True)
            W2 = tf.Variable(tf.truncated_normal([3, 3, 4, 2], stddev=0.3))
            x = conv_cell(x, W2, has_batchnorm=True)
        self._test_tf_model(graph, {a.op.name: [1, 8, 8, 3]}, [x.op.name])
        self._test_tf_model(graph, {a.op.name: [10, 8, 8, 3]}, [x.op.name])

    def test_simple_convnet(self):
        def weight_variable(shape):
            initial = tf.truncated_normal(shape, stddev=0.1)
            return tf.Variable(initial)

        def bias_variable(shape):
            initial = tf.constant(0.1, shape=shape)
            return tf.Variable(initial)

        def conv2d(x, W):
            return tf.nn.conv2d(x, W, strides=[1, 1, 1, 1], padding='SAME')

        def max_pool_2x2(x):
            return tf.nn.max_pool(x, ksize=[1, 2, 2, 1], strides=[1, 2, 2, 1], padding='SAME')

        def avg_pool_2x2(x):
            return tf.nn.avg_pool(x, ksize=[1, 2, 2, 1], strides=[1, 2, 2, 1], padding='SAME')

        graph = tf.Graph()
        with graph.as_default():
            W_conv1 = weight_variable([5, 5, 1, 32])
            b_conv1 = bias_variable([32])

            x_image = tf.placeholder(tf.float32, shape=[None, 28, 28, 1])
            h_conv1 = tf.nn.relu(conv2d(x_image, W_conv1) + b_conv1)
            h_pool1 = max_pool_2x2(h_conv1)

            W_conv2 = weight_variable([5, 5, 32, 64])
            b_conv2 = bias_variable([64])

            h_conv2 = tf.nn.relu(conv2d(h_pool1, W_conv2) + b_conv2)
            h_pool2 = avg_pool_2x2(h_conv2)

        output_name = [h_pool2.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 28, 28, 1]}, output_name)

    def test_convnet_classifier(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            W_conv1 = tf.Variable(tf.truncated_normal([3, 3, 3, 2], stddev=0.3))
            h_conv1 = tf.nn.conv2d(x_image, W_conv1, strides=[1, 1, 1, 1], padding='SAME')
            h_conv1_flat = tf.reshape(h_conv1, [-1, 8 * 8 * 2])
            W_fc1 = tf.Variable(tf.truncated_normal([8 * 8 * 2, 4], stddev=0.3))
            h_fc1 = tf.matmul(h_conv1_flat, W_fc1)
        output_name = [h_fc1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 8, 8, 3]}, output_name)
        self._test_tf_model(graph, {x_image.op.name: [10, 8, 8, 3]}, output_name)

    @unittest.skip('Type 12 cannot be mapped')
    def test_convnet_quantized(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            W_conv1 = tf.Variable(tf.truncated_normal([3, 3, 3, 2], stddev=0.3))
            h_conv1 = tf.nn.conv2d(x_image, W_conv1, strides=[1, 1, 1, 1], padding='SAME')
            h_conv1_flat = tf.reshape(h_conv1, [-1, 8 * 8 * 2])
            W_fc1 = tf.Variable(tf.truncated_normal([8 * 8 * 2, 4], stddev=0.3))
            h_fc1 = tf.matmul(h_conv1_flat, W_fc1)

        output_name = [h_fc1.op.name]
        # quantized
        self._test_tf_model(
            graph, {x_image.op.name: [1, 8, 8, 3]},
            output_name,
            delta=0.20,
            quantize_tf_model=True)

    def test_pad_conv_fuse(self):
        graph = tf.Graph()
        with graph.as_default():
            x = tf.placeholder(tf.float32, shape=[None, 32, 18, 3])
            W = tf.Variable(tf.truncated_normal([9, 9, 3, 5], stddev=1))
            paddings = tf.constant([[0, 0], [5, 5], [1, 1], [0, 0]])
            x_pad = tf.pad(x, paddings, "CONSTANT")
            output = tf.nn.conv2d(x_pad, W, strides=[1, 1, 1, 1], padding='VALID')
        output_name = [output.op.name]
        self._test_tf_model(graph, {x.op.name: [1, 32, 18, 3]}, output_name, delta=.05)

    def test_dilated_conv(self):
        Cin = 3
        Cout = 5
        # params in format (Hin, Win, K, dilation)
        params = [(30, 18, 3, 3), (16, 12, 3, 4), (15, 21, 1, 3), (27, 18, 5, 3), (14, 20, 3, 2)]
        for param in params:
            Hin, Win, K, d = param
            graph = tf.Graph()
            with graph.as_default():
                x = tf.placeholder(tf.float32, shape=[None, Hin, Win, Cin])
                W = tf.Variable(tf.truncated_normal([K, K, Cin, Cout], stddev=1))
                output = tf.nn.convolution(
                    x, W, strides=[1, 1], padding='VALID', dilation_rate=[d, d])
            output_name = [output.op.name]
            self._test_tf_model(graph, {x.op.name: [1, Hin, Win, Cin]}, output_name, delta=.01)

    def test_depthwise_conv2d_native(self):
        options = dict(
            depthwise_multiplier = [1,2],
            strides = [[1,1,1,1], [1,2,2,1]],
            padding = ['VALID', 'SAME'],
        )
        product = itertools.product(*options.values())
        for prod in product:
            params = dict(zip(options.keys(), prod))
            graph = tf.Graph()
            with graph.as_default():
                x_image = tf.placeholder(tf.float32, shape=[None, 16, 16, 3])
                kernels = tf.Variable(
                    tf.truncated_normal([3, 3, 3, params['depthwise_multiplier']],
                    stddev=0.3))
                conv1 = tf.nn.depthwise_conv2d_native(
                    input=x_image, filter=kernels, strides=params['strides'],
                    padding=params['padding'])
            output_name = [conv1.op.name]
            self._test_tf_model(graph, {x_image.op.name: [1, 16, 16, 3]}, output_name)

class TFSingleLayerTest(TFNetworkTest):
    """
    Small models from tensorflow.layers
    """

    def test_dense(self):
        # dense layer with some activation
        graph = tf.Graph()
        with graph.as_default():
            x = tf.placeholder(tf.float32, shape=[None, 10])
            y = tf.layers.dense(
                inputs=x,
                units=16,
                activation=tf.sigmoid,
                bias_initializer=tf.random_uniform_initializer)
        output_name = [y.op.name]
        self._test_tf_model(
            graph, {x.op.name: [1, 10]},
            output_name,
            delta=1e-2,
            quantize_tf_model=False,
            use_cpu_only=True)

    @unittest.skip('Type 12 cannot be mapped')
    def test_dense_quantized(self):
        # dense layer with some activation
        graph = tf.Graph()
        with graph.as_default():
            x = tf.placeholder(tf.float32, shape=[None, 10])
            y = tf.layers.dense(
                inputs=x,
                units=16,
                activation=tf.sigmoid,
                bias_initializer=tf.random_uniform_initializer)

        output_name = [y.op.name]
        self._test_tf_model(
            graph, {x.op.name: [1, 10]}, output_name, delta=0.05, quantize_tf_model=True)

    def test_dense_concat(self):
        graph = tf.Graph()
        with graph.as_default():
            x = tf.placeholder(tf.float32, shape=[None, 10])
            y = tf.layers.dense(
                inputs=x,
                units=16,
                activation=tf.nn.relu,
                bias_initializer=tf.random_uniform_initializer)
            z1 = tf.layers.dense(
                inputs=y,
                units=20,
                activation=tf.nn.relu,
                bias_initializer=tf.random_uniform_initializer)
            z2 = tf.layers.dense(
                inputs=y,
                units=20,
                activation=tf.nn.relu,
                bias_initializer=tf.random_uniform_initializer)
            z3 = tf.layers.dense(
                inputs=y,
                units=20,
                activation=tf.nn.relu,
                bias_initializer=tf.random_uniform_initializer)
            z = tf.concat([z1, z2, z3], axis=1)

        output_name = [z.op.name]
        self._test_tf_model(graph, {x.op.name: [1, 10]}, output_name, use_cpu_only=True)

    def test_conv2d_no_bias(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            W = tf.Variable(tf.random_normal((5, 5, 3, 4)))  # [Kh, Kw, Cin, Cout]
            conv1 = tf.nn.conv2d(input=x_image, filter=W, strides=[1, 1, 1, 1], padding='SAME')
        output_name = [conv1.op.name]
        self._test_tf_model(
            graph, {x_image.op.name: [1, 8, 8, 3]},
            output_name,
            delta=1e-2,
            quantize_tf_model=False,
            use_cpu_only=True)

    def test_conv2d(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            conv1 = tf.layers.conv2d(
                inputs=x_image,
                filters=4,
                kernel_size=[5, 5],
                padding='same',
                activation=tf.nn.relu,
                bias_initializer=tf.constant_initializer([1, 2, 3, 4]))
        output_name = [conv1.op.name]
        self._test_tf_model(
            graph, {x_image.op.name: [1, 8, 8, 3]},
            output_name,
            delta=1e-2,
            quantize_tf_model=False,
            use_cpu_only=True)

    @unittest.skip('Type 12 cannot be mapped')
    def test_conv2d_quantized(self):
        # conv layer with "fused activation"
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            conv1 = tf.layers.conv2d(
                inputs=x_image,
                filters=4,
                kernel_size=[5, 5],
                padding='same',
                activation=tf.nn.relu,
                bias_initializer=tf.random_uniform_initializer)

        output_name = [conv1.op.name]
        self._test_tf_model(
            graph, {x_image.op.name: [1, 8, 8, 3]},
            output_name,
            delta=0.05,
            quantize_tf_model=True)

    def test_conv2d_valid(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            conv1 = tf.layers.conv2d(
                inputs=x_image,
                filters=4,
                kernel_size=[3, 3],
                padding='valid',
                activation=tf.nn.relu,
                bias_initializer=tf.random_uniform_initializer)
        output_name = [conv1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 8, 8, 3]}, output_name)

    def test_conv2d_stride2(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            conv1 = tf.layers.conv2d(
                inputs=x_image,
                filters=4,
                kernel_size=[3, 3],
                padding='valid',
                strides=(2, 2),
                bias_initializer=tf.random_uniform_initializer)
        output_name = [conv1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 8, 8, 3]}, output_name)

    @unittest.skip('SpaceToBatchND, BatchToSpaceND does not yet support some of the inputs')
    def test_conv2d_dilated(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 32, 32, 3])
            conv1 = tf.layers.conv2d(
                inputs=x_image,
                filters=4,
                kernel_size=[3, 3],
                padding='valid',
                dilation_rate=(3, 4),  # does not yet support non-equal dilation rate
                bias_initializer=tf.random_uniform_initializer)
        output_name = [conv1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 32, 32, 3]}, output_name)

    def test_conv2d_transpose(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[1, 2, 2, 8])
            conv1 = tf.layers.conv2d_transpose(
                inputs=x_image,
                filters=4,
                kernel_size=[5, 5],
                padding='same',
                activation=tf.nn.relu,
                bias_initializer=tf.random_uniform_initializer)
        output_name = [conv1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 2, 2, 8]}, output_name)

    def test_conv2d_transpose_valid(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            conv1 = tf.layers.conv2d_transpose(
                inputs=x_image,
                filters=4,
                kernel_size=[3, 3],
                padding='valid',
                activation=tf.nn.relu,
                bias_initializer=tf.random_uniform_initializer)
        output_name = [conv1.op.name]
        self._test_tf_model(
            graph, {x_image.op.name: [1, 8, 8, 3]}, output_name)

    def test_conv2d_transpose_stride2(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            conv1 = tf.layers.conv2d_transpose(
                inputs=x_image,
                filters=4,
                kernel_size=[3, 3],
                padding='valid',
                strides=(2, 2),
                bias_initializer=tf.random_uniform_initializer)
        output_name = [conv1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 8, 8, 3]}, output_name)

    def test_conv2d_ave_pooling(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 16, 16, 3])
            conv1 = tf.layers.conv2d(
                inputs=x_image,
                filters=4,
                kernel_size=[3, 3],
                padding='same',
                activation=tf.nn.relu,
                bias_initializer=tf.random_uniform_initializer)
            pool1 = tf.layers.average_pooling2d(inputs=conv1, pool_size=[2, 2], strides=2)
        output_name = [pool1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 16, 16, 3]}, output_name)

    def test_conv2d_max_pooling(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 16, 16, 3])
            conv1 = tf.layers.conv2d(
                inputs=x_image,
                filters=4,
                kernel_size=[3, 3],
                padding='same',
                activation=tf.nn.relu,
                bias_initializer=tf.random_uniform_initializer)
            pool1 = tf.layers.max_pooling2d(inputs=conv1, pool_size=[3, 3], strides=1, padding='same')
        output_name = [pool1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 16, 16, 3]}, output_name)

    def test_conv2d_bn(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[1, 16, 16, 3])
            conv1 = tf.layers.conv2d(
                inputs=x_image,
                filters=4,
                kernel_size=[3, 3],
                padding='same',
                activation=tf.nn.relu,
                bias_initializer=tf.random_uniform_initializer)
            bn1 = tf.layers.batch_normalization(inputs=conv1, axis=-1)
        output_name = [bn1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 16, 16, 3]}, output_name)

    def test_conv2d_spatial_bn(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 16, 16, 3])
            bn1 = tf.layers.batch_normalization(inputs=x_image, axis=2)
        output_name = [bn1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 16, 16, 3]}, output_name)

    @unittest.skip('DepthwiseConv2dNative not implemented')
    def test_separable_conv2d(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 8, 3])
            conv1 = tf.layers.separable_conv2d(
                inputs=x_image, filters=4, kernel_size=[3, 3], padding='valid', depth_multiplier=2)
        output_name = [conv1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 8, 8, 3]}, output_name)

    def test_conv1d(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 3])
            conv1 = tf.layers.conv1d(
                inputs=x_image, filters=2, kernel_size=3, padding='valid', use_bias=True)
        output_name = [conv1.op.name]
        self._test_tf_model(
            graph, {x_image.op.name: [1, 8, 3]}, output_name, data_mode='linear', delta=.05)

    def test_conv1d_dense(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 3])
            conv1 = tf.layers.conv1d(
                inputs=x_image,
                filters=2,
                kernel_size=3,
                padding='same',
                bias_initializer=tf.random_uniform_initializer)
            conv1_flat = tf.reshape(conv1, [-1, 8 * 2])
            y = tf.layers.dense(inputs=conv1_flat, units=6, activation=tf.nn.relu)
        output_name = [y.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 8, 3]}, output_name)
        self._test_tf_model(graph, {x_image.op.name: [10, 8, 3]}, output_name)

    def test_conv1d_ave_pooling(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 3])
            conv1 = tf.layers.conv1d(inputs=x_image, filters=2, kernel_size=5, padding='same')
            pool1 = tf.layers.average_pooling1d(inputs=conv1, pool_size=2, strides=2)
        output_name = [pool1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 8, 3]}, output_name)

    def test_conv1d_max_pooling(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 8, 3])
            conv1 = tf.layers.conv1d(inputs=x_image, filters=2, kernel_size=3, padding='same')
            pool1 = tf.layers.max_pooling1d(inputs=conv1, pool_size=2, strides=1)
        output_name = [pool1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 8, 3]}, output_name)

    def test_conv2d_resize_bilinear(self):
        graph = tf.Graph()
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 16, 16, 3])
            conv1 = tf.layers.conv2d(
                inputs=x_image,
                filters=3,
                kernel_size=[3, 3],
                padding='same',
                activation=tf.nn.relu)
            bl1 = tf.image.resize_bilinear(images=conv1, size=[32, 32])
        output_name = [bl1.op.name]
        self._test_tf_model(graph, {x_image.op.name: [1, 16, 16, 3]}, output_name)

    def test_concat_constants(self):
        graph = tf.Graph()
        x, y = np.meshgrid(np.linspace(0., 1., 256), np.linspace(0., 1., 256))
        x = np.reshape(x, [1, 256, 256, 1])
        y = np.reshape(y, [1, 256, 256, 1])
        with graph.as_default():
            x_image = tf.placeholder(tf.float32, shape=[None, 256, 256, 3])
            xx = tf.constant(x, dtype=tf.float32)
            yy = tf.constant(y, dtype=tf.float32)
            img_concatenated = tf.concat([x_image, xx, yy], -1)
        output_name = [img_concatenated.op.name]
        self._test_tf_model_constant(graph, {x_image.op.name: [1, 256, 256, 3]}, output_name)

    def test_split(self):
        graph = tf.Graph()
        with graph.as_default():
            x_input = tf.placeholder(tf.float32, shape=[None, 10, 10, 6])
            y1, y2 = tf.split(x_input, 2, axis=3)
            z = tf.add(y1, y2)
        self._test_tf_model_constant(graph, {x_input.op.name: [1, 10, 10, 6]}, [z.op.name])

    def test_add(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.add(a, b)
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_sub(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.math.subtract(a, b)
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_mul(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.math.multiply(a, b)
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_floor_mod(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.floormod(a, b)
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_floor_div(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.floor_div(a, b)
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_real_div(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.divide(a, b)
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_bias_add(self):
        # shape = [3, 4, 5]
        shape = [1, 2, 2, 4]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=[4])
            out = tf.nn.bias_add(a, b)
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: [4]}, [out.op.name])

    def test_maximum(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.maximum(a, b)
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_minimum(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.minimum(a, b)
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_reduce_prod(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.reduce_prod(a, axis=0)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name],
                                     data_mode='random_zero_mean_with_zeros')

    def test_reduce_mean(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.reduce_mean(a, axis=-1)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_reduce_sum(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.reduce_sum(a, axis=-1)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_reduce_max(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.reduce_max(a, axis=-1)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_reduce_min(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.reduce_min(a, axis=-1)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_logical_and(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.logical_and(tf.less(a, b), tf.less(a, b))
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_logical_or(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.logical_or(tf.less(a, b), tf.less(a, b))
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_logical_not(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.logical_not(tf.less(a, b))
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_cast(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.cast(a, tf.int32)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_sin(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.sin(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_cos(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.cos(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_tan(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.tan(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_sqrt(self):
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=[None, 10, 10, 6])
            out = tf.sqrt(a)
        self._test_tf_model_constant(graph, {a.op.name: [1, 10, 10, 6]}, [out.op.name], data_mode='random_large')

    def test_rsqrt(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.rsqrt(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name], data_mode='random_large')

    def test_pow(self):
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=[None, 5, 5, 6])
            out = tf.pow(a, 4)
        self._test_tf_model_constant(graph, {a.op.name: [1, 5, 5, 6]}, [out.op.name])

    def test_log(self):
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=[None, 20])
            out = tf.log(a)
        self._test_tf_model_constant(graph, {a.op.name: [1, 20]}, [out.op.name], data_mode='random')

    def test_exp(self):
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=[None, 20])
            out = tf.exp(a)
        self._test_tf_model_constant(graph, {a.op.name: [1, 20]}, [out.op.name])

    def test_abs(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.abs(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_square(self):
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=[None, 20])
            out = tf.square(a)
        self._test_tf_model_constant(graph, {a.op.name: [1, 20]}, [out.op.name])

    def test_squared_difference(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.squared_difference(a, b)
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_sign(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.sign(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name], data_mode='random_int')

    def test_ceil(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.ceil(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name], data_mode='random_int')

    def test_floor(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.floor(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name], data_mode='random_int')

    def test_round(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.round(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_negative(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.negative(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_equal(self):
        shape_a = [1, 4, 5]
        shape_b = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape_a)
            b = tf.placeholder(tf.float32, shape=shape_b)
            out = tf.equal(a, b)
        self._test_tf_model_constant(
            graph, {a.op.name: shape_a, b.op.name: shape_b}, [out.op.name],
            data_mode='random_zero_mean_with_zeros')

    def test_not_equal(self):
        shape_a = [1, 4, 5]
        shape_b = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape_a)
            b = tf.placeholder(tf.float32, shape=shape_b)
            out = tf.not_equal(a, b)
        self._test_tf_model_constant(
            graph, {a.op.name: shape_a, b.op.name: shape_b}, [out.op.name],
            data_mode='random_zero_mean_with_zeros')

    def test_less(self):
        shape_a = [1, 4, 5]
        shape_b = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape_a)
            b = tf.placeholder(tf.float32, shape=shape_b)
            out = tf.less(a, b)
        self._test_tf_model_constant(
            graph, {a.op.name: shape_a, b.op.name: shape_b}, [out.op.name],
            data_mode='random_zero_mean_with_zeros')

    def test_less_equal(self):
        shape_a = [1, 4, 5]
        shape_b = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape_a)
            b = tf.placeholder(tf.float32, shape=shape_b)
            out = tf.less_equal(a, b)
        self._test_tf_model_constant(
            graph, {a.op.name: shape_a, b.op.name: shape_b}, [out.op.name],
            data_mode='random_zero_mean_with_zeros')

    def test_greater(self):
        shape_a = [1, 4, 5]
        shape_b = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape_a)
            b = tf.placeholder(tf.float32, shape=shape_b)
            out = tf.greater(a, b)
        self._test_tf_model_constant(
            graph, {a.op.name: shape_a, b.op.name: shape_b}, [out.op.name],
            data_mode='random_zero_mean_with_zeros')

    def test_greater_equal(self):
        shape_a = [1, 4, 5]
        shape_b = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape_a)
            b = tf.placeholder(tf.float32, shape=shape_b)
            out = tf.greater_equal(a, b)
        self._test_tf_model_constant(
            graph, {a.op.name: shape_a, b.op.name: shape_b}, [out.op.name],
            data_mode='random_zero_mean_with_zeros')

    def test_strided_slice(self):
        shape = [3, 2, 3]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.strided_slice(a, [1, 0, 0], [2, -1, 3])
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_expand_dims(self):
        shape = [3, 2, 3]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.expand_dims(a, axis=-1)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_tile(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.tile(a, [1, 2, 3])
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_unary_activation_sigmoid(self):
        shape = [1, 5, 5, 6]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.sigmoid(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_unary_activation_relu(self):
        shape = [1, 5, 5, 6]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.nn.relu(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_unary_activation_relu6(self):
        shape = [1, 5, 5, 6]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.nn.relu6(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name], data_mode='random_large')

    def test_unary_activation_leaky_relu(self):
        shape = [1, 5, 5, 6]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.nn.leaky_relu(a, 0.15)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_unary_activation_tanh(self):
        shape = [1, 5, 5, 6]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.tanh(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_unary_activation_elu(self):
        shape = [1, 5, 5, 6]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.nn.elu(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_stack(self):
        shape = [1]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            out = tf.stack([a, b], axis=1)
        self._test_tf_model_constant(graph, {a.op.name: shape, b.op.name: shape}, [out.op.name])

    def test_gather_nd(self):
        shape = [2, 3, 2]
        indices = [[[0, 0], [0, 1]], [[1, 0], [1, 1]]]
        graph = tf.Graph()
        with graph.as_default():
            params = tf.placeholder(tf.float32, shape=shape)
            out = tf.gather_nd(params=params, indices=indices)
        self._test_tf_model_constant(graph, {params.op.name: shape}, [out.op.name])

    def test_scatter_nd(self):
        graph = tf.Graph()
        with graph.as_default():
            indices = tf.constant([[0], [2]])
            updates = tf.placeholder(tf.float32, shape=[2, 4, 4])
            shape = tf.constant([4, 4, 4])
            out = tf.scatter_nd(indices, updates, shape)
        self._test_tf_model_constant(graph, {updates.op.name: [2, 4, 4]}, [out.op.name])

    def test_constant_pad(self):
        shape = [1, 2, 2, 5]
        graph = tf.Graph()
        with graph.as_default():
            paddings = tf.constant([[0, 0], [1, 1], [2, 2], [0, 0]])
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.pad(a, paddings=paddings, mode='constant')
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_constant_pad_v2(self):
        shape = [1, 2, 2, 5]
        graph = tf.Graph()
        with graph.as_default():
            paddings = tf.constant([[0, 0], [1, 1], [2, 2], [0, 0]])
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.pad(a, paddings=paddings, mode='constant', constant_values=1)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_mirror_pad(self):
        shape = [1, 2, 2, 5]
        graph = tf.Graph()
        with graph.as_default():
            paddings = tf.constant([[0, 0], [1, 1], [1, 1], [0, 0]])
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.pad(a, paddings=paddings, mode='reflect')
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_one_hot(self):
        shape = [2, 2, 3]
        graph = tf.Graph()
        # indices as constants
        with graph.as_default():
            indices = [[0, 2], [1, -1]]
            one_hot = tf.one_hot(indices=indices, depth=3)
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.add(one_hot, a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

        # indices as inputs
        # todo: add implementation
        # with graph.as_default():
        #     indices = tf.placeholder(tf.int32, shape=[1])
        #     out = tf.one_hot(indices=indices, depth=3)
        # self._test_tf_model_constant(graph, {indices.op.name: [1]}, [out.op.name], data_mode='random_zeros_ones')

    def test_size(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.int32, shape=[1])
            size = tf.size(a)
            out = tf.add(size, b)
        self._test_tf_model_constant(
            graph, {a.op.name: shape, b.op.name: [1]},
            [out.op.name], data_mode='random_int')

    def test_all(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.keras.backend.all(a, axis=0)
        self._test_tf_model_constant(
            graph, {a.op.name: shape}, [out.op.name],
            data_mode='random_zeros_ones', validate_bool_only=True)

    def test_any(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.keras.backend.any(a, axis=0)
        self._test_tf_model_constant(
            graph, {a.op.name: shape}, [out.op.name],
            data_mode='random_zeros_ones', validate_bool_only=True)

    def test_topk(self):
        shape = [12, 5, 9, 7]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            values, indices = tf.math.top_k(a, k=3)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [values.op.name])
        self._test_tf_model_constant(graph, {a.op.name: shape}, [indices.op.name])

    def test_argmax(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.argmax(a, axis=-1)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_argmin(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.argmin(a, axis=-1)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_fill(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.add(tf.fill(dims=shape, value=1.0), a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_clip(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.clip_by_value(a, clip_value_min=-0.2, clip_value_max=0.2)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_log_softmax(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.nn.log_softmax(a, axis=-1)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_slice(self):
        shape = [3, 4, 10]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.slice(a, begin=[0, 1, 0], size=[-1, 2, 3])
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])
        tf.reset_default_graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.strided_slice(a, begin=[0, 1, 0], end=[-1, 2, 5], strides=[1, 2, 1])
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_resize_bilinear(self):
        sizes = [[20, 30], [20, 30], [25, 45]]
        align_corners = [True, False, False]
        for sz, ac in zip(sizes, align_corners):
            graph = tf.Graph()
            with graph.as_default():
                x_input = tf.placeholder(tf.float32, shape=[None, 10, 10, 3])
                z = tf.image.resize_bilinear(x_input, size=sz,
                                             align_corners=ac)
                output_name = [z.op.name]
            self._test_tf_model_constant(graph, {x_input.op.name: [1, 10, 10, 3]},
                                         output_name)

    def test_resize_nearest_neighbor(self):
        sizes = [[20, 30]]
        align_corners = [False]
        for sz, ac in zip(sizes, align_corners):
            graph = tf.Graph()
            with graph.as_default():
                x_input = tf.placeholder(tf.float32, shape=[None, 10, 10, 3])
                z = tf.image.resize_nearest_neighbor(x_input, size=sz,
                                                     align_corners=ac)
                output_name = [z.op.name]
            self._test_tf_model_constant(graph, {x_input.op.name: [1, 10, 10, 3]},
                                         output_name)

    def test_strided_slice_ellipsis_mask(self):
        shape = [3, 4, 10]
        graph = tf.Graph()

        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.strided_slice(a, begin=[-1, 5], end=[1, -6], strides=[1, 1], end_mask=1, ellipsis_mask=2)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_crop_resize(self):
        graph = tf.Graph()
        with graph.as_default():
            # placeholder constructor returns a tensor not an op
            x = tf.placeholder(tf.float32, shape=[None, 20])
            # Make a redundant tensor. It should get trimmed
            dummy = tf.placeholder(tf.float32, shape=[None, 10])

            W = tf.Variable(tf.ones([20, 10]))
            b = tf.Variable(tf.ones([10]))

            y = tf.matmul(x, W) + b

        self._test_tf_model(graph, {x.op.name: [1, 20]}, [y.op.name])
        self._test_tf_model(graph, {x.op.name: [8, 20]}, [y.op.name])

    def test_where(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            c = tf.placeholder(tf.bool, shape=shape)
            out = tf.where(c, a, b)
        self._test_tf_model_constant(graph, {
            a.op.name: shape, b.op.name: shape, c.op.name: shape}, [out.op.name])

    @unittest.skipIf(not HAS_TF_1_14, 'Missing TF 1.14. Skipping test.')
    def test_where_v2(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            b = tf.placeholder(tf.float32, shape=shape)
            c = tf.placeholder(tf.bool, shape=shape)
            out = tf.where_v2(c, a, b)
        self._test_tf_model_constant(graph, {
            a.op.name: shape, b.op.name: shape, c.op.name: shape}, [out.op.name])

    def test_where_non_zero(self):
        shape = [3, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.where(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name], data_mode='random_zeros_ones')

    def test_transpose(self):
        shape = [4, 3, 1]
        graph = tf.Graph()
        with graph.as_default():
            axes = np.random.permutation(len(shape))
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.transpose(a, axes)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_space_to_depth(self):
        shapes = [[1, 2, 2, 1], [1, 2, 2, 3], [1, 4, 4, 1], [4, 4, 6, 2]]
        for shape in shapes:
            graph = tf.Graph()
            with graph.as_default():
                a = tf.placeholder(tf.float32, shape=shape)
                out = tf.space_to_depth(a, 2)
            self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_depth_to_space(self):
        shapes = [[1, 1, 1, 4], [1, 1, 1, 12], [1, 2, 2, 4], [4, 2, 3, 8]]
        for shape in shapes:
            graph = tf.Graph()
            with graph.as_default():
                a = tf.placeholder(tf.float32, shape=shape)
                out = tf.depth_to_space(a, 2)
            self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_space_to_batch_nd(self):
        shapes = [[1, 2, 2, 1], [1, 2, 2, 3], [1, 4, 4, 1], [2, 2, 4, 1]]
        for shape in shapes:
            graph = tf.Graph()
            with graph.as_default():
                a = tf.placeholder(tf.float32, shape=shape)
                out = tf.space_to_batch_nd(a, block_shape=[2, 2], paddings=[[0, 0], [0, 0]])
            self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_space_to_batch_nd_with_paddings(self):
        shapes = [[1, 2, 2, 1], [1, 2, 2, 3], [1, 4, 4, 1], [2, 2, 4, 1]]
        for shape in shapes:
            graph = tf.Graph()
            with graph.as_default():
                a = tf.placeholder(tf.float32, shape=shape)
                out = tf.space_to_batch_nd(a, block_shape=[2, 2], paddings=[[2, 2], [3, 3]])
            self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_batch_to_space_nd(self):
        shapes = [[4, 1, 1, 1], [4, 1, 1, 3], [4, 2, 2, 1], [8, 1, 3, 1]]
        for shape in shapes:
            graph = tf.Graph()
            with graph.as_default():
                a = tf.placeholder(tf.float32, shape=shape)
                out = tf.batch_to_space_nd(a, block_shape=[2, 2], crops=[[0, 0], [0, 0]])
            self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    def test_batch_to_space_nd_with_cropping(self):
        shapes = [[4, 3, 3, 1], [4, 3, 3, 3], [4, 2, 2, 1], [8, 5, 3, 1]]
        for shape in shapes:
            graph = tf.Graph()
            with graph.as_default():
                a = tf.placeholder(tf.float32, shape=shape)
                out = tf.batch_to_space_nd(a, block_shape=[2, 2], crops=[[1, 2], [1, 1]])
            self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])

    @unittest.skip('numeric')
    def test_lrn(self):
        shape = [1, 4, 4, 5]
        graph = tf.Graph()
        with graph.as_default():
            a = tf.placeholder(tf.float32, shape=shape)
            out = tf.nn.lrn(a)
        self._test_tf_model_constant(graph, {a.op.name: shape}, [out.op.name])


if __name__ == '__main__':
    unittest.main()
    # suite = unittest.TestSuite()
    # suite.addTest(TFSingleLayerTest('test_mirror_pad'))
    # unittest.TextTestRunner().run(suite)
