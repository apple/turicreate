#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _

import logging as _logging
import os.path as _os_path

import torch as _torch

from six import string_types as _string_types
from .converter import TorchConverter, torch_to_mil_types
from coremltools.converters.mil.input_types import InputType, TensorType
from coremltools.converters.mil.mil import Program, types


def load(model_spec, debug=False, **kwargs):
    """
    Convert PyTorch .pt file to mil CoreML format.

    Parameters
    ----------
    model_spec: String path to .pt file, or a TorchScript object representing
        the model to convert.
    debug: bool, optional. Defaults to False.
        This flag should generally be False except for debugging purposes
        for diagnosing conversion errors. Setting this flag to True will
        print the list of supported and unsupported ops found in the model
        if conversion fails due to an unsupported op.
    inputs: Can be a singular element or list of elements of the following form
        1. Any subclass of InputType
        2. torch.Tensor (only shape and dtype will be used)
        3. list of (1. or 2.)
        Inputs are parsed in the flattened order that the model accepts them.
        If names are not specified: input keys for calling predict on the converted model
        will be internal symbols of the input to the graph.
        User can specify a subset of names.
    outputs (optional): List of output name strings. If specified: keys of output dictionary
        will be these names in order of flattened returned outputs. If not specified:
        output dictionary keys will be the internal output symbols in the graph.
        User can specify a subset of names.
    cut_at_symbols (optional): List of internal symbol name strings. Graph conversion will
        terminate once these symbols have been generated. For debugging use
        only.
    """

    torchscript = _torchscript_from_model(model_spec)

    def _convert_to_inputtype(inputs):
        input_type = []
        for _input in inputs:
            if isinstance(_input, (list, tuple)):
                input_type.append(_convert_to_inputtype(_input))
            elif isinstance(_input, InputType):
                input_type.append(_input)
            elif isinstance(_input, _torch.Tensor):
                input_type.append(
                    TensorType(
                        shape=_input.shape, dtype=torch_to_mil_types[_input.dtype]
                    )
                )
            else:
                raise ValueError(
                    "Unknown type {} for conversion to InputType.".format(type(_input))
                )
        return input_type

    inputs = _convert_to_inputtype(kwargs["inputs"])
    outputs = kwargs.get("outputs", None)
    cut_at_symbols = kwargs.get("cut_at_symbols", None)
    converter = TorchConverter(torchscript, inputs, outputs, cut_at_symbols)

    try:
        prog = converter.convert()
    except RuntimeError as e:
        if debug and "convert function" in str(e):
            implemented, missing = converter.check_ops()
            print("the following model ops are IMPLEMENTED:")
            print("\n".join(["  " + str(x) for x in sorted(implemented)]))
            print("the following model ops are MISSING:")
            print("\n".join(["  " + str(x) for x in sorted(missing)]))
        raise e
    except Exception as e:
        raise e

    return prog


def _torchscript_from_model(model_spec):
    if isinstance(model_spec, _string_types) and model_spec.endswith(".pt"):
        filename = _os_path.abspath(model_spec)
        return _torch.jit.load(filename)
    elif isinstance(model_spec, _torch.jit.ScriptModule):
        return model_spec
    else:
        raise TypeError(
            "@model must either be a PyTorch .pt file or a TorchScript object, received: {}".format(
                type(model_spec)
            )
        )
