# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _


def _get_nn_spec(spec):
    if spec.WhichOneof('Type') == 'neuralNetwork':
        nn_spec = spec.neuralNetwork
    elif spec.WhichOneof('Type') == 'neuralNetworkClassifier':
        nn_spec = spec.neuralNetworkClassifier
    elif spec.WhichOneof('Type') == 'neuralNetworkRegressor':
        nn_spec = spec.neuralNetworkRegressor
    else:
        raise ValueError('Specification must contain a neural network')
    return nn_spec


def _find_disconnected_load_constants(nn_spec, disconnected_load_constants):
    nn_layers = nn_spec.layers
    for layer in nn_layers:
        layer_type = layer.WhichOneof('layer')
        if layer_type == 'loadConstant' or layer_type == 'loadConstantND':
            disconnected_load_constants[layer.output[0]] = layer

        for inp in layer.input:
            if inp in disconnected_load_constants:
                disconnected_load_constants.pop(inp)

        if layer_type == 'loop':
            _find_disconnected_load_constants(
                layer.loop.conditionNetwork, disconnected_load_constants)
            _find_disconnected_load_constants(layer.loop.bodyNetwork, disconnected_load_constants)
            if layer.loop.conditionVar in disconnected_load_constants:
                disconnected_load_constants.pop(layer.loop.conditionVar)

        if layer_type == 'branch':
            _find_disconnected_load_constants(layer.branch.ifBranch, disconnected_load_constants)
            _find_disconnected_load_constants(layer.branch.elseBranch, disconnected_load_constants)


def _delete_disconnected_load_constants(nn_spec, disconnected_load_constants):
    nn_layers = nn_spec.layers
    N = len(nn_layers)
    for i in range(N-1, -1, -1):
        layer = nn_layers[i]
        layer_type = layer.WhichOneof('layer')
        if layer_type == 'loadConstant' or layer_type == 'loadConstantND':
            if layer.output[0] in disconnected_load_constants:
                nn_layers.remove(layer)

        if layer_type == 'loop':
            _delete_disconnected_load_constants(layer.loop.conditionNetwork, disconnected_load_constants)
            _delete_disconnected_load_constants(layer.loop.bodyNetwork, disconnected_load_constants)

        if layer_type == 'branch':
            _delete_disconnected_load_constants(layer.branch.ifBranch, disconnected_load_constants)
            _delete_disconnected_load_constants(layer.branch.elseBranch, disconnected_load_constants)


def remove_disconnected_constants(spec):
    """
    remove constant layers whose outputs are not connected to any other layer
    """
    nn_spec = _get_nn_spec(spec)
    disconnected_load_constants = dict()  # output_name -> layer reference
    _find_disconnected_load_constants(nn_spec, disconnected_load_constants)
    if len(disconnected_load_constants) > 0:
        _delete_disconnected_load_constants(nn_spec, disconnected_load_constants)
        print('[Core ML Pass] {} disconnected constants nodes deleted'.format(
            len(disconnected_load_constants)))
