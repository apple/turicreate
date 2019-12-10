# Copyright (c) 2018, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from .spec_inspection_utils import *


def print_network_spec_parameter_info_style(mlmodel_spec, interface_only=False):
    """ Print the network information summary.
    Args:
    mlmodel_spec : the mlmodel spec
    interface_only : Shows only the input and output of the network
    """
    inputs, outputs, layers_info = summarize_neural_network_spec(mlmodel_spec)

    print('Inputs:')
    for i in inputs:
        name, description = i
        print('  {} {}'.format(name, description))

    print('Outputs:')
    for o in outputs:
        name, description = o
        print('  {} {}'.format(name, description))

    if layers_info is None:
        print('\n(This MLModel is not a neural network model or does not contain any layers)')

    if layers_info and not interface_only:
        print('\nLayers:')
        for idx, l in enumerate(layers_info):
            layer_type, name, in_blobs, out_blobs, params_info = l
            print('[{}] ({}) {}'.format(idx, layer_type, name))
            print('  Input blobs: {}'.format(in_blobs))
            print('  Output blobs: {}'.format(out_blobs))
            if len(params_info) > 0:
                print('  Parameters: ')
            for param in params_info:
                print('    {} = {}'.format(param[0], param[1]))

    print('\n')


def print_network_spec_coding_style(mlmodel_spec, interface_only=False):
    """
    Args:
    mlmodel_spec : the mlmodel spec
    interface_only : Shows only the input and output of the network
    """

    inputs = [(blob.name, get_feature_description_summary(blob)) for blob in mlmodel_spec.description.input]
    outputs = [(blob.name, get_feature_description_summary(blob)) for blob in mlmodel_spec.description.output]

    input_names = []
    print('Inputs:')
    for i in inputs:
        name, description = i
        print('  {} {}'.format(name, description))
        input_names.append(name)

    output_names = []
    print('Outputs:')
    for o in outputs:
        name, description = o
        print('  {} {}'.format(name, description))
        output_names.append(name)

    if interface_only:
        return

    nn_spec = None

    if mlmodel_spec.HasField('neuralNetwork'):
        nn_spec = mlmodel_spec.neuralNetwork
    elif mlmodel_spec.HasField('neuralNetworkClassifier'):
        nn_spec = mlmodel_spec.neuralNetworkClassifier
    elif mlmodel_spec.HasField('neuralNetworkRegressor'):
        nn_spec = mlmodel_spec.neuralNetworkRegressor

    if nn_spec is None:
        print('\n(This MLModel is not a neural network model)')
        return

    print('\n')
    summarize_neural_network_spec_code_style(nn_spec, input_names=input_names, output_names=output_names)


def print_network_spec(mlmodel_spec, interface_only=False, style=''):
    """ Print the network information summary.
    Args:
    mlmodel_spec : the mlmodel spec
    interface_only : Shows only the input and output of the network
    style : str. Either 'coding' or default, which prints information on parameters of layers.
    """

    if style == 'coding':
        print_network_spec_coding_style(mlmodel_spec, interface_only=interface_only)
    else:
        print_network_spec_parameter_info_style(mlmodel_spec, interface_only=interface_only)
