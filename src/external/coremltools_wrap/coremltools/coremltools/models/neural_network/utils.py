from .builder import NeuralNetworkBuilder
from coremltools.models.utils import _get_model
import copy as _copy


def make_image_input(
    model,
    input_name,
    is_bgr=False,
    red_bias=0.0,
    blue_bias=0.0,
    green_bias=0.0,
    gray_bias=0.0,
    scale=1.0,
    image_format="NHWC",
):
    """
    Convert input of type multiarray to type image

    Parameters
    ----------
    TODO

    Returns
    -------
    model: MLModel
    A coreML MLModel object

    Examples
    --------
    TODO
    """

    spec = model.get_spec()

    if spec.WhichOneof("Type") not in [
        "neuralNetwork",
        "neuralNetworkClassifier",
        "neuralNetworkRegressor",
    ]:
        raise ValueError(
            "Provided model must be of type neuralNetwork, neuralNetworkClassifier or neuralNetworkRegressor"
        )

    if not isinstance(input_name, list):
        input_name = [input_name]

    spec_inputs = [i.name for i in spec.description.input]
    for name in input_name:
        if name not in spec_inputs:
            msg = "Provided input_name: {}, is not an existing input to the model"
            raise ValueError(msg.format(name))

    builder = NeuralNetworkBuilder(spec=spec)
    builder.set_pre_processing_parameters(
        image_input_names=input_name,
        is_bgr=is_bgr,
        red_bias=red_bias,
        green_bias=green_bias,
        blue_bias=blue_bias,
        gray_bias=gray_bias,
        image_scale=scale,
        image_format=image_format,
    )
    return _get_model(spec)


def make_nn_classifier(
    model,
    class_labels,
    predicted_feature_name=None,
    predicted_probabilities_output=None,
):
    """
    Convert a model of type "neuralNetwork" to type "neuralNetworkClassifier"

    Parameters
    ----------
    TODO

    Returns
    -------
    model: MLModel
    A coreML MLModel object

    Examples
    --------
    TODO
    """

    spec = model.get_spec()

    if spec.WhichOneof("Type") != "neuralNetwork":
        raise ValueError('Provided model must be of type "neuralNetwork"')

    # convert type to "neuralNetworkClassifier" and copy messages from "neuralNetwork"
    nn_spec = _copy.deepcopy(spec.neuralNetwork)
    spec.ClearField("neuralNetwork")
    for layer in nn_spec.layers:
        spec.neuralNetworkClassifier.layers.add().CopyFrom(layer)
    for preprocessing in nn_spec.preprocessing:
        spec.neuralNetworkClassifier.preprocessing.add().CopyFrom(preprocessing)
    spec.neuralNetworkClassifier.arrayInputShapeMapping = nn_spec.arrayInputShapeMapping
    spec.neuralNetworkClassifier.imageInputShapeMapping = nn_spec.imageInputShapeMapping
    spec.neuralNetworkClassifier.updateParams.CopyFrom(nn_spec.updateParams)

    # set properties related to classifier
    builder = NeuralNetworkBuilder(spec=spec)
    message = "Class labels must be a list of integers / strings or a file path"
    classes_in = class_labels
    if isinstance(classes_in, str):
        import os

        if not os.path.isfile(classes_in):
            raise ValueError("Path to class labels (%s) does not exist." % classes_in)
        with open(classes_in, "r") as f:
            classes = f.read()
        classes = classes.splitlines()
    elif isinstance(classes_in, list):  # list[int or str]
        classes = classes_in
        assert all([isinstance(x, (int, str)) for x in classes]), message
    else:
        raise ValueError(message)

    kwargs = {}
    if predicted_feature_name is not None:
        kwargs["predicted_feature_name"] = predicted_feature_name
    if predicted_probabilities_output is not None:
        kwargs["prediction_blob"] = predicted_probabilities_output
    builder.set_class_labels(classes, **kwargs)

    return _get_model(spec)
