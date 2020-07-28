from enum import Enum
from coremltools import _SPECIFICATION_VERSION_IOS_13, _SPECIFICATION_VERSION_IOS_14


class AvailableTarget(Enum):
    # iOS versions
    iOS13 = _SPECIFICATION_VERSION_IOS_13
    iOS14 = _SPECIFICATION_VERSION_IOS_14

    # macOS versions (aliases of iOS versions)
    macOS15 = _SPECIFICATION_VERSION_IOS_13
    macOS16 = _SPECIFICATION_VERSION_IOS_14

    # watchOS versions (aliases of iOS versions)
    watchOS6 = _SPECIFICATION_VERSION_IOS_13
    watchOS7 = _SPECIFICATION_VERSION_IOS_14

    # tvOS versions (aliases of iOS versions)
    tvOS13 = _SPECIFICATION_VERSION_IOS_13
    tvOS14 = _SPECIFICATION_VERSION_IOS_14


_get_features_associated_with = {}


def register_with(name):
    def decorator(func):
        if name not in _get_features_associated_with:
            _get_features_associated_with[name] = func
        else:
            raise ValueError("Function is already registered with {}".format(name))
        return func

    return decorator


@register_with(AvailableTarget.iOS14)
def iOS14Features(spec):
    features_list = []

    if spec.WhichOneof("Type") == "neuralNetwork":
        nn_spec = spec.neuralNetwork
    elif spec.WhichOneof("Type") in "neuralNetworkClassifier":
        nn_spec = spec.neuralNetworkClassifier
    elif spec.WhichOneof("Type") in "neuralNetworkRegressor":
        nn_spec = spec.neuralNetworkRegressor
    else:
        raise ValueError("Invalid neural network specification for the model")

    # Non-zero default optional values
    for idx, input in enumerate(spec.description.input):
        value = 0
        if input.type.isOptional:
            value = max(value, input.type.multiArrayType.floatDefaultValue)
            value = max(value, input.type.multiArrayType.doubleDefaultValue)
            value = max(value, input.type.multiArrayType.intDefaultValue)

        if value != 0:
            msg = "Support of non-zero default optional values for inputs."
            features_list.append(msg)
            break

    # Layers or modifications introduced in iOS14
    new_layers = [
        "oneHot",
        "cumSum",
        "clampedReLU",
        "argSort",
        "pooling3d",
        "convolution3d",
        "globalPooling3d",
    ]
    for layer in nn_spec.layers:
        layer_type = layer.WhichOneof("layer")

        msg = ""

        if layer_type in new_layers:
            msg = "{} {}".format(layer_type.capitalize(), "operation")

        if layer_type == "tile" and len(layer.input) == 2:
            msg = "Dynamic Tile operation"

        if layer_type == "upsample" and layer.upsample.linearUpsampleMode in [1, 2]:
            msg = "Upsample operation with Align Corners mode"

        if layer_type == "reorganizeData" and layer.reorganizeData.mode == 2:
            msg = "Pixel Shuffle operation"

        if layer_type == "sliceDynamic" and layer.sliceDynamic.squeezeMasks:
            msg = "Squeeze mask for dynamic slice operation"

        if layer_type == "sliceStatic" and layer.sliceDynamic.squeezeMasks:
            msg = "Squeeze mask for static slice operation"

        if msg != "" and (msg not in features_list):
            features_list.append(msg)

    return features_list


def check_deployment_compatibility(spec, representation=None, deployment_target=None):
    if representation is None:
        representation = "nn_proto"

    if deployment_target is None:
        deployment_target = AvailableTarget.iOS13

    if representation != "nn_proto":
        raise ValueError(
            "Deployment is supported only for mlmodel in nn_proto representation. Provided: {}".format(
                representation
            )
        )

    if not isinstance(deployment_target, AvailableTarget):
        raise TypeError(
            "Argument for deployment_target must be an enumeration from Enum class AvailableTarget"
        )

    for any_target in AvailableTarget:

        if any_target.value > deployment_target.value:
            missing_features = _get_features_associated_with[any_target](spec)

            if missing_features:
                msg = (
                    "Provided minimum deployment target requires model to be of version {} but converted model "
                    "uses following features which are available from version {} onwards.\n ".format(
                        deployment_target.value, any_target.value
                    )
                )

                for i, feature in enumerate(missing_features):
                    msg += "   {}. {}\n".format(i + 1, feature)

                raise ValueError(msg)

    # Default exception throwing if not able to find the reason behind spec version bump
    if spec.specificationVersion > deployment_target.value:
        msg = (
            "Provided deployment target requires model to be of version {} but converted model has version {} "
            "suitable for later releases".format(
                deployment_target.value, spec.specificationVersion,
            )
        )
        raise ValueError(msg)
