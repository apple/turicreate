# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import os, sys
import tensorflow.compat.v1 as tf
import numpy as np
import pytest
import unittest
import shutil, tempfile
from tensorflow.python.tools.freeze_graph import freeze_graph
from tensorflow.tools.graph_transforms import TransformGraph

import coremltools

# local to pytest
from test_utils import generate_data, tf_transpose

DEBUG = False

def _parse_coreml_input_shapes(mlmodel):
    return {x.name : list(x.type.multiArrayType.shape) for x in
        mlmodel._spec.description.input}

def _parse_coreml_name_to_tf(coreml_name):
    if coreml_name.endswith('__invar__'):
        tf_name = coreml_name.replace('__invar__', '')
    elif coreml_name.endswith('__outvar__'):
        tf_name = coreml_name.replace('__outvar__', '')
    else:
        tf_name = coreml_name
    return tf_name


class TFNetworkTest(unittest.TestCase):

    @classmethod
    def setUpClass(self):
        """
        Set up the unit test by loading common utilities.
        """

    def _get_tf_tensor_name(self, graph, name):
        """
        Convenience function to get the name of first output tensor of an op with name
        """
        return graph.get_operation_by_name(name).outputs[0].name

    def _simple_freeze(self, input_graph, input_checkpoint, output_graph, output_node_names):
        # output_node_names is a string of names separated by comma
        freeze_graph(
            input_graph=input_graph,
            input_saver="",
            input_binary=True,
            input_checkpoint=input_checkpoint,
            output_node_names=output_node_names,
            restore_op_name="save/restore_all",
            filename_tensor_name="save/Const:0",
            output_graph=output_graph,
            clear_devices=True,
            initializer_nodes="")

    def _quantize_static_tf_model(self, logdir, model_path, output_names):

        with open(model_path, 'rb') as f:
            serialized = f.read()

        gdef = tf.GraphDef()
        gdef.ParseFromString(serialized)

        tf.reset_default_graph()
        graph = tf.Graph()
        with graph.as_default() as g:
            transforms = [
                "add_default_attributes", "remove_nodes(op=Identity, op=CheckNumerics)",
                "fold_constants(ignore_errors=true)", "fold_batch_norms", "fold_old_batch_norms",
                "quantize_weights(minimum_size=1)", "quantize_nodes", "strip_unused_nodes",
                "sort_by_execution_order"
            ]

            transformed_graph_def = TransformGraph(gdef, [], output_names, transforms)
            tf.import_graph_def(transformed_graph_def, name='')

        tf.train.write_graph(graph, logdir, "./tf_quantized_frozen.pb", as_text=False)
        return os.path.join(logdir, 'tf_quantized_frozen.pb')

    def _test_tf_model(
            self,
            graph,
            input_shapes,
            output_node_names,
            data_mode='random',
            input_refs=None,
            delta=1e-2,
            use_cpu_only=False,
            graph_optimizations="freeze",  # one of ["freeze", "convert_variables_to_constants", None]
            quantize_tf_model=False,
            quantize_mlmodel=False,
            quantize_config={}):
        """
        Common entry to testing routine.
        graph - defined TensorFlow graph.
        input_shapes -  dict str:shape for each input op (placeholder)
        output_node_names - output_node_names, a list of strings
        data_mode - auto-generated input vectors, can be 'random', 'zeros', 'ones', 'linear', etc.
        input_refs - a dictionary of reference input in tensorFlow axis order, each entry is str:shape.
            When using auto-generated input vectors, set input_refs to None.
        delta - maximum difference of normalized TensorFlow and CoreML outputs
        use_cpu_only - If True, instantiate and run CoreML model with CPU only
        graph_optimizations == "freeze" - Force TensorFlow graph to be frozen before converting.
        quantize_tf_model - If True, try to quantize TensorFlow model before converting
        quantize_mlmodel - If True, quantize the mlmodel after converting.
        quantize_config - Dictionary with test quantization parameters
        """

        # Some file processing
        model_dir = tempfile.mkdtemp()
        graph_def_file = os.path.join(model_dir, 'tf_graph.pb')
        checkpoint_file = os.path.join(model_dir, 'tf_model.ckpt')
        static_model_file = os.path.join(model_dir, 'tf_static.pb')
        coreml_model_file = os.path.join(model_dir, 'coreml_model.mlmodel')

        # add a saver
        tf.reset_default_graph()
        if graph_optimizations == "freeze":
            with graph.as_default() as g:
                saver = tf.train.Saver()

        if input_refs is None:
            feed_dict = {
                self._get_tf_tensor_name(graph, name): generate_data(input_shapes[name], data_mode)
                for name in input_shapes
            }
        else:
            feed_dict = {
                self._get_tf_tensor_name(graph, name): input_refs[name]
                for name in list(input_refs.keys())
            }

        with tf.Session(graph=graph) as sess:
            # initialize
            initializer_op = tf.global_variables_initializer()
            sess.run(initializer_op)
            # run the result
            fetches = [graph.get_operation_by_name(name).outputs[0] for name in output_node_names]
            result = sess.run(fetches, feed_dict=feed_dict)
            # save graph definition somewhere
            tf.train.write_graph(sess.graph, model_dir, graph_def_file, as_text=False)
            # save the weights if freezing is needed
            if not graph_optimizations:
                static_model_file = graph_def_file
            elif graph_optimizations == "freeze":
                saver.save(sess, checkpoint_file)
                self._simple_freeze(
                    input_graph=graph_def_file,
                    input_checkpoint=checkpoint_file,
                    output_graph=static_model_file,
                    output_node_names=",".join(output_node_names))
            else:
                output_graph_def = tf.graph_util.convert_variables_to_constants(
                    sess, graph.as_graph_def(), output_node_names)
                with tf.gfile.GFile(static_model_file, "wb") as f:
                    f.write(output_graph_def.SerializeToString())

        # if TF needs to be quantized, quantize the graph
        if quantize_tf_model:
            static_model_file = self._quantize_static_tf_model(
                model_dir, static_model_file, output_node_names)

        # convert to CoreML
        mlmodel = coremltools.converters.tensorflow.convert(
            static_model_file,
            inputs=input_shapes,
            outputs=output_node_names,
            use_cpu_only=use_cpu_only)

        # Quantize MLModel if needed
        if quantize_mlmodel:
            from coremltools.models.neural_network.quantization_utils import quantize_weights
            nbits = quantize_config['nbits']
            mode = quantize_config['mode']
            mlmodel = quantize_weights(mlmodel, nbits, quantization_mode=mode)

        if DEBUG:
            print('\n mlmodel description: \n')
            from coremltools.models.neural_network.printer import print_network_spec
            print_network_spec(mlmodel.get_spec(), style='coding')
            mlmodel.save(coreml_model_file)
            print('\n mlmodel saved at %s' % coreml_model_file)

        coreml_input_names = [str(x) for x in mlmodel.input_description]
        coreml_input_shapes = _parse_coreml_input_shapes(mlmodel)

        # Transpose input data as CoreML requires
        coreml_inputs = {}
        for name in coreml_input_names:
            tfop_name = _parse_coreml_name_to_tf(name)
            if tfop_name in input_shapes:
                coreml_inputs[name] = tf_transpose(
                    feed_dict[self._get_tf_tensor_name(graph, tfop_name)])
            else:
                coreml_inputs[name] = np.zeros(coreml_input_shapes[name])

        # Run predict in CoreML
        coreml_output = mlmodel.predict(coreml_inputs, useCPUOnly=use_cpu_only)

        for idx, out_name in enumerate(output_node_names):
            tf_out = result[idx]
            if len(tf_out.shape) == 0:
                tf_out = np.array([tf_out])

            tp = tf_out.flatten()
            if out_name in coreml_output:
                coreml_out = coreml_output[out_name]
            elif out_name+'__outvar__' in coreml_output:
                coreml_out = coreml_output[out_name+'__outvar__']
            else:
                self.assertTrue(False, 'CoreML output not found')

            cp = coreml_out.flatten()

            self.assertTrue(tf_out.shape == coreml_out.shape)
            for i in range(len(tp)):
                max_den = max(1.0, tp[i], cp[i])
                self.assertAlmostEqual(tp[i] / max_den, cp[i] / max_den, delta=delta)

        # Cleanup files - models on disk no longer useful
        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)

    def _test_tf_model_constant(
            self,
            graph,
            input_shapes,
            output_node_names,
            data_mode='random_zero_mean',
            delta=1e-2,
            use_cpu_only=False,
            validate_bool_only=False):
        """
        Common entry to testing routine for graphs that have no variables.

        Parameters
        ----------
        graph: tf.Graph()
            TensorFlow graph.
        input_shapes: dict [str : shape]
            Shapes for each input (placeholder).
        output_node_names: list of str
            Output tensor names.
        data_mode: str
            Data mode for the placeholder data generation.
        input_refs: a dictionary of reference input in tensorFlow axis order.
            Each entry is str:shape. When using auto-generated input vectors,
            set input_refs to None.
        delta: float
            Delta for error checking, default 1e-2.
        use_cpu_only: bool
            If true, force use CPU only, default False.
        validate_bool_only: bool
            If true, only validate it's zero or non-zero, otherwise, validate
            float values, default False.
        """

        model_dir = tempfile.mkdtemp()
        frozen_model_file = os.path.join(model_dir, 'tf_frozen.pb')
        coreml_model_file = os.path.join(model_dir, 'coreml_model.mlmodel')

        feed_dict = {
            self._get_tf_tensor_name(graph, name): generate_data(input_shapes[name], data_mode)
            for name in input_shapes
        }

        with tf.Session(graph=graph) as sess:
            # initialize
            sess.run(tf.global_variables_initializer())
            # run the result
            fetches = []
            for name in output_node_names:
                fetches += graph.get_operation_by_name(name).outputs

            result = sess.run(fetches, feed_dict=feed_dict)

            output_graph_def = tf.graph_util.convert_variables_to_constants(
                sess,  # The session is used to retrieve the weights
                tf.get_default_graph().as_graph_def(
                ),  # The graph_def is used to retrieve the nodes
                output_node_names  # The output node names are used to select the useful nodes
            )
            with tf.gfile.GFile(frozen_model_file, 'wb') as f:
                f.write(output_graph_def.SerializeToString())

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
            print('\n mlmodel saved at %s' % coreml_model_file)

        # Transpose input data as CoreML requires
        coreml_inputs = {
            name: tf_transpose(feed_dict[self._get_tf_tensor_name(graph, name)])
            for name in input_shapes
        }

        # Run predict in CoreML
        coreml_output = mlmodel.predict(coreml_inputs, useCPUOnly=use_cpu_only)

        idx = 0
        for node_name in output_node_names:
            num_outputs = len(graph.get_operation_by_name(node_name).outputs)
            for out_id in range(num_outputs):
                tf_out = result[idx]
                if len(tf_out.shape) == 0:
                    tf_out = np.array([tf_out])
                tp = tf_out.flatten()
                out_name = node_name if num_outputs == 1 else node_name + '_' + str(out_id)
                coreml_out = coreml_output[out_name]
                cp = coreml_out.flatten()

                self.assertTrue(tf_out.shape == coreml_out.shape, msg=(tf_out.shape, 'vs.', coreml_out.shape))

                if validate_bool_only:
                    cp = np.logical_and(cp, cp)
                for i in range(len(tp)):
                    max_den = max(1.0, tp[i], cp[i])
                    self.assertAlmostEqual(tp[i] / max_den, cp[i] / max_den, delta=delta)

                idx += 1

        # Cleanup files - models on disk no longer useful
        if os.path.exists(model_dir):
            shutil.rmtree(model_dir)
