import keras as _keras
import numpy as _np
import six

_KERAS_LAYERS_1D = [
    _keras.layers.Conv1D,
    _keras.layers.UpSampling1D,
    _keras.layers.ZeroPadding1D,
    _keras.layers.Cropping1D,
    _keras.layers.MaxPooling1D,
    _keras.layers.AveragePooling1D,
    _keras.layers.GlobalMaxPooling1D,
    _keras.layers.GlobalAveragePooling1D,
]

_KERAS_ACTIVATION_LAYERS = [
    _keras.layers.Activation,
    _keras.layers.advanced_activations.LeakyReLU,
    _keras.layers.advanced_activations.PReLU,
    _keras.layers.advanced_activations.ELU,
    _keras.layers.advanced_activations.ThresholdedReLU,
]

_KERAS_NORMALIZATION_LAYERS = [
    _keras.layers.BatchNormalization,
]

_KERAS_RECURRENT_LAYERS = [
    _keras.layers.recurrent.LSTM,
    _keras.layers.recurrent.SimpleRNN,
    _keras.layers.recurrent.GRU,
    _keras.layers.wrappers.Bidirectional,
]

_KERAS_MERGE_LAYERS = [
    _keras.layers.Add,
    _keras.layers.Multiply,
    _keras.layers.Average,
    _keras.layers.Maximum,
    _keras.layers.Concatenate,
    _keras.layers.Dot,
]

_KERAS_SKIP_LAYERS = [
    _keras.layers.core.Dropout,
    _keras.layers.core.SpatialDropout1D,
    _keras.layers.core.SpatialDropout2D,
]

from distutils.version import StrictVersion as _StrictVersion

if _keras.__version__ >= _StrictVersion("2.2.0"):
    from keras.engine.input_layer import InputLayer
else:
    from keras.engine.topology import InputLayer


def _to_list(x):
    if type(x) is not list:
        return [x]
    else:
        return x


def _insert_to_dict(d, key, e):
    # d is a dict where key maps to a list
    if key not in d:
        d[key] = []
    if e not in d[key]:
        d[key].append(e)


def _is_merge_layer(layer):
    for lt in _KERAS_MERGE_LAYERS:
        if isinstance(layer, lt):
            return True
    return False


class NetGraph(object):
    """
    Attributes:
    layer_list - a list of layer names in the Keras model
    connection_map - a map where the key is a layer, the value is a list of its successors
    reverse_connection_map - a map where the key is a layer, the value is a list of its predecessors
    keras_layer_map - a map where the key is a layer name, the value is Keras layer
    model - a reference of the keras model.
    blob_names - blob names for each one of the edge.
    """

    def __init__(self, model):
        self.layer_list = []
        self.edge_map = {}
        self.reverse_edge_map = {}
        self.keras_layer_map = {}

        self.input_layers = []
        self.output_layers = []
        self.layers_inputs = {}  # each layer's input blobs
        self.layers_outputs = {}  # each layer's output blobs

        # these will be pairs of the form (name, shape) because it'll show up on the interface
        self.optional_inputs = []
        self.optional_outputs = []
        self.layers_optional_inputs = {}
        self.layers_optional_outputs = {}

        self.model = model

    def _add_layer(self, keras_layer):
        # add a layer without adding connections.
        # when a layer exist alreday, this operation won't do anything
        layer = keras_layer.name
        if layer not in self.layer_list:
            self.layer_list.append(layer)
            self.keras_layer_map[layer] = keras_layer

    def _replace_blob_name(self, old_name, new_name):
        # replace a blob with a new name
        for l in self.layers_outputs:
            for idx, b in enumerate(self.layers_outputs[l]):
                if b == old_name:
                    self.layers_outputs[l][idx] = new_name

        for l in self.layers_inputs:
            for idx, b in enumerate(self.layers_inputs[l]):
                if b == old_name:
                    self.layers_inputs[l][idx] = new_name

    def get_predecessors(self, layer_name):
        if layer_name in self.reverse_edge_map:
            return self.reverse_edge_map[layer_name][:]  # needs to make a copy
        else:
            return []

    def get_successors(self, layer_name):
        if layer_name in self.edge_map:
            return self.edge_map[layer_name][:]  # needs to make a copy
        else:
            return []

    def get_keras_layer(self, layer_name):
        return self.keras_layer_map[layer_name]

    def get_coreml_layers(self, keras_layer):
        coreml_layers = []
        for key in self.keras_layer_map:
            if self.keras_layer_map[key] == keras_layer:
                coreml_layers.append(key)
        return coreml_layers

    def make_input_layers(self):
        """
        Extract the ordering of the input layers.
        """
        self.input_layers = []
        in_nodes = (
            self.model._inbound_nodes
            if hasattr(self.model, "_inbound_nodes")
            else self.model.inbound_nodes
        )
        if hasattr(self.model, "input_layers"):
            input_keras_layers = self.model.input_layers[:]
            self.input_layers = [None] * len(input_keras_layers)
            for layer in self.layer_list:
                keras_layer = self.keras_layer_map[layer]
                if isinstance(keras_layer, InputLayer):
                    if keras_layer in input_keras_layers:
                        idx = input_keras_layers.index(keras_layer)
                        self.input_layers[idx] = layer
        elif hasattr(self.model, "inputs"):
            for ts in _to_list(self.model.inputs):
                # search for the InputLayer that matches this ts
                for l in self.layer_list:
                    kl = self.keras_layer_map[l]
                    if isinstance(kl, InputLayer) and kl.input == ts:
                        self.input_layers.append(l)
        elif len(in_nodes) <= 1:
            for ts in _to_list(self.model.input):
                # search for the InputLayer that matches this ts
                for l in self.layer_list:
                    kl = self.keras_layer_map[l]
                    if isinstance(kl, InputLayer) and kl.input == ts:
                        self.input_layers.append(l)
        else:
            raise ValueError("Input values cannot be identified.")

    def make_output_layers(self):
        """
        Extract the ordering of output layers.
        """
        self.output_layers = []
        # import pytest; pytest.set_trace()
        if hasattr(self.model, "output_layers"):
            # find corresponding output layers in CoreML model
            # assume output layers are not shared
            # Helper function to recursively extract output layers
            # even if the model has a layer which is a nested model
            def extract_output_layers(keras_model):
                output_layers = []
                for layer in keras_model.output_layers:
                    if hasattr(layer, "output_layers"):
                        output_layers.extend(extract_output_layers(layer))
                    else:
                        output_layers.append(layer)
                return output_layers

            for kl in extract_output_layers(self.model):
                coreml_layers = self.get_coreml_layers(kl)
                if len(coreml_layers) > 0:
                    for cl in coreml_layers:
                        self.output_layers.append(cl)
        elif len(self.model.outputs) > 0:
            for model_output in self.model.outputs:
                for l in self.layer_list:
                    k_layer = self.keras_layer_map[l]
                    in_nodes = (
                        k_layer._inbound_nodes
                        if hasattr(k_layer, "_inbound_nodes")
                        else k_layer.inbound_nodes
                    )
                    for idx in range(len(in_nodes)):
                        out_tensor = k_layer.get_output_at(idx)
                        if out_tensor == model_output or (
                            out_tensor.name in model_output.name
                        ):
                            self.output_layers.append(l)
        if len(self.output_layers) == 0:
            raise ValueError("No outputs can be identified")

    def get_input_layers(self):
        return self.input_layers

    def get_output_layers(self):
        return self.output_layers

    def generate_blob_names(self):
        """
        Generate blob names for each one of the edge.  At this time, Keras does not
        support "fork" operation (a layer with more than 1 blob output). So we just
        use names of the src layer to identify a blob.  We also assume all neural
        networks are singly-connected graphs - which should be the case.
        """
        # generate blob names that represent edges in blob_name_map
        # because of the InputLayers, input blobs are also generated.

        # Generate each layer's input / output blob names
        for layer in self.layer_list:
            keras_layer = self.keras_layer_map[layer]
            # no need to generate InputLayers' blobs
            if not isinstance(keras_layer, InputLayer):
                # layer's input blob names depend on predecessors
                preds = self.get_predecessors(layer)
                for pred in preds:
                    blob_name = pred + "_output"
                    _insert_to_dict(self.layers_inputs, layer, blob_name)
                # layer's output blob is just named after itself
                blob_name = layer + "_output"
                _insert_to_dict(self.layers_outputs, layer, blob_name)

    def get_layer_blobs(self, layer):
        keras_layer = self.keras_layer_map[layer]
        if isinstance(keras_layer, InputLayer):
            return None, None
        else:
            input_blobs = self.layers_inputs[layer]
            output_blobs = self.layers_outputs[layer]
            if layer in self.layers_optional_inputs:
                input_blobs += self.layers_optional_inputs[layer]
            if layer in self.layers_optional_outputs:
                output_blobs += self.layers_optional_outputs[layer]
            return input_blobs, output_blobs

    def reset_model_input_names(self, new_names):
        # call this method after make_input_layers() is called
        if new_names is None:
            return
        if len(new_names) != len(self.input_layers):
            print("Input name length mismatch")
            return
        for i, in_layer in enumerate(self.input_layers):
            old_blob_name = in_layer + "_output"
            new_blob_name = new_names[i]
            succs = self.get_successors(in_layer)
            for succ in succs:
                idx = self.layers_inputs[succ].index(old_blob_name)
                self.layers_inputs[succ][idx] = new_blob_name

    def reset_model_output_names(self, new_names):
        if new_names is None:
            return
        if len(new_names) != len(self.output_layers):
            print("Output name length mismatch")
            return
        for i, out_layer in enumerate(self.output_layers):
            old_blob_name = self.layers_outputs[self.output_layers[i]][0]
            self._replace_blob_name(old_blob_name, new_names[i])

    # need to update both layer's in/out list and graph in/out ports
    def add_recurrent_optionals(self):
        # call this after blob names are generated
        for layer in self.layer_list:
            keras_layer = self.keras_layer_map[layer]
            if type(keras_layer) in _KERAS_RECURRENT_LAYERS:
                if not isinstance(keras_layer, _keras.layers.wrappers.Bidirectional):
                    hidden_size = keras_layer.units
                else:
                    hidden_size = keras_layer.forward_layer.units
                h_in_name = layer + "_h_in"
                h_out_name = layer + "_h_out"
                self.optional_inputs.append((h_in_name, hidden_size))
                self.optional_outputs.append((h_out_name, hidden_size))
                _insert_to_dict(self.layers_optional_inputs, layer, h_in_name)
                _insert_to_dict(self.layers_optional_outputs, layer, h_out_name)
                if isinstance(keras_layer, _keras.layers.recurrent.LSTM):
                    c_in_name = layer + "_c_in"
                    c_out_name = layer + "_c_out"
                    self.optional_inputs.append((c_in_name, hidden_size))
                    self.optional_outputs.append((c_out_name, hidden_size))
                    _insert_to_dict(self.layers_optional_inputs, layer, c_in_name)
                    _insert_to_dict(self.layers_optional_outputs, layer, c_out_name)
                elif isinstance(keras_layer, _keras.layers.wrappers.Bidirectional):
                    c_in_name = layer + "_c_in"
                    c_out_name = layer + "_c_out"
                    h_in_name_rev = layer + "_h_in_rev"
                    c_in_name_rev = layer + "_c_in_rev"
                    h_out_name_rev = layer + "_h_out_rev"
                    c_out_name_rev = layer + "_c_out_rev"
                    self.optional_inputs.append((c_in_name, hidden_size))
                    self.optional_outputs.append((c_out_name, hidden_size))
                    self.optional_inputs.append((h_in_name_rev, hidden_size))
                    self.optional_inputs.append((c_in_name_rev, hidden_size))
                    self.optional_outputs.append((h_out_name_rev, hidden_size))
                    self.optional_outputs.append((c_out_name_rev, hidden_size))
                    _insert_to_dict(self.layers_optional_inputs, layer, c_in_name)
                    _insert_to_dict(self.layers_optional_outputs, layer, c_out_name)
                    _insert_to_dict(self.layers_optional_inputs, layer, h_in_name_rev)
                    _insert_to_dict(self.layers_optional_inputs, layer, c_in_name_rev)
                    _insert_to_dict(self.layers_optional_outputs, layer, h_out_name_rev)
                    _insert_to_dict(self.layers_optional_outputs, layer, c_out_name_rev)

    def _get_first_embedded_model(self):
        for idx, layer in enumerate(self.layer_list):
            keras_layer = self.keras_layer_map[layer]
            if isinstance(keras_layer, _keras.models.Sequential) or isinstance(
                keras_layer, _keras.models.Model
            ):
                return idx
        return -1

    def _get_first_shared_layer(self):
        for idx, layer in enumerate(self.layer_list):
            keras_layer = self.keras_layer_map[layer]
            inbound_nodes = (
                keras_layer.inbound_nodes
                if hasattr(keras_layer, "inbound_nodes")
                else keras_layer._inbound_nodes
            )
            inbound_nodes = [
                node for node in inbound_nodes if len(node.inbound_layers) > 0
            ]
            if (
                not _is_merge_layer(self.keras_layer_map[layer])
                and len(self.get_predecessors(layer)) > 1
                and len(inbound_nodes) > 1
            ):
                return idx
        return -1

    def _get_first_layer_of_type(self, layer_type):
        for idx, layer in enumerate(self.layer_list):
            keras_layer = self.keras_layer_map[layer]
            if isinstance(keras_layer, layer_type):
                return idx
        return -1

    def _add_edge(self, src, snk):
        if src not in self.edge_map:
            self.edge_map[src] = []
        if snk not in self.edge_map[src]:
            self.edge_map[src].append(snk)
        if snk not in self.reverse_edge_map:
            self.reverse_edge_map[snk] = []
        if src not in self.reverse_edge_map[snk]:
            self.reverse_edge_map[snk].append(src)

    def _remove_edge(self, src, snk):
        self.edge_map[src].remove(snk)
        if len(self.edge_map[src]) == 0:
            self.edge_map.pop(src)
        self.reverse_edge_map[snk].remove(src)
        if len(self.reverse_edge_map[snk]) == 0:
            self.reverse_edge_map.pop(snk)

    def _remove_layer(self, layer):
        """
        remove the layer and its input/output edges
        """
        successors = self.get_successors(layer)
        predecessors = self.get_predecessors(layer)
        # remove all edges
        for succ in successors:
            self._remove_edge(layer, succ)
        for pred in predecessors:
            self._remove_edge(pred, layer)
        # remove layer in the data structures
        self.keras_layer_map.pop(layer)
        self.layer_list.remove(layer)

    def _remove_layer_and_reconnect(self, layer):
        """ Remove the layer, and reconnect each of its predecessor to each of
        its successor
        """
        successors = self.get_successors(layer)
        predecessors = self.get_predecessors(layer)
        # remove layer's edges
        for succ in successors:
            self._remove_edge(layer, succ)
        for pred in predecessors:
            self._remove_edge(pred, layer)

        # connect predecessors and successors
        for pred in predecessors:
            for succ in successors:
                self._add_edge(pred, succ)

        # remove layer in the data structures
        self.layer_list.remove(layer)
        self.keras_layer_map.pop(layer)

        # re-assign input and output layers if layer happens to be an
        # input / output layer
        if layer in self.input_layers:
            idx = self.input_layers.index(layer)
            self.input_layers.pop(idx)
            for pred in predecessors:
                self.input_layers.insert(idx, pred)
                idx += 1
        if layer in self.output_layers:
            idx = self.output_layers.index(layer)
            self.output_layers.pop(idx)
            for succ in successors:
                self.output_layers.insert(idx, succ)
                idx += 1

    def _remove_old_edges(self, layer):
        predecessors = self.get_predecessors(layer)
        successors = self.get_successors(layer)
        for pred in predecessors:
            self._remove_edge(pred, layer)
        for succ in successors:
            self._remove_edge(layer, succ)

    def _remove_layers_of_type(self, layer_type):
        idx = self._get_first_layer_of_type(layer_type)
        while idx >= 0:
            layer = self.layer_list[idx]
            self._remove_layer_and_reconnect(layer)
            idx = self._get_first_layer_of_type(layer_type)

    def remove_skip_layers(self, skip_layers):
        for skip_layer in skip_layers:
            self._remove_layers_of_type(skip_layer)

    def remove_internal_input_layers(self):
        idx, nb_layers = 0, len(self.layer_list)
        while idx < nb_layers:
            layer = self.layer_list[idx]
            keras_layer = self.keras_layer_map[layer]
            if (
                isinstance(keras_layer, InputLayer)
                and len(self.get_predecessors(layer)) > 0
            ):
                # these are internal input layers that needs to be taken out
                self._remove_layer_and_reconnect(layer)
                idx -= 1
                nb_layers -= 1
            idx += 1

    def _insert_layer_after(self, layer_idx, new_layer, new_keras_layer):
        """ Insert the new_layer, whose parameter is stored in a Keras layer
        structure new_keras_layer, after the layer whose position is layer_idx.
        """
        layer = self.layer_list[layer_idx]
        self.layer_list.insert(layer_idx + 1, new_layer)
        self.keras_layer_map[new_layer] = new_keras_layer
        successors = self.get_successors(layer)
        # add edge layer -> new_layer
        self._add_edge(layer, new_layer)
        # add edges new_layer -> layer_successor, remove layer -> successor
        for succ in successors:
            self._add_edge(new_layer, succ)
            self._remove_edge(layer, succ)
        # if layer is an output layer, change the output layer tag
        if layer in self.output_layers:
            idx = self.output_layers.index(layer)
            self.output_layers[idx] = new_layer

    def _insert_layer_between(self, src, snk, new_layer, new_keras_layer):
        """ Insert the new_layer, whose keras layer parameters are stored in
        new_keras_layer, between src and snk.
        """
        if snk is None:
            insert_pos = self.layer_list.index(src) + 1
        else:
            insert_pos = self.layer_list.index(snk)  # insert position
        self.layer_list.insert(insert_pos, new_layer)
        self.keras_layer_map[new_layer] = new_keras_layer
        if src is None:  # snk is an input layer
            self._add_edge(new_layer, snk)
        elif snk is None:  # src is an output layer
            self._add_edge(src, new_layer)
        else:
            self._add_edge(src, new_layer)
            self._add_edge(new_layer, snk)
            self._remove_edge(src, snk)

        # if layer is an output layer, change the output layer tag
        if src in self.output_layers:
            idx = self.output_layers.index(src)
            self.output_layers[idx] = new_layer

    def defuse_activation(self):
        """ Defuse the fused activation layers in the network.
        """
        idx, nb_layers = 0, len(self.layer_list)
        while idx < nb_layers:
            layer = self.layer_list[idx]
            k_layer = self.keras_layer_map[layer]
            if isinstance(k_layer, _keras.layers.TimeDistributed):
                k_layer = k_layer.layer
            if (
                isinstance(k_layer, _keras.layers.Conv2D)
                or isinstance(k_layer, _keras.layers.Conv1D)
                or isinstance(k_layer, _keras.layers.SeparableConv2D)
                or isinstance(k_layer, _keras.layers.SeparableConv1D)
                or isinstance(k_layer, _keras.layers.Dense)
            ):

                func_name = (
                    k_layer.activation.func_name
                    if six.PY2
                    else k_layer.activation.__name__
                )

                if func_name != "linear":
                    # Create new layer
                    new_layer = layer + "__activation__"
                    new_keras_layer = _keras.layers.core.Activation(func_name)
                    # insert new layer after it
                    self._insert_layer_after(idx, new_layer, new_keras_layer)
                    idx += 1
                    nb_layers += 1
            idx += 1

    def is_activation(self, layer):
        keras_layer = self.keras_layer_map[layer]
        for activation_type in _KERAS_ACTIVATION_LAYERS:
            if isinstance(keras_layer, activation_type):
                return True
        return False

    def is_1d_layer(self, layer):
        keras_layer = self.keras_layer_map[layer]
        for layer_type in _KERAS_LAYERS_1D:
            if isinstance(keras_layer, layer_type):
                return True
        return False

    def _get_1d_interface_edges(self):
        """ Get edges that represents transition from not-1D to 1D, and 1D to
        not-1D. A 'in_edge e(u,v)' means u operates on non-1D blobs, but v
        operates on 1D blobs. An 'out_edge e(u,v)' means u operates on 1D
        blobs, but v operates on non-1D blobs.
        """
        in_edges = set()
        for layer in self.layer_list:
            if not self.is_1d_layer(layer):
                continue
            preds = self.get_predecessors(layer)
            if len(preds) == 0:
                in_edges.add((None, layer))
            else:
                # because 1D layers are all 1-input,
                # there should only be 1 predecessor
                u, v = preds[0], layer
                while u and (
                    self.is_activation(u) or type(u) in _KERAS_NORMALIZATION_LAYERS
                ):
                    preds = self.get_predecessors(u)
                    v = u
                    u = preds[0] if len(preds) > 0 else None
                if u is None or (not self.is_1d_layer(u)):
                    in_edges.add((u, v))

        out_edges = set()
        for layer in self.layer_list:
            if not self.is_1d_layer(layer):
                continue
            # cases for 1d->output
            if layer in self.output_layers:
                out_edges.add((layer, None))

            succs = self.get_successors(layer)
            if len(succs) > 0:
                # this should be handled in 1d->output already
                if not self.is_activation(succs[0]):
                    for succ in succs:
                        if not self.is_1d_layer(succ):
                            out_edges.add((layer, succ))
                else:
                    act_layer = succs[0]
                    succs = self.get_successors(act_layer)
                    if len(succs) == 0:
                        out_edges.add((act_layer, None))
                    else:
                        for succ in succs:
                            if not self.is_1d_layer(succ):
                                out_edges.add((act_layer, succ))

        return list(in_edges), list(out_edges)

    def insert_1d_permute_layers(self):
        """
        Insert permutation layers before a 1D start point or after 1D end point
        """
        idx, nb_layers = 0, len(self.layer_list)
        in_edges, out_edges = self._get_1d_interface_edges()

        # Hacky Warning: (1) use a 4-D permute, which is not likely to happen in Keras,
        # to represent actual permutation needed for (seq, c, h, w) in CoreML
        # (2) Assume 2-D input shape has meaning (seq, c), and during CoreML runtime,
        # it is represented as 4D blob, (seq, c, h, w)
        for in_edge in in_edges:
            src, snk = in_edge
            if src is None:
                permute_layer = "_permute_" + snk
            else:
                permute_layer = src + "_permute_" + snk
            keras_permute = _keras.layers.Permute(
                dims=(3, 1, 2, 0)
            )  # assume w = 1, switch seq and w
            self._insert_layer_between(src, snk, permute_layer, keras_permute)
        for out_edge in out_edges:
            src, snk = out_edge
            if snk is None:
                permute_layer = src + "_permute_"
            else:
                permute_layer = src + "_permute_" + snk
            keras_permute = _keras.layers.Permute(
                dims=(3, 1, 2, 0)
            )  # assume w = 1, switch seq and w back
            self._insert_layer_between(src, snk, permute_layer, keras_permute)

    def insert_permute_for_spatial_bn(self):

        # find spatial batchnorm layers
        spatial_bn_layers = []
        for layer in self.layer_list:
            keras_layer = self.keras_layer_map[layer]
            if (
                isinstance(keras_layer, _keras.layers.BatchNormalization)
                and len(keras_layer.input_shape) == 4
            ):
                if keras_layer.axis == 1 or keras_layer.axis == 2:
                    spatial_bn_layers.append(layer)

        for sbn in spatial_bn_layers:
            axis = self.keras_layer_map[sbn].axis
            # axis == 1: swap H axis; axis == 2 : swap W axis
            dims = (0, 2, 1, 3) if axis == 1 else (0, 3, 2, 1)
            # add permutation before spatial batchnorm
            pred = self.get_predecessors(sbn)[0]
            permute_layer = pred + "_permute_" + sbn
            keras_permute = _keras.layers.Permute(dims=dims)
            self._insert_layer_between(pred, sbn, permute_layer, keras_permute)
            # add permutation after spatial batchnorm
            succs = self.get_successors(sbn)
            if len(succs) == 0:
                permute_layer = sbn + "_permute_"
                keras_permute = _keras.layers.Permute(dims=dims)
                self._insert_layer_between(sbn, None, permute_layer, keras_permute)
            else:
                for succ in succs:
                    permute_layer = sbn + "_permute_" + succ
                    keras_permute = _keras.layers.Permute(dims=dims)
                    self._insert_layer_between(sbn, succ, permute_layer, keras_permute)

    def build(self, is_top_level=True):
        # sanity check.
        model = self.model
        if not (
            type(model) == _keras.models.Sequential
            or type(model) == _keras.models.Model
        ):
            raise TypeError("Keras layer of type %s is not supported." % type(model))

        # build the graph without considering embedded subgraphs
        for i, layer in enumerate(model.layers):
            in_nodes = (
                layer._inbound_nodes
                if hasattr(layer, "_inbound_nodes")
                else layer.inbound_nodes
            )
            for node in in_nodes:
                for pred in node.inbound_layers:
                    if pred.name not in self.layer_list:
                        self.layer_list.append(pred.name)
                        self.keras_layer_map[pred.name] = pred
                    self._add_edge(pred.name, layer.name)
            self.layer_list.append(layer.name)
            self.keras_layer_map[layer.name] = layer

        # Duplicate models for weight sharing
        idx = self._get_first_shared_layer()
        while idx >= 0:
            layer = self.layer_list[idx]
            keras_layer = self.keras_layer_map[layer]
            predecessors = self.reverse_edge_map[layer]
            successors = self.edge_map[layer]
            new_layers = [layer + "_" + str(i) for i in range(len(predecessors))]
            self.layer_list[idx : idx + 1] = new_layers
            for i, new_layer in enumerate(new_layers):
                self.edge_map[new_layer] = []
                self.reverse_edge_map[new_layer] = []
                self.keras_layer_map[new_layer] = keras_layer
                pred = predecessors[i]
                self._add_edge(pred, new_layer)
                for succ in successors:
                    self._add_edge(new_layer, succ)
            self._remove_old_edges(layer)
            self.keras_layer_map.pop(layer)
            idx = self._get_first_shared_layer()

        # Expand the sub-models
        idx = self._get_first_embedded_model()
        while idx >= 0:
            # grab the input and output edges of the embedded model
            embedded_model = self.layer_list[idx]
            # build the embedded model
            embedded_keras_model = self.keras_layer_map[embedded_model]
            embedded_graph = NetGraph(embedded_keras_model)
            embedded_graph.build(is_top_level=False)
            # replace the embedded model with the layers of the embedded graph
            embedded_layer_list = embedded_graph.layer_list
            new_layer_list = []
            for embedded_layer_name in embedded_layer_list:
                new_layer_name = embedded_model + "_" + embedded_layer_name
                new_layer_list.append(new_layer_name)
                self.keras_layer_map[new_layer_name] = embedded_graph.keras_layer_map[
                    embedded_layer_name
                ]
                # add edge [embed_layer -> its succ]
                embedded_successors = embedded_graph.get_successors(embedded_layer_name)
                for embed_succ_name in embedded_successors:
                    new_embed_succ_name = embedded_model + "_" + embed_succ_name
                    self._add_edge(new_layer_name, new_embed_succ_name)
                # add edge [pred -> embed_layer]
                embedded_predecessors = embedded_graph.get_predecessors(
                    embedded_layer_name
                )
                for embed_pred_name in embedded_predecessors:
                    new_embed_pred_name = embedded_model + "_" + embed_pred_name
                    self._add_edge(new_embed_pred_name, new_layer_name)

            self.layer_list[idx + 1 : idx + 1] = new_layer_list
            # replace input / output edges to the model with input/output edges of the embedded layers
            predecessors = self.get_predecessors(embedded_model)
            embedded_inputs = embedded_graph.get_input_layers()
            for i, pred in enumerate(predecessors):
                embed_input = embedded_inputs[i]
                new_embed_input = embedded_model + "_" + embed_input
                self._add_edge(pred, new_embed_input)

            embedded_outputs = embedded_graph.get_output_layers()
            successors = self.get_successors(embedded_model)
            for i, succ in enumerate(successors):
                embed_output = embedded_outputs[i]
                new_embed_output = embedded_model + "_" + embed_output

                self._add_edge(new_embed_output, succ)

            # clear up the embedded model
            self._remove_layer(embedded_model)
            idx = self._get_first_embedded_model()

        # tag input layers and and output layers
        self.make_input_layers()
        self.make_output_layers()

        # make graph level adjustments - do this only on top level
        if is_top_level:
            self.remove_skip_layers(_KERAS_SKIP_LAYERS)  # done 1 pass
            self.insert_1d_permute_layers()
            self.insert_permute_for_spatial_bn()
            self.defuse_activation()
            self.remove_internal_input_layers()

    def print_layer_list(self):
        print("\n")
        print("layer_list")
        print(self.layer_list)

    def print_edge_map(self):
        print("\n")
        print("edge map:")
        for src in self.edge_map:
            for snk in self.edge_map[src]:
                print("  ", src, "-->", snk)

    def print_reverse_edge_map(self):
        print("\n")
        print("reverse edge map: ")
        for snk in self.reverse_edge_map:
            for src in self.reverse_edge_map[snk]:
                print("  ", snk, "<--", src)

    def print_mapping(self):
        print("\nmapping:")
        for key in self.keras_layer_map:
            print(
                key,
                "-->",
                self.keras_layer_map[key],
                "(",
                self.keras_layer_map[key].name,
                ")",
            )

    def print_all(self):
        print("=" * 80)
        self.print_layer_list()
        self.print_edge_map()
        self.print_reverse_edge_map()
        self.print_mapping()
