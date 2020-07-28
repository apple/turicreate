# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import absolute_import as _
from __future__ import division as _
from __future__ import print_function as _

import logging
import os
import gc

import six

import tensorflow as tf

from tempfile import mktemp
from .basic_graph_ops import fill_outputs
from .converter import TFConverter
from .tf_graph_pass import *  # pylint: disable=unused-wildcard-import,wildcard-import
from .tfssa import NetworkEnsemble, SSAFunction
from .parsed_tf_node import ParsedTFNode
from coremltools.converters._profile_utils import _profile
from tqdm import tqdm as _tqdm
from distutils.version import StrictVersion as _StrictVersion


class TFLoader:
    """Abstract class for TensorFlow model loader."""

    def __init__(self, model, debug=False, **kwargs):
        """
        TensorFlow model loader.

        Parameters
        ----------
        model: TensorFlow model
            Model generated using TensorFlow.
        debug: bool, optional, defaults to False
            If true, display verbose logging and visualizations.
        kwargs: dict(str, Any), optional, defaults to None
            Dictionary of additional arguments.
        """
        self.model = model
        self.debug = debug
        self.kwargs = kwargs
        self._graph_def = None
        self._tf_ssa = None

    @_profile
    def load(self):
        """Load TensorFlow model into MIL program."""

        logging.info("Loading TensorFlow model '{}'".format(self.model))
        outputs = self.kwargs.get("outputs", None)
        self._graph_def = self._graph_def_from_model(outputs)

        if self._graph_def is not None and len(self._graph_def.node) == 0:
            msg = "tf.Graph should have at least 1 node, Got empty graph."
            raise ValueError(msg)

        self._tf_ssa = self._tf_ssa_from_graph_def()

        del self._graph_def
        gc.collect()

        if self.debug:
            import graphviz

            dot_string = self._tf_ssa.get_dot_string(
                annotation=True, name_and_op_style=True, highlight_debug_nodes=[]
            )
            graphviz.Source(dot_string).view(
                filename="/tmp/ssa_before_tf_passes", cleanup=True
            )

        program = self._program_from_tf_ssa()
        logging.debug("program:\n{}".format(program))
        return program

    # @abstractmethod
    def _graph_def_from_model(self, outputs=None):
        """Load TensorFlow model into GraphDef. Overwrite for different TF versions."""
        pass

    # @abstractmethod
    def _tf_ssa_from_graph_def(self, fn_name="main"):
        """Load GraphDef and parse into NetworkEnsemble (TFSSA)."""
        pass

    # @abstractmethod
    def _program_from_tf_ssa(self):
        """Load NetworkEnsemble (TFSSA) and parse into MIL program."""
        pass

    @staticmethod
    def extract_sub_graph(graph_def, outputs=None):
        """Extract sub-graph based on user-provided outputs."""
        if outputs is None or len(outputs) == 0:
            return graph_def
        msg = "Extracting sub-graph based on outputs '{}' from the full model"
        logging.debug(msg.format(outputs))
        outputs = outputs if isinstance(outputs, list) else [outputs]
        outputs = [i.split(":")[0] for i in outputs]
        if tf.__version__ < _StrictVersion("1.13.1"):
            return tf.graph_util.extract_sub_graph(graph_def, outputs)
        else:
            return tf.compat.v1.graph_util.extract_sub_graph(graph_def, outputs)


class TF1Loader(TFLoader):
    def __init__(self, model, debug=False, **kwargs):
        """
        TensorFlow 1.x model loader.

        Parameters
        ----------
        model: Model created with TensorFlow 1.x
            One of the following model format:
                - TensorFlow tf.Graph object or frozen graph (.pb) file path
                - TensorFlow tf.keras.Model object or HDF5 (.h5) file path
                - TensorFlow SavedModel directory path
        debug: bool, optional. Defaults to False.
            This flag should generally be False except for debugging purposes
            for diagnosing conversion errors. Setting this flag to True will
            cause graph pass errors to be ignored, forcefully returning a
            NetworkEnsemble object.
        kwargs: dict(str, Any), optional
            Dictionary of additional arguments.
        """
        TFLoader.__init__(self, model, debug, **kwargs)

    def _graph_def_from_model(self, outputs=None):
        """Overwrites TFLoader._graph_def_from_model()"""
        msg = "Expected model format: [tf.Graph | .pb | SavedModel | tf.keras.Model | .h5], got {}"
        if isinstance(self.model, tf.Graph) and hasattr(self.model, "as_graph_def"):
            graph_def = self.model.as_graph_def(add_shapes=True)
            return self.extract_sub_graph(graph_def, outputs)
        elif isinstance(self.model, tf.keras.Model):
            graph_def = self._from_tf_keras_model(self.model)
            return self.extract_sub_graph(graph_def, outputs)
        elif isinstance(self.model, six.string_types):
            if not os.path.exists(str(self.model)):
                raise ValueError('Input model "{}" does not exist'.format(self.model))
            elif os.path.isfile(str(self.model)) and self.model.endswith(".pb"):
                if tf.__version__ < _StrictVersion("1.13.1"):
                    with open(self.model, "rb") as f:
                        gd = tf.GraphDef()
                        gd.ParseFromString(f.read())
                    with tf.Graph().as_default() as graph:
                        tf.import_graph_def(gd, name="")
                else:
                    with tf.io.gfile.GFile(self.model, "rb") as f:
                        gd = tf.compat.v1.GraphDef()
                        gd.ParseFromString(f.read())
                    with tf.Graph().as_default() as graph:
                        tf.graph_util.import_graph_def(gd, name="")
                graph_def = graph.as_graph_def(add_shapes=True)
                return self.extract_sub_graph(graph_def, outputs)
            elif os.path.isfile(str(self.model)) and self.model.endswith(".h5"):
                graph_def = self._from_tf_keras_model(self.model)
                return self.extract_sub_graph(graph_def, outputs)
            elif os.path.isdir(str(self.model)):
                graph_def = self._from_saved_model(self.model)
                return self.extract_sub_graph(graph_def, outputs)
            else:
                raise NotImplementedError(msg.format(self.model))
        else:
            raise NotImplementedError(msg.format(self.model))

    def _tf_ssa_from_graph_def(self, fn_name="main"):
        """Overwrites TFLoader._tf_ssa_from_graph_def()"""
        graph_dict = {}
        for node in self._graph_def.node:
            graph_dict[node.name] = ParsedTFNode(node)

        tensor_array_resource_removal(graph_dict)
        graph = insert_get_tuple(graph_dict)
        graph = fill_outputs(graph)
        delete_disconnected_nodes(graph)

        tf_ssa = NetworkEnsemble()
        tf_ssa.functions[fn_name] = SSAFunction(graph)
        return tf_ssa

    def _program_from_tf_ssa(self):
        """Overwrites TFLoader._mil_program_from_tf_ssa()"""
        # Applying frontend passes on TFSSA. Note that these are different from
        # passes applied to MIL in TF frontend.
        tf_passes = [
            delete_asserts,
            functionalize_loops,
            constant_propagation,
            cond_to_where,
            remove_variable_nodes,
            fuse_dilation_conv,
        ]

        if self.debug:
            for tf_pass in _tqdm(
                tf_passes, desc="Running TensorFlow Graph Passes", unit=" passes"
            ):
                try:
                    tf_pass(self._tf_ssa)
                except Exception as e:
                    logging.exception('Exception in pass "{}": {}'.format(tf_pass, e))
                    logging.info("Ignoring exception and continuing to next pass")
        else:
            for tf_pass in _tqdm(
                tf_passes, desc="Running TensorFlow Graph Passes", unit=" passes"
            ):
                tf_pass(self._tf_ssa)

        if self.debug:
            import graphviz

            dot_string = self._tf_ssa.get_dot_string(
                annotation=True, name_and_op_style=True, highlight_debug_nodes=[]
            )
            graphviz.Source(dot_string).view(
                filename="/tmp/ssa_after_tf_passes", cleanup=True
            )

        converter = TFConverter(self._tf_ssa, **self.kwargs)
        return converter.convert()

    @staticmethod
    def _from_saved_model(saved_model_dir):
        from tensorflow.python.tools import freeze_graph

        # must import here as tf.contrib is only available on TF 1.x
        from tensorflow.contrib.saved_model.python.saved_model import reader

        saved_model_tags = reader.get_saved_model_tag_sets(saved_model_dir)[0]
        if not saved_model_tags:
            msg = "Unsupported SavedModel directory format: no tag_sets available"
            raise NotImplementedError(msg)

        # get model outputs
        output_node_names = []
        if tf.__version__ < _StrictVersion("1.13.1"):
            sess = tf.Session()
        else:
            sess = tf.compat.v1.Session()
        metagraph = tf.saved_model.loader.load(
            sess, saved_model_tags, saved_model_dir
        )
        for sd in metagraph.signature_def.values():
            output_node_names += [o.name.split(":")[0] for o in sd.outputs.values()]

        sess.close()

        # get frozen graph
        output_graph = mktemp()
        tf.compat.v1.reset_default_graph() if tf.__version__ >= _StrictVersion("1.13.1") else tf.reset_default_graph()
        freeze_graph.freeze_graph(
            input_graph=None,
            input_saver=None,
            input_binary=None,
            input_checkpoint=None,
            output_node_names=",".join(output_node_names),
            restore_op_name=None,
            filename_tensor_name=None,
            output_graph=output_graph,
            clear_devices=True,
            initializer_nodes="",
            variable_names_whitelist="",
            variable_names_blacklist="",
            input_meta_graph=None,
            input_saved_model_dir=saved_model_dir,
            saved_model_tags=",".join(saved_model_tags),
        )

        if tf.__version__ < _StrictVersion("1.13.1"):
            graph_def = tf.GraphDef()
            with open(output_graph, "rb") as f:
                graph_def.ParseFromString(f.read())
            graph_def = tf.graph_util.remove_training_nodes(graph_def)
        else:
            graph_def = tf.compat.v1.GraphDef()
            with open(output_graph, "rb") as f:
                graph_def.ParseFromString(f.read())
            graph_def = tf.compat.v1.graph_util.remove_training_nodes(graph_def)
        with tf.Graph().as_default() as graph:
            tf.graph_util.import_graph_def(graph_def, name="")
        return graph.as_graph_def(add_shapes=True)

    @staticmethod
    def _from_tf_keras_model(keras_model):
        from tensorflow.python.keras.saving import saving_utils
        from tensorflow.python.framework.convert_to_constants import (
            convert_variables_to_constants_v2,
        )

        if not isinstance(keras_model, tf.keras.Model):
            keras_model = tf.keras.models.load_model(keras_model, None)

        tf.keras.backend.clear_session()
        tf.keras.backend.set_learning_phase(False)
        fn = saving_utils.trace_model_call(keras_model)
        cf = fn.get_concrete_function()
        try:
            frozen_fn = convert_variables_to_constants_v2(cf)
            return frozen_fn.graph.as_graph_def(add_shapes=True)
        except Exception:
            raise NotImplementedError("Unhandled tf.keras model format")
