#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import numpy as np
import torch
import torch.nn as nn
from six import string_types as _string_types
from coremltools import TensorType
from coremltools.converters.mil.testing_reqs import _converter
from coremltools.models import MLModel
from coremltools._deps import _IS_MACOS

class ModuleWrapper(nn.Module):
    """
    Helper class to transform torch function into torch nn module.
    This helps to keep the testing interface same for torch functional api.

    """
    def __init__(self, function, kwargs=None):
        super(ModuleWrapper, self).__init__()
        self.function = function
        self.kwargs = kwargs if kwargs else {}

    def forward(self, x):
        return self.function(x, **self.kwargs)

np.random.seed(1984)


def _flatten(object):
    flattened_list = []
    for item in object:
        if isinstance(item, (list, tuple)):
            flattened_list.extend(_flatten(item))
        else:
            flattened_list.append(item)
    return flattened_list


def convert_to_coreml_inputs(input_description, inputs):
    """Convenience function to combine a CoreML model's input description and
    set of raw inputs into the format expected by the model's predict function.
    """
    flattened_inputs = _flatten(inputs)
    coreml_inputs = {
        str(x): inp.numpy() for x, inp in zip(input_description, flattened_inputs)
    }
    return coreml_inputs


def convert_to_mlmodel(model_spec, tensor_inputs, backend="nn_proto"):
    def _convert_to_inputtype(inputs):
        if isinstance(inputs, list):
            return [_convert_to_inputtype(x) for x in inputs]
        elif isinstance(inputs, tuple):
            return tuple([_convert_to_inputtype(x) for x in inputs])
        elif isinstance(inputs, torch.Tensor):
            return TensorType(shape=inputs.shape)
        else:
            raise ValueError(
                "Unable to parse type {} into InputType.".format(type(inputs))
            )

    inputs = list(_convert_to_inputtype(tensor_inputs))
    proto = _converter._convert(model_spec, inputs=inputs, convert_to=backend, convert_from="torch")
    return MLModel(proto, useCPUOnly=True)


def generate_input_data(input_size):
    if isinstance(input_size, list):
        return [torch.rand(_size) for _size in input_size]
    else:
        return torch.rand(input_size)


def trace_model(model, input_data):
    model.eval()
    if isinstance(input_data, list):
        input_data = tuple(input_data)
    torch_model = torch.jit.trace(model, input_data)
    return torch_model


def run_compare_torch(
    input_data, model, expected_results=None, places=5, input_as_shape=True, backend="nn_proto"
):
    """
        Traces a model and runs a numerical test.
        Args:
            input_as_shape <bool>: If true generates random input data with shape.
            expected_results <iterable, optional>: Expected result from running pytorch model.
    """
    model.eval()
    if input_as_shape:
        input_data = generate_input_data(input_data)
    model_spec = trace_model(model, input_data)
    convert_and_compare(
        input_data, model_spec, expected_results=expected_results, atol=10.0 ** -places, backend=backend
    )


def flatten_and_detach_torch_results(torch_results):
    if isinstance(torch_results, (list, tuple)):
        return [x.detach().numpy() for x in _flatten(torch_results)]
    # Do not need to flatten
    return [torch_results.detach().numpy()]


def convert_and_compare(input_data, model_spec, expected_results=None, atol=1e-5, backend="nn_proto"):
    """
        If expected results is not set, it will by default
        be set to the flattened output of the torch model.
    """
    if isinstance(model_spec, _string_types):
        torch_model = torch.jit.load(model_spec)
    else:
        torch_model = model_spec

    if not isinstance(input_data, (list, tuple)):
        input_data = [input_data]

    if not expected_results:
        expected_results = torch_model(*input_data)
    expected_results = flatten_and_detach_torch_results(expected_results)
    mlmodel = convert_to_mlmodel(model_spec, input_data, backend=backend)
    coreml_inputs = convert_to_coreml_inputs(mlmodel.input_description, input_data)
    if _IS_MACOS:
        coreml_results = mlmodel.predict(coreml_inputs)
        sorted_coreml_results = [
            coreml_results[key] for key in sorted(coreml_results.keys())
        ]

        for torch_result, coreml_result in zip(expected_results, sorted_coreml_results):
            np.testing.assert_equal(coreml_result.shape, torch_result.shape)
            np.testing.assert_allclose(coreml_result, torch_result, atol=atol)
