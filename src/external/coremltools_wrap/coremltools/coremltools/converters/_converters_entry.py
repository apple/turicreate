from __future__ import absolute_import as _

import gc
import coremltools
from six import string_types as _string_types
import collections

from coremltools.converters.mil.input_types import InputType, ClassifierConfig
from coremltools.converters.mil.converter import _convert
from coremltools.converters.mil.mil import Program
from coremltools._deps import _HAS_TORCH, _HAS_TF_1, _HAS_TF_2
from coremltools.converters._profile_utils import _profile
from coremltools import __version__ as ct_version
from coremltools.models import _METADATA_VERSION, _METADATA_SOURCE
from coremltools.converters.mil._deployment_compatibility import (
    AvailableTarget,
    check_deployment_compatibility,
)

if _HAS_TF_1:
    import tensorflow as tf
    from coremltools.converters.mil.frontend.tensorflow.load import TF1Loader
if _HAS_TF_2:
    import tensorflow as tf
    from coremltools.converters.mil.frontend.tensorflow2.load import TF2Loader

if _HAS_TORCH:
    import torch
    from coremltools.converters.mil.frontend.torch.load import (
        _torchscript_from_model as pytorch_load,
    )


@_profile
def convert(
    model,
    source="auto",
    inputs=None,
    outputs=None,
    classifier_config=None,
    minimum_deployment_target=None,
    **kwargs
):
    """
    Convert TensorFlow or Pytorch models to Core ML model format. Whether a
    parameter is required may differ between frameworks (see below). Note that
    this function is aliased as `ct.convert` in the tutorials.

    Parameters
    ----------
    model:
        TensorFlow 1, TensorFlow 2 or Pytorch model in one of the following
        format:

        For TensorFlow versions 1.x:
            - Frozen `tf.Graph <https://www.tensorflow.org/api_docs/python/tf/Graph>`_
            - Frozen graph (`.pb`) file path
            - `tf.keras.Model <https://www.tensorflow.org/api_docs/python/tf/keras>`_
            -  `HDF5 <https://keras.io/api/models/model_saving_apis/>`_ file path (`.h5`)
            - `SavedModel <https://www.tensorflow.org/guide/saved_model>`_ directory path
        For TensorFlow versions 2.x:
            - `tf.keras.Model <https://www.tensorflow.org/api_docs/python/tf/keras>`_
            - `HDF5 file path <https://keras.io/api/models/model_saving_apis/>`_ (`.h5`)
            - `SavedModel <https://www.tensorflow.org/guide/saved_model>`_ directory path
            - A `concrete function <https://www.tensorflow.org/guide/concrete_function>`_
        For Pytorch:
            - A `TorchScript <https://pytorch.org/docs/stable/jit.html>`_ object
            - Path to a `.pt` file

    source: str (optional)
        One of `auto`, `tensorflow`, or `pytorch`. `auto` determines the
        framework automatically for most cases. Raise ValueError if it fails
        to determine the source framework.

    inputs: list of `TensorType` or `ImageType`
        - Inputs are required for PyTorch model, but optional for TensorFlow.
        - For PyTorch models, the inputs may be nested list or tuple, but for
          TensorFlow models it must be a flat list.
        - For TensorFlow, if inputs is `None`, the inputs are `Placeholder`
          nodes in the model (if model is frozen graph) or function inputs (if
          model is tf function).
        - For TensorFlow, if inputs is not `None`, inputs may contain only a
          subset of all Placeholder in the TF model.

    outputs: list[str] (optional)

        TensorFlow 1 and 2:
            - `outputs` are optional.

            - If specified, `outputs` is a list of string representing node
              names.

            - If `outputs` are not specified, converter infers outputs as all
              terminal identity nodes.

        PyTorch:
            - `outputs` must not be specified.

    classifier_config: ClassifierConfig class (optional)
        The configuration if the mlmodel is intended to be a classifier.

    minimum_deployment_target: coremltools.target enumeration (optional)
        - one of the members of enum "coremltools.target."
        - When not-specified or None, converter aims for as minimum of a deployment target as possible

    Returns
    -------
    model: MLModel
        A Core ML MLModel object

    Examples
    --------
    TensorFlow 1, 2 (`model` is a frozen graph):

        >>> with tf.Graph().as_default() as graph:
        >>>     x = tf.placeholder(tf.float32, shape=(1, 2, 3), name="input")
        >>>     y = tf.nn.relu(x, name="output")

        # Automatically infer inputs and outputs
        >>> mlmodel = ct.convert(graph)
	    >>> test_input = np.random.rand(1, 2, 3) - 0.5
        >>> results = mlmodel.predict({"input": test_input})
        >>> print(results['output'])

    TensorFlow 2 (`model` is tf.Keras model path):

        >>> x = tf.keras.Input(shape=(32,), name='input')
        >>> y = tf.keras.layers.Dense(16, activation='softmax')(x)
        >>> keras_model = tf.keras.Model(x, y)

        >>> keras_model.save(h5_path)
        >>> mlmodel = ct.convert(h5_path)

        >>> test_input = np.random.rand(2, 32)
        >>> results = mlmodel.predict({'input': test_input})
        >>> print(results['Identity'])

    Pytorch:

        >>> model = torchvision.models.mobilenet_v2()
        >>> model.eval()
        >>> example_input = torch.rand(1, 3, 256, 256)
        >>> traced_model = torch.jit.trace(model, example_input)

        >>> input = ct.TensorType(name='input_name', shape=(1, 3, 256, 256))
        >>> mlmodel = ct.convert(traced_model, inputs=[input])
        >>> results = mlmodel.predict({"input": example_input.numpy()})
        >>> print(results['1651']) # 1651 is the node name given by PyTorch's JIT

    See `here <https://coremltools.readme.io/docs/neural-network-conversion>`_ for
    more advanced options
    """
    if minimum_deployment_target is not None and not isinstance(
        minimum_deployment_target, AvailableTarget
    ):
        msg = (
            "Unrecognized value of argument 'minimum_deployment_target': {}. "
            "It needs to be a member of 'coremltools.target' enumeration. "
            "For example, coremltools.target.iOS13"
        )
        raise TypeError(msg.format(minimum_deployment_target))

    source = source.lower()
    if source not in {"auto", "tensorflow", "pytorch"}:
        msg = (
            'Unrecognized value of argument "source": {}. '
            'It must be one of ["auto", "tensorflow", "pytorch"].'
        )
        raise ValueError(msg.format(source))

    def raise_if_duplicated(input_list):
        # Detect duplicated inputs
        input_names = [t.name for t in input_list if t.name is not None]
        dups = [
            item
            for item, count in collections.Counter(input_names).items()
            if count > 1
        ]
        if len(dups) > 0:
            raise ValueError("Duplicated inputs: {}".format(dups))

    if inputs is not None:
        if not isinstance(inputs, list):
            msg = '"inputs" must be of type list'
            raise ValueError(msg)

    if classifier_config is not None:
        if not isinstance(classifier_config, ClassifierConfig):
            msg = '"classifier_config" must be of type ClassifierConfig'
            raise ValueError(msg)

    if source == "tensorflow" and _HAS_TF_2:
        source = "tensorflow2"

    if source == "auto" and _HAS_TF_1:
        try:
            loader = TF1Loader(model, outputs=outputs)
            loader._graph_def_from_model(outputs=outputs)
            source = "tensorflow"
        except:
            pass

    if source == "auto" and _HAS_TF_2:
        try:
            loader = TF2Loader(model, outputs=outputs)
            loader._graph_def_from_model(outputs=outputs)
            source = "tensorflow2"
        except:
            pass

    if source == "auto" and _HAS_TORCH:
        try:
            pytorch_load(model)
            source = "pytorch"
        except:
            pass

    if source == "auto" and isinstance(model, Program):
        source = "mil"

    convert_to = kwargs.get("convert_to", "nn_proto")
    kwargs.pop("convert_to", None)

    if source == "auto":
        msg = (
            "Unable to determine the type of the model, i.e. the source framework. "
            'Please provide the value of argument "source", from one of '
            '["tensorflow", "pytorch"]. Note that model conversion requires the '
            "source package that generates the model. Please make sure you have "
            "the appropriate version of source package installed. E.g., if you're "
            "converting model originally trained with TensorFlow 1.14, make sure "
            "you have `tensorflow==1.14` installed."
        )
        raise ValueError(msg)

    elif source in {"tensorflow", "tensorflow2"}:

        if source == "tensorflow" and not _HAS_TF_1:
            raise ValueError(
                'Converter was called with source="tensorflow", but missing tensorflow package'
            )

        if inputs is not None:
            raise_if_duplicated(inputs)

        if inputs is not None and not all(
            [isinstance(_input, InputType) for _input in inputs]
        ):
            raise ValueError("Input should be a list of TensorType or ImageType")

        proto_spec = _convert(
            model,
            convert_from=source,
            convert_to=convert_to,
            inputs=inputs,
            outputs=outputs,
            classifier_config=classifier_config,
            **kwargs
        )

    elif source == "pytorch":
        if "example_inputs" in kwargs:
            msg = 'Unexpected argument "example_inputs" found'
            raise ValueError(msg)

        def _flatten_list(_inputs):
            ret = []
            for _input in _inputs:
                if isinstance(_input, (list, tuple)):
                    ret.extend(_flatten_list(_input))
                elif isinstance(_input, InputType):
                    ret.append(_input)
                else:
                    raise ValueError(
                        "Unknown type {} for flattening into InputType.".format(
                            type(_input)
                        )
                    )
            return ret

        flat_inputs = _flatten_list(inputs)
        raise_if_duplicated(flat_inputs)
        if inputs is not None and not all(
            [isinstance(_input, InputType) for _input in flat_inputs]
        ):
            raise ValueError(
                "Input should be a list/tuple (or nested lists/tuples) of TensorType or ImageType"
            )
        if outputs is not None:
            raise ValueError("outputs must not be specified for PyTorch")

        proto_spec = _convert(
            model,
            convert_from="torch",
            convert_to=convert_to,
            inputs=inputs,
            outputs=outputs,
            classifier_config=classifier_config,
            **kwargs
        )

    elif source == "mil":
        if not isinstance(model, Program):
            msg = "Converter was asked to convert MIL input, but input is not a MIL program!"
            raise ValueError(msg)

        proto_spec = _convert(
            model,
            convert_from="mil",
            convert_to=convert_to,
            example_inputs=inputs,
            classifier_config=classifier_config,
            **kwargs
        )

    model = coremltools.models.MLModel(proto_spec, useCPUOnly=True)

    if minimum_deployment_target is not None:
        check_deployment_compatibility(
            spec=proto_spec,
            representation=convert_to,
            deployment_target=minimum_deployment_target,
        )

    del proto_spec
    gc.collect()

    # recording metadata: coremltools version, source framework and version
    if source in {"tensorflow", "tensorflow2"} and (_HAS_TF_1 or _HAS_TF_2):
        src_pkg_version = "tensorflow=={0}".format(tf.__version__)
    elif source == "pytorch" and _HAS_TORCH:
        src_pkg_version = "torch=={0}".format(torch.__version__)
    else:
        src_pkg_version = "unknown"

    model.user_defined_metadata[_METADATA_VERSION] = ct_version
    model.user_defined_metadata[_METADATA_SOURCE] = src_pkg_version

    return model
