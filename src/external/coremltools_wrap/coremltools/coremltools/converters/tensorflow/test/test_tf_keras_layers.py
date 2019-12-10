import unittest
import sys, os, shutil, tempfile
import tensorflow as tf
import numpy as np
import coremltools

from tensorflow.python.tools.freeze_graph import freeze_graph
from tensorflow.keras import backend as K
from tensorflow.keras.models import Sequential, Model
from tensorflow.keras.layers import Dense, Activation, Conv2D, Conv1D, Flatten, BatchNormalization, Conv2DTranspose, \
    SeparableConv2D

from test_utils import generate_data, tf_transpose
from test_base import TFNetworkTest

DEBUG = False


class TFKerasNetworkTest(TFNetworkTest):
    @classmethod
    def setUpClass(self):
        """Set up the unit test by loading common utilities.
        """
        K.set_learning_phase(0)

    def _test_keras_model(
            self, model, data_mode='random', delta=1e-2, use_cpu_only=True, has_variables=True):
        """Saves out the backend TF graph from the Keras model and tests it  
        """

        model_dir = tempfile.mkdtemp()
        graph_def_file = os.path.join(model_dir, 'tf_graph.pb')
        checkpoint_file = os.path.join(model_dir, 'tf_model.ckpt')
        frozen_model_file = os.path.join(model_dir, 'tf_frozen.pb')
        coreml_model_file = os.path.join(model_dir, 'coreml_model.mlmodel')

        input_shapes = {inp.op.name: inp.shape.as_list() for inp in model.inputs}
        for name, shape in input_shapes.items():
            input_shapes[name] = [dim if dim is not None else 1 for dim in shape]

        output_node_names = [output.op.name for output in model.outputs]

        tf_graph = K.get_session().graph
        tf.reset_default_graph()
        if has_variables:
            with tf_graph.as_default() as g:
                saver = tf.train.Saver()

        # TODO - if Keras backend has_variable is False, we're not making variables constant
        with tf.Session(graph=tf_graph) as sess:
            sess.run(tf.global_variables_initializer())
            feed_dict = {}
            for name, shape in input_shapes.items():
                tensor_name = tf_graph.get_operation_by_name(name).outputs[0].name
                feed_dict[tensor_name] = generate_data(shape, data_mode)
            # run the result
            fetches = [
                tf_graph.get_operation_by_name(name).outputs[0] for name in output_node_names
            ]
            result = sess.run(fetches, feed_dict=feed_dict)
            # save graph definition somewhere
            tf.train.write_graph(sess.graph, model_dir, graph_def_file, as_text=False)

            # freeze_graph() has been raising error with tf.keras models since no
            # later than TensorFlow 1.6, so we're not using freeze_graph() here.
            # See: https://github.com/tensorflow/models/issues/5387
            output_graph_def = tf.graph_util.convert_variables_to_constants(
                sess,  # The session is used to retrieve the weights
                tf_graph.as_graph_def(),  # The graph_def is used to retrieve the nodes
                output_node_names  # The output node names are used to select the useful nodes
            )
            with tf.gfile.GFile(frozen_model_file, "wb") as f:
                f.write(output_graph_def.SerializeToString())

        K.clear_session()

        # convert to CoreML
        mlmodel = coremltools.converters.tensorflow.convert(
            frozen_model_file,
            inputs=input_shapes,
            outputs=output_node_names,
            use_cpu_only=use_cpu_only)

        if DEBUG:
            print('\n mlmodel description: \n')
            from coremltools.models.neural_network.printer import print_network_spec
            print_network_spec(mlmodel.get_spec(), style='coding')
            mlmodel.save(coreml_model_file)
            print('\n mlmodel saved at %s' % (coreml_model_file))

        # Transpose input data as CoreML requires
        coreml_inputs = {
            name: tf_transpose(feed_dict[self._get_tf_tensor_name(tf_graph, name)])
            for name in input_shapes
        }

        # Run predict in CoreML
        coreml_output = mlmodel.predict(coreml_inputs, useCPUOnly=use_cpu_only)

        for idx, out_name in enumerate(output_node_names):
            tf_out = result[idx]
            if len(tf_out.shape) == 0:
                tf_out = np.array([tf_out])
            tp = tf_out.flatten()
            coreml_out = coreml_output[out_name]
            cp = coreml_out.flatten()
            self.assertTrue(tf_out.shape == coreml_out.shape)
            for i in range(len(tp)):
                max_den = max(1.0, tp[i], cp[i])
                self.assertAlmostEqual(tp[i] / max_den, cp[i] / max_den, delta=delta)

        # Cleanup files - models on disk no longer useful
        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)


class KerasBasicNumericCorrectnessTest(TFKerasNetworkTest):
    def test_dense_softmax(self):
        np.random.seed(1987)
        # Define a model
        model = Sequential()
        model.add(Dense(32, input_shape=(32,), activation='softmax'))
        # Set some random weights
        model.set_weights([np.random.rand(*w.shape) for w in model.get_weights()])
        # Test it
        self._test_keras_model(model)


if __name__ == '__main__':
    unittest.main()
