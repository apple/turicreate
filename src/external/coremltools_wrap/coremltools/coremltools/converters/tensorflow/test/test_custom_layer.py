import pytest
import unittest
import tempfile
import tensorflow as tf
import numpy as np
import os

from coremltools.converters.tensorflow import convert
from tensorflow.python.tools.freeze_graph import freeze_graph
from coremltools.proto import NeuralNetwork_pb2
from coremltools.converters.nnssa.coreml import shapes as custom_shape_update

class CustomLayerTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        self.test_temp_dir = tempfile.mkdtemp()

    def _simple_freeze(self, input_graph, input_checkpoint, output_graph,
                        output_node_names):
        # output_node_names is a string of names separated by comma
        freeze_graph(input_graph=input_graph,
                    input_saver="",
                    input_binary=False,
                    input_checkpoint=input_checkpoint,
                    output_node_names=output_node_names,
                    restore_op_name="save/restore_all",
                    filename_tensor_name="save/Const:0",
                    output_graph=output_graph,
                    clear_devices=True,
                    initializer_nodes="")

    def _test_tf_graph(self, graph,
                        output_feature_names,
                        input_name_shape_dict,
                        useCPUOnly=True,
                        add_custom_layers=False,
                        custom_conversion_functions={},
                        custom_shape_functions={}):
        # Create temporary model
        model_dir = self.test_temp_dir
        graph_def_file = os.path.join(model_dir, 'temp.pbtxt')
        checkpoint_file = os.path.join(model_dir, 'temp.ckpt')
        frozen_model_file = os.path.join(model_dir, 'temp.pb')

        tf.reset_default_graph()

        with tf.Session(graph = graph) as sess:
            # initialize
            sess.run(tf.global_variables_initializer())
            # prepare the tensorflow inputs
            feed_dict = {}
            for in_tensor_name in input_name_shape_dict:
                in_tensor_shape = input_name_shape_dict[in_tensor_name]
                feed_dict[in_tensor_name + ':0'] = np.random.rand(*in_tensor_shape)
            # run the result
            fetches = [graph.get_operation_by_name(name).outputs[0] for name in \
                output_feature_names]
            tf_result = sess.run(fetches, feed_dict=feed_dict)
            # save graph definition somewhere
            tf.train.write_graph(sess.graph, model_dir, graph_def_file)
            # save the weights
            saver = tf.train.Saver()
            saver.save(sess, checkpoint_file)

        # freeze the graph
        self._simple_freeze(
            input_graph=graph_def_file,
            input_checkpoint=checkpoint_file,
            output_graph=frozen_model_file,
            output_node_names=",".join(output_feature_names))


        coreml_model = convert(frozen_model_file,
                            outputs=output_feature_names,
                            inputs=input_name_shape_dict,
                            add_custom_layers=add_custom_layers,
                            custom_conversion_functions=custom_conversion_functions,
                            custom_shape_functions=custom_shape_functions)
        return coreml_model

# Custom Layer Tests
class TestCustomLayer(CustomLayerTest):
    # Test custom layer with conversion function
    def test_custom_topk(self):
        # Custom shape function
        def _shape_topk(layer_spec, input_shapes):
            params = layer_spec.topK
            value_shape = index_shape = input_shapes[0][:-1] + [params.K]
            output_shapes = [value_shape, index_shape]
            return output_shapes

        # Custom conversion function
        def _convert_topk(ssa_converter, node):
            coreml_nn_builder = ssa_converter._get_builder()
            constant_inputs = node.attr

            params = NeuralNetwork_pb2.CustomLayerParams()
            params.className = 'Top_K'
            params.description = "Custom layer that corresponds to the top_k TF op"
            params.parameters["sorted"].boolValue = node.attr.get('sorted')
            # get the value of k
            k = constant_inputs.get(node.inputs[1], 3)
            params.parameters["k"].intValue = k
            layer = coreml_nn_builder.add_custom(name=node.name,
                                        input_names=[node.inputs[0]],
                                        output_names=['output'],
                                        custom_proto_spec=params)
            custom_shape_update.propagate_single_layer(layer, ssa_converter.tensor_shapes, custom_shape_function=_shape_topk)

        graph = tf.Graph()
        with graph.as_default() as g:
            x = tf.placeholder(tf.float32, shape=[None, 8], name='input')
            y = tf.layers.dense(inputs=x, units=12, activation=tf.nn.relu)
            y = tf.nn.softmax(y, axis=1)
            y = tf.nn.top_k(y, k=3, sorted=False, name='output')

        output_name = ['output']
        inputs = {'input':[1, 8]}

        coreml_model = self._test_tf_graph(graph,
                                           output_name,
                                           inputs,
                                           add_custom_layers=True,
                                           custom_conversion_functions={'TopKV2': _convert_topk},
                                           custom_shape_functions={'TopKV2':_shape_topk})
        
        spec = coreml_model.get_spec()
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[3].custom)
        self.assertEqual('Top_K', layers[3].custom.className)
        self.assertEqual(3, layers[3].custom.parameters['k'].intValue)
        self.assertEqual(False, layers[3].custom.parameters['sorted'].boolValue)

    # Test custom layer with no custom conversion funtion provided path
    def test_custom_acos(self):
        # Custom Shape function
        def _shape_acos(layer_spec, input_shapes):
            return input_shapes[:]

        graph = tf.Graph()
        with graph.as_default() as g:
            x = tf.placeholder(tf.float32, shape=[None, 8], name='input')
            y = tf.layers.dense(inputs=x, units=12, activation=tf.nn.relu)
            y = tf.math.acos(y, name='output')
        
        output_name = ['output']
        inputs = {'input':[1, 8]}

        coreml_model = self._test_tf_graph(graph,
                                           output_name,
                                           inputs,
                                           add_custom_layers=True,
                                           custom_shape_functions={'Acos':_shape_acos})
        
        spec = coreml_model.get_spec()
        layers = spec.neuralNetwork.layers
        self.assertIsNotNone(layers[2].custom)
        self.assertEqual('Acos', layers[2].custom.className)

if __name__ == '__main__':
    unittest.main() 
