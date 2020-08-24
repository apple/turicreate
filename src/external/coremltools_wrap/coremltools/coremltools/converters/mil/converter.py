#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters._profile_utils import _profile
from .mil.passes.common_pass import common_pass


class ConverterRegistry:
    frontends = {}
    backends = {}

    @staticmethod
    def frontend(converter):
        ConverterRegistry.frontends[converter.name] = converter
        return converter

    @staticmethod
    def backend(converter):
        ConverterRegistry.backends[converter.name] = converter
        return converter


@ConverterRegistry.frontend
class MILFrontend:
    name = "mil"

    def __call__(self, model, *args, **kwargs):
        return model


@ConverterRegistry.frontend
class TensorFlowFrontend:
    name = "tensorflow"

    def __call__(self, *args, **kwargs):
        from .frontend.tensorflow.load import TF1Loader

        tf1_loader = TF1Loader(*args, **kwargs)
        return tf1_loader.load()


@ConverterRegistry.frontend
class TensorFlow2Frontend:
    name = "tensorflow2"

    def __call__(self, *args, **kwargs):
        from .frontend.tensorflow2.load import TF2Loader

        tf2_loader = TF2Loader(*args, **kwargs)
        return tf2_loader.load()


@ConverterRegistry.frontend
class TorchFrontend:
    name = "torch"

    def __call__(self, *args, **kwargs):
        from .frontend.torch import load

        return load(*args, **kwargs)


@ConverterRegistry.backend
class NNProtoBackend:
    name = "nn_proto"

    def __call__(self, *args, **kwargs):
        from .backend.nn import load

        return load(*args, **kwargs)


@ConverterRegistry.frontend
class CustomFrontend:
    name = "custom"

    def __call__(self, *args, **kwargs):
        from coremltools.converters.mil.mil.passes.common_pass import common_pass

        return common_pass(*args, **kwargs)


@_profile
def _convert(
    model,
    convert_from="TensorFlow",
    convert_to="nn_proto",
    converter_registry=ConverterRegistry,
    **kwargs
):
    """
    Convert from an external representation.

    Args:
        model (Any): The model to convert.
        convert_from (str): The name of the input converter.
        convert_to (str): The name of the output converter.
        converter_registry: Converter registries.
    Returns:
        The converted model.
    """
    frontend_converter_type = converter_registry.frontends.get(convert_from.lower())
    if not frontend_converter_type:
        msg = 'Frontend converter "{}" not implemented, must be one of: {}'
        raise NotImplementedError(
            msg.format(convert_from, list(converter_registry.frontends.keys()))
        )
    frontend_converter = frontend_converter_type()

    backend_converter_type = converter_registry.backends.get(convert_to.lower())
    if not backend_converter_type:
        msg = 'Backend converter "{}" not implemented, must be one of: {}'
        raise NotImplementedError(
            msg.format(convert_to, list(converter_registry.backends.keys()))
        )
    backend_converter = backend_converter_type()

    prog = frontend_converter(model, **kwargs)
    common_pass(prog)
    out = backend_converter(prog, **kwargs)

    return out
