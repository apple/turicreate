# -*- coding: utf-8 -*-

#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import absolute_import as _
from __future__ import division as _
from __future__ import print_function as _

import logging as _logging
import os.path as _os_path

from six import string_types as _string_types
from tqdm import tqdm as _tqdm
import tensorflow as _tf

from tensorflow.python.framework import dtypes as _dtypes
from tensorflow.python.framework.convert_to_constants import (
    convert_variables_to_constants_v2 as _convert_variables_to_constants_v2,
)
from tensorflow.python.framework.function_def_to_graph import (
    function_def_to_graph as _function_def_to_graph,
)
from tensorflow.python.keras.saving import saving_utils as _saving_utils

from tensorflow.lite.python.util import (
    run_graph_optimizations as _run_graph_optimizations,
)
from tensorflow.lite.python.util import get_grappler_config as _get_grappler_config

from .converter import TF2Converter
from coremltools.converters.mil.frontend.tensorflow.basic_graph_ops import fill_outputs
from coremltools.converters.mil.frontend.tensorflow.tf_graph_pass import (
    constant_propagation,
    remove_variable_nodes,
    tensor_array_resource_removal,
    insert_get_tuple,
    delete_disconnected_nodes,
    fuse_dilation_conv,
)
from coremltools.converters.mil.frontend.tensorflow2.tf_graph_pass import (
    flatten_sub_graph_namespaces,
    rewrite_control_flow_functions,
)
from coremltools.converters.mil.frontend.tensorflow.tfssa import (
    NetworkEnsemble,
    SSAFunction,
)
from coremltools.converters.mil.frontend.tensorflow.parsed_tf_node import ParsedTFNode
from coremltools.converters.mil.frontend.tensorflow.load import TFLoader


class TF2Loader(TFLoader):
    def __init__(self, model, debug=False, **kwargs):
        """
        TensorFlow 2.x model loader.

        Parameters
        ----------
        model: Model created with TensorFlow 2.x
            One of the following model format:
                - TensorFlow tf.keras.Model object or HDF5 (.h5) file path
                - TensorFlow SavedModel directory path
                - TensorFlow list of concrete functions(s)
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
        msg = (
            "Expected model format: [SavedModel | [concrete_function] | "
            "tf.keras.Model | .h5], got {}"
        )
        if (
            isinstance(self.model, list)
            or isinstance(self.model, _tf.keras.Model)
            or isinstance(self.model, _string_types)
        ):
            cfs = []
            if isinstance(self.model, list):
                cfs = self.model
            if isinstance(self.model, _tf.keras.Model):
                cfs = self._concrete_fn_from_tf_keras_or_h5(self.model)
            elif isinstance(self.model, _string_types):
                if not _os_path.exists(self.model):
                    raise ValueError(
                        'Input model "{}" does not exist'.format(self.model)
                    )
                elif _os_path.isfile(self.model) and self.model.endswith(".h5"):
                    cfs = self._concrete_fn_from_tf_keras_or_h5(self.model)
                elif _os_path.isdir(self.model):
                    saved_model = _tf.saved_model.load(self.model)
                    sv = saved_model.signatures.values()
                    cfs = sv if isinstance(sv, list) else list(sv)
                else:
                    raise NotImplementedError(msg.format(self.model))

            graph_def = self._graph_def_from_concrete_fn(cfs)
            return self.extract_sub_graph(graph_def, outputs)
        else:
            raise NotImplementedError(msg.format(self.model))

    def _tf_ssa_from_graph_def(self, fn_name="main"):
        """Overwrites TFLoader._tf_ssa_from_graph_def()"""
        with _tf.Graph().as_default() as tf_graph:
            _tf.graph_util.import_graph_def(self._graph_def, name="")

        # sub-graphs' input shapes are required for extracting sub-graphs
        sg_input_shapes = self._populate_sub_graph_input_shapes(
            tf_graph, tf_graph._functions
        )

        # get graph_dict and sub-graphs' inputs / outputs
        graph_dict, inputs, outputs, ret = self._dict_from_graph_def(
            tf_graph, fn_name, sg_input_shapes
        )

        tf_ssa = NetworkEnsemble()
        for name, graph in graph_dict.items():
            tensor_array_resource_removal(graph)
            graph = insert_get_tuple(graph)
            graph = fill_outputs(graph)
            if name == "main":  # skip for sub-graphs as input can be also output
                delete_disconnected_nodes(graph)
            tf_ssa.functions[name] = SSAFunction(
                graph, inputs=inputs[name], outputs=outputs[name], ret=ret[name]
            )

        return tf_ssa

    def _program_from_tf_ssa(self):
        # Notes:
        # - "flatten_while_loop_namespaces" should be after "constant_propagation"
        #   as it changes node names which constant propagation pass is relying on
        #   to perform session.run(), renamed nodes are not understandable for TF.
        tf_passes = [
            # delete_asserts,  # FIXME: rdar://62472804
            constant_propagation,
            rewrite_control_flow_functions,
            flatten_sub_graph_namespaces,
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
                    _logging.exception('Exception in pass "{}": {}'.format(tf_pass, e))
                    _logging.info("Ignoring exception and continuing to next pass")

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

        converter = TF2Converter(self._tf_ssa, **self.kwargs)
        return converter.convert()

    def _populate_sub_graph_input_shapes(self, graph, graph_fns):
        """
        Populate function (sub-graph) input shapes from control flow op's inputs
        Note that the functions (sub-graphs) are not nested but the control flow
        ops are nested. The input shapes are used to extract sub-graphs from the
        parent graph (as the input of function_def_to_graph).

        Parameter
        ---------
        graph: tf.Graph
            TensorFlow graph.
        graph_fns: list of graph functions.
            List of TensorFlow graph functions.

        Returns
        -------
        sg_input_shapes: dict(str: list)
            Dictionary of function (sub-graph) name and input shape pairs.
        """
        sg_input_shapes = {}
        sub_graphs = []
        for op in graph.get_operations():
            if op.type not in {"StatelessIf", "If", "StatelessWhile", "While"}:
                continue

            sg1, sg2 = None, None
            if op.type in {"StatelessIf", "If"}:
                sg1 = op.get_attr("then_branch").name
                sg2 = op.get_attr("else_branch").name
            if op.type in {"StatelessWhile", "While"}:
                sg1 = op.get_attr("cond").name
                sg2 = op.get_attr("body").name

            # memorize input shapes for sub-graph conversions
            op_input_shapes = [i.get_shape() for i in op.inputs]
            sg_input_shapes.update({sg1: op_input_shapes, sg2: op_input_shapes})
            sub_graphs += [sg1, sg2]

        for name in sub_graphs:
            sg = graph_fns.get(name)
            fn_def = sg.definition
            op_input_shapes = sg_input_shapes[name]
            op_input_shapes = op_input_shapes[-len(fn_def.signature.input_arg) :]
            fn_graph = _function_def_to_graph(fn_def, input_shapes=op_input_shapes)
            sg_input_shapes.update(
                self._populate_sub_graph_input_shapes(fn_graph, graph_fns)
            )

        return sg_input_shapes

    @staticmethod
    def _dict_from_graph_def(graph, fn_name="main", sg_input_shapes=None):
        """
        Loads a tf.Graph and transform it into dictionary of ParsedTFNodes.
        Potentially contains multiple functions, in such case, recursively
        resolve functions (sub-graphs).

        Parameters
        ----------
        graph: tf.Graph
            TensorFlow graph.
        fn_name: str, optional, defaults to 'main'
            Function name of the graph.
        sg_input_shapes: dict(str: list)
            Dictionary of name and input shapes for functions / sub-graphs.

        Returns
        -------
        dict(str: dict(str: ParsedTFNode))
            Dictionary of function name and dictionary of node name and
            ParsedTFNode object.
        """
        graph_dict = {fn_name: {}}
        graph_inputs = {fn_name: []}
        graph_outputs = {fn_name: []}
        graph_ret = {fn_name: {}}

        for op in graph.get_operations():
            graph_dict[fn_name].update({op.name: ParsedTFNode(op.node_def)})

        for name, sg in graph._functions.items():
            sg_def = sg.definition
            input_shapes = sg_input_shapes[name]
            input_shapes = input_shapes[-len(sg_def.signature.input_arg) :]
            fn_graph = _function_def_to_graph(sg_def, input_shapes=input_shapes)

            graph_dict.update(
                TF2Loader._dict_from_graph_def(fn_graph, name, sg_input_shapes)[0]
            )
            graph_inputs.update({name: [t.name.split(":")[0] for t in fn_graph.inputs]})
            graph_outputs.update(
                {name: [t.name.split(":")[0] for t in fn_graph.outputs]}
            )

            # ret is a mapping from the output arg names from `signature` to the
            # outputs from `node_def` that should be returned by the function.
            sg_def_ret = sg_def.ret
            sg_def_ret["identity_0"] = sg_def_ret.pop("identity")
            graph_ret.update({name: sg_def_ret})

        return graph_dict, graph_inputs, graph_outputs, graph_ret

    @staticmethod
    def _concrete_fn_from_tf_keras_or_h5(keras_model):
        if isinstance(keras_model, _tf.keras.Model):
            input_signature = _saving_utils.model_input_signature(
                keras_model, keep_original_batch_size=True
            )
            fn = _saving_utils.trace_model_call(keras_model, input_signature)
        else:
            keras_model = _tf.keras.models.load_model(keras_model)
            input_signature = _saving_utils.model_input_signature(
                keras_model, keep_original_batch_size=True
            )
            fn = _saving_utils.trace_model_call(keras_model, input_signature)
        return [fn.get_concrete_function()]

    @staticmethod
    def _graph_def_from_concrete_fn(cfs):
        if len(cfs) != 1:
            raise NotImplementedError("Only a single concrete function is supported.")

        frozen_fn = _convert_variables_to_constants_v2(cfs[0], lower_control_flow=False)
        graph_def = frozen_fn.graph.as_graph_def(add_shapes=True)

        # run a Grappler's constant folding pass.
        fn_inputs = [t for t in frozen_fn.inputs if t.dtype != _dtypes.resource]
        graph_def = _run_graph_optimizations(
            graph_def,
            fn_inputs,
            frozen_fn.outputs,
            config=_get_grappler_config(["constfold", "dependency"]),
            graph=frozen_fn.graph,
        )
        return graph_def
