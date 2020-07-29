import os


def _shape_notation(int_shape):
    X = ["S", "B", "C", "H", "W"]
    return [X[i] for i in int_shape]


def plot_graph(graph, graph_img_path="graph.png", show_coreml_mapped_shapes=False):
    """
    Plot graph using pydot

    It works in two steps:
    1. Add nodes to pydot
    2. connect nodes added in pydot

    :param graph
    :return: writes down a png/pdf file using dot
    """

    try:
        # pydot-ng is a fork of pydot that is better maintained.
        import pydot_ng as pydot  # type: ignore
    except:
        # pydotplus is an improved version of pydot
        try:
            import pydotplus as pydot  # type: ignore
        except:
            # Fall back on pydot if necessary.
            try:
                import pydot  # type: ignore
            except:
                return None

    dot = pydot.Dot()
    dot.set("rankdir", "TB")
    dot.set("concentrate", True)
    dot.set_node_defaults(shape="record")

    # Add nodes corresponding to graph inputs
    graph_inputs = []
    for input_ in graph.inputs:
        if show_coreml_mapped_shapes:
            if input_[0] in graph.onnx_coreml_shape_mapping:
                shape = tuple(
                    _shape_notation(graph.onnx_coreml_shape_mapping[input_[0]])
                )
            else:
                shape = "NA, "
        else:
            shape = tuple(input_[2])
        label = "%s\n|{|%s}|{{%s}|{%s}}" % ("Input", input_[0], "", str(shape))
        pydot_node = pydot.Node(input_[0], label=label)
        dot.add_node(pydot_node)
        graph_inputs.append(input_[0])

    # Traverse graph and add nodes to pydot
    for node in graph.nodes:
        inputlabels = ""
        for input_ in node.inputs:
            if show_coreml_mapped_shapes:
                if input_ in graph.onnx_coreml_shape_mapping:
                    inputlabels += (
                        str(
                            tuple(
                                _shape_notation(graph.onnx_coreml_shape_mapping[input_])
                            )
                        )
                        + ", "
                    )
                else:
                    inputlabels += "NA, "
            else:
                if input_ in graph.shape_dict:
                    inputlabels += str(tuple(graph.shape_dict[input_])) + ", "
                else:
                    inputlabels += "NA, "
        outputlabels = ""
        for output_ in node.outputs:
            if show_coreml_mapped_shapes:
                if output_ in graph.onnx_coreml_shape_mapping:
                    outputlabels += (
                        str(
                            tuple(
                                _shape_notation(
                                    graph.onnx_coreml_shape_mapping[output_]
                                )
                            )
                        )
                        + ", "
                    )
                else:
                    outputlabels += "NA, "
            else:
                if output_ in graph.shape_dict:
                    outputlabels += str(tuple(graph.shape_dict[output_])) + ", "
                else:
                    outputlabels += "NA, "
        output_names = ", ".join([output_ for output_ in node.outputs])
        input_names = ", ".join([input_ for input_ in node.inputs])
        label = "%s\n|{{%s}|{%s}}|{{%s}|{%s}}" % (
            node.op_type,
            input_names,
            output_names,
            inputlabels,
            outputlabels,
        )
        pydot_node = pydot.Node(node.name, label=label)
        dot.add_node(pydot_node)

    # add edges
    for node in graph.nodes:
        for child in node.children:
            # add edge in pydot
            dot.add_edge(pydot.Edge(node.name, child.name))
        for input_ in node.inputs:
            if input_ in graph_inputs:
                dot.add_edge(pydot.Edge(input_, node.name))

    # write out the image file
    _, extension = os.path.splitext(graph_img_path)
    if not extension:
        extension = "pdf"
    else:
        extension = extension[1:]
    dot.write(graph_img_path, format=extension)
