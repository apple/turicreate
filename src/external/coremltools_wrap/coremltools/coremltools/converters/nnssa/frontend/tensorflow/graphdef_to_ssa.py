# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import tensorflow as tf
from .parse import graphdef_to_dict
from ...commons.basic_graph_ops import *
from ...nnssa import *
from .graph_pass import insert_get_tuple
from .graph_pass import delete_disconnected_nodes
from .graph_pass import tensor_array_resource_removal


def load_tf_graph(graph_file):
    """
    Given a graphdef file on disk, loads it, returning a pair of
    graph_def and tf.Graph
    """
    # We load the protobuf file from the disk and parse it to retrieve the
    # unserialized graph_def
    with tf.io.gfile.GFile(graph_file, "rb") as f:
        graph_def = tf.compat.v1.GraphDef()
        graph_def.ParseFromString(f.read())

    # Then, we import the graph_def into a new Graph and returns it
    with tf.Graph().as_default() as graph:
        # The name var will prefix every op/nodes in your graph
        # Since we load everything in a new graph, this is not needed
        tf.import_graph_def(graph_def, name="")
    return graph.as_graph_def(add_shapes=True), graph


def graphdef_to_ssa(graphdef_or_file, main_method_name='main'):
    """
    Loads a graphdef file and transform into NetworkEnsemble.
    """
    if isinstance(graphdef_or_file, (bytes, str)):
        gdorig, g = load_tf_graph(graphdef_or_file)
    else:
        gdorig = graphdef_or_file
        with tf.Graph().as_default() as g:
            tf.import_graph_def(gdorig, name="")

    gd = graphdef_to_dict(gdorig)
    tensor_array_resource_removal(gd)
    gd = insert_get_tuple(gd)
    gd = fill_outputs(gd)
    delete_disconnected_nodes(gd)

    ssa = NetworkEnsemble()
    ssa.functions[main_method_name] = SSAFunction(gd)
    return ssa
