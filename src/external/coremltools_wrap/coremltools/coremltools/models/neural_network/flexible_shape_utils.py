# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
Utilities to annotate Neural Network Features with flexible shape information.
Only available in coremltools 2.0b1 and onwards
"""

from ..utils import _get_feature, _get_nn_layers, _get_input_names
from ... import _MINIMUM_FLEXIBLE_SHAPES_SPEC_VERSION
from ... import _MINIMUM_NDARRAY_SPEC_VERSION

_SEQUENCE_KEY = "S"
_BATCH_KEY = "B"
_CHANNEL_KEY = "C"
_HEIGHT_KEY = "H"
_WIDTH_KEY = "W"

_CONSTRAINED_KEYS = [_CHANNEL_KEY, _HEIGHT_KEY, _WIDTH_KEY]


class Shape(object):
    def __init__(self, shape_value):
        if shape_value < 1:
            raise Exception("Invalid value. Size/Shape values must be > 0")
        self._value = shape_value

    @property
    def value(self):
        return self._value


class Size(Shape):
    def __init__(self, size_value):
        super(Size, self).__init__(size_value)


class NeuralNetworkMultiArrayShape:
    """
    An object representing a shape for a multiArray feature in a
    neural network. Valid shapes must have have only the Channel [C]
    shape or the Channel, Height and Width [C, H, W] shapes populated
    """

    def __init__(self, channel=None, height=None, width=None):
        self._shape = {
            _CHANNEL_KEY: Shape(int(channel)) if channel else None,
            _HEIGHT_KEY: Shape(int(height)) if height else None,
            _WIDTH_KEY: Shape(int(width)) if width else None,
        }

    def set_channel_shape(self, channel_shape):
        self._shape[_CHANNEL_KEY] = Shape(channel_shape)

    def set_height_shape(self, height_shape):
        self._shape[_HEIGHT_KEY] = Shape(height_shape)

    def set_width_shape(self, width_shape):
        self._shape[_WIDTH_KEY] = Shape(width_shape)

    def _validate_multiarray_shape(self):
        num_dims = len([v for v in self._shape.values() if v])
        if num_dims != 1 and num_dims != 3:
            raise Exception(
                "For neural networks, shape must be of length 1 or 3"
                ", representing input shape [C] or [C,H,W], respectively"
            )

        if num_dims == 1:
            if not self._shape["C"]:
                raise Exception("Channel Shape not specified")

    @property
    def multiarray_shape(self):
        num_dims = len([v for v in self._shape.values() if v])
        if num_dims == 1:
            return [self._shape[_CHANNEL_KEY].value]
        elif num_dims == 3:
            return [
                self._shape[_CHANNEL_KEY].value,
                self._shape[_HEIGHT_KEY].value,
                self._shape[_WIDTH_KEY].value,
            ]
        else:
            raise Exception("Invalid multiarray shape for neural network")


class NeuralNetworkImageSize:
    """
    An object representing a size for an image feature inside a
    neural network. Valid sizess for height and width are > 0.
    """

    def __init__(self, height=None, width=None):
        self._height = Size(height)
        self._width = Size(width)

    def set_width(self, width):
        self._width = Size(width)

    def set_height(self, height):
        self._height = Size(height)

    @property
    def width(self):
        return self._width.value

    @property
    def height(self):
        return self._height.value


class ShapeRange(object):
    def __init__(self, lowerBound, upperBound):
        unBounded = False

        if upperBound == -1:
            unBounded = True

        if not unBounded and lowerBound > upperBound:
            raise Exception(
                "lowerBound > upperBound for range ({},{})".format(
                    lowerBound, upperBound
                )
            )

        if not unBounded and upperBound < 1:
            raise Exception("Invalid upperBound: {} ".format(upperBound))

        if lowerBound == 0:
            lowerBound = 1

        if lowerBound < 1:
            raise Exception("Invalid lowerBound: {}".format(lowerBound))

        self._lowerBound = lowerBound
        self._upperBound = upperBound
        self._unBounded = unBounded

    @property
    def lowerBound(self):
        return self._lowerBound

    @property
    def upperBound(self):
        return self._upperBound

    @property
    def isUnbounded(self):
        return self._unBounded

    @property
    def isFlexible(self):
        return not (self._lowerBound == self._upperBound)


class NeuralNetworkMultiArrayShapeRange:
    """
    An object representing a range of shapes for a multiArray feature in a
    neural network. Valid shape ranges must have have only the Channel [C]
    range or the Channel, Height and Width [C, H, W] ranges populated. A "-1"
    value in an upper bound represents an unbounded range.
    """

    def __init__(self, input_ranges=None):
        self.arrayShapeRange = {}

        if input_ranges:
            if not isinstance(input_ranges, dict):
                raise Exception(
                    "Attempting to initialize a shape range with something other than a dictionary of shapes."
                )
            self.arrayShapeRange = {}
            for key, value in input_ranges.items():
                if key in _CONSTRAINED_KEYS:
                    self.arrayShapeRange[key] = self._create_shape_range(value)
            self.validate_array_shape_range()

    def _create_shape_range(self, r):
        if not isinstance(r, tuple):
            raise Exception("Range should be a ShapeRange or a tuple object")
        elif len(r) != 2:
            raise Exception("Range tuple should be at least length 2")
        return ShapeRange(r[0], r[1])

    def add_channel_range(self, channel_range):
        if not isinstance(channel_range, ShapeRange):
            channel_range = self._create_shape_range(channel_range)
        self.arrayShapeRange[_CHANNEL_KEY] = channel_range

    def add_height_range(self, height_range):
        if not isinstance(height_range, ShapeRange):
            height_range = self._create_shape_range(height_range)
        self.arrayShapeRange[_HEIGHT_KEY] = height_range

    def add_width_range(self, width_range):
        if not isinstance(width_range, ShapeRange):
            width_range = self._create_shape_range(width_range)
        self.arrayShapeRange[_WIDTH_KEY] = width_range

    def get_shape_range_dims(self):
        return len(self.arrayShapeRange.keys())

    def validate_array_shape_range(self):
        num_dims = self.get_shape_range_dims()
        if num_dims != 1 and num_dims != 3:
            raise Exception(
                "For neural networks, shape must be of length 1 or 3"
                ", representing input shape [C] or [C,H,W], respectively"
            )

        if num_dims == 1:
            if _CHANNEL_KEY not in self.arrayShapeRange.keys():
                raise Exception("Channel Shape Range not specified")

        if num_dims == 3:
            if (
                _CHANNEL_KEY not in self.arrayShapeRange.keys()
                or _HEIGHT_KEY not in self.arrayShapeRange.keys()
                or _WIDTH_KEY not in self.arrayShapeRange.keys()
            ):
                raise Exception(
                    "Shape range constraint missing for either channel, height, or width."
                )

    def get_channel_range(self):
        return self.arrayShapeRange[_CHANNEL_KEY]

    def get_height_range(self):
        return self.arrayShapeRange[_HEIGHT_KEY]

    def get_width_range(self):
        return self.arrayShapeRange[_WIDTH_KEY]

    def isFlexible(self):
        """
        Returns true if any one of the channel, height, or width ranges of this shape allow more than one input value.
        """
        for key, value in self.arrayShapeRange.items():
            if key in _CONSTRAINED_KEYS:
                if value.isFlexible:
                    return True

        return False


class NeuralNetworkImageSizeRange:
    """
    An object representing a range of sizes for an image feature inside a
    neural network. Valid ranges for height and width are > 0. A "-1"
    upper bound value for either width or height represents an unbounded size
    for that dimension.
    """

    def __init__(self, height_range=None, width_range=None):
        if height_range and not isinstance(height_range, ShapeRange):
            if not isinstance(height_range, tuple):
                raise Exception("Height range should be a ShapeRange or a tuple object")
            elif len(height_range) != 2:
                raise Exception("Height range tuple should be at least length 2")
            height_range = ShapeRange(height_range[0], height_range[1])

        if width_range and not isinstance(width_range, ShapeRange):
            if not isinstance(width_range, tuple):
                raise Exception("Width range should be a ShapeRange or a tuple object")
            elif len(width_range) != 2:
                raise Exception("Width range tuple should be at least length 2")
            width_range = ShapeRange(width_range[0], width_range[1])

        self._height_range = height_range
        self._width_range = width_range

    def add_width_range(self, width_range):
        if not isinstance(width_range, ShapeRange):
            if not isinstance(width_range, tuple):
                raise Exception("Width range should be a ShapeRange or a tuple object")
            elif len(width_range) != 2:
                raise Exception("Width range tuple should be at least length 2")

        self._width_range = ShapeRange(width_range[0], width_range[1])

    def add_height_range(self, height_range):
        if not isinstance(height_range, ShapeRange):
            if not isinstance(height_range, tuple):
                raise Exception("Height range should be a ShapeRange or a tuple object")
            elif len(height_range) != 2:
                raise Exception("Height range tuple should be at least length 2")

        self._height_range = ShapeRange(height_range[0], height_range[1])

    def get_width_range(self):
        return self._width_range

    def get_height_range(self):
        return self._height_range


def add_enumerated_multiarray_shapes(spec, feature_name, shapes):
    """
    Annotate an input or output multiArray feature in a Neural Network spec to
    to accommodate a list of enumerated array shapes

    :param spec: MLModel
        The MLModel spec containing the feature

    :param feature_name: str
        The name of the image feature for which to add shape information.
        If the feature is not found in the input or output descriptions then
        an exception is thrown

    :param shapes: [] | NeuralNetworkMultiArrayShape
        A single or a list of NeuralNetworkImageSize objects which encode valid
        size information for a image feature

    Examples
    --------
    .. sourcecode:: python

        >>> import coremltools
        >>> from coremltools.models.neural_network import flexible_shape_utils
        >>> spec = coremltools.utils.load_spec('mymodel.mlmodel')
        >>> array_shapes = [flexible_shape_utils.NeuralNetworkMultiArrayShape(3)]
        >>> second_shape = flexible_shape_utils.NeuralNetworkMultiArrayShape()
        >>> second_shape.set_channel_shape(3)
        >>> second_shape.set_height_shape(10)
        >>> second_shape.set_width_shape(15)
        >>> array_shapes.append(second_shape)
        >>> flexible_shape_utils.add_enumerated_multiarray_shapes(spec, feature_name='my_multiarray_featurename', shapes=array_shapes)

    :return:
        None. The spec object is updated
    """

    if not isinstance(shapes, list):
        shapes = [shapes]

    for shape in shapes:
        if not isinstance(shape, NeuralNetworkMultiArrayShape):
            raise Exception(
                "Shape ranges should be of type NeuralNetworkMultiArrayShape"
            )
        shape._validate_multiarray_shape()

    feature = _get_feature(spec, feature_name)
    if feature.type.WhichOneof("Type") != "multiArrayType":
        raise Exception(
            "Trying to add enumerated shapes to " "a non-multiArray feature type"
        )

    if feature.type.multiArrayType.WhichOneof("ShapeFlexibility") != "enumeratedShapes":
        feature.type.multiArrayType.ClearField("ShapeFlexibility")

    eshape_len = len(feature.type.multiArrayType.enumeratedShapes.shapes)

    # Add default array shape to list of enumerated shapes if enumerated shapes
    # field is currently empty
    if eshape_len == 0:
        fixed_shape = feature.type.multiArrayType.shape
        if len(fixed_shape) == 1:
            fs = NeuralNetworkMultiArrayShape(fixed_shape[0])
            shapes.append(fs)
        elif len(fixed_shape) == 3:
            fs = NeuralNetworkMultiArrayShape()
            fs.set_channel_shape(fixed_shape[0])
            fs.set_height_shape(fixed_shape[1])
            fs.set_width_shape(fixed_shape[2])
            shapes.append(fs)
        else:
            raise Exception(
                "Original fixed multiArray shape for {} is invalid".format(feature_name)
            )

    for shape in shapes:
        s = feature.type.multiArrayType.enumeratedShapes.shapes.add()
        s.shape.extend(shape.multiarray_shape)

    # Bump up specification version
    spec.specificationVersion = max(
        _MINIMUM_FLEXIBLE_SHAPES_SPEC_VERSION, spec.specificationVersion
    )


def add_enumerated_image_sizes(spec, feature_name, sizes):
    """
    Annotate an input or output image feature in a Neural Network spec to
    to accommodate a list of enumerated image sizes

    :param spec: MLModel
        The MLModel spec containing the feature

    :param feature_name: str
        The name of the image feature for which to add size information.
        If the feature is not found in the input or output descriptions then
        an exception is thrown

    :param sizes:  [] | NeuralNetworkImageSize
        A single or a list of NeuralNetworkImageSize objects which encode valid
        size information for a image feature

    Examples
    --------
    .. sourcecode:: python

        >>> import coremltools
        >>> from coremltools.models.neural_network import flexible_shape_utils
        >>> spec = coremltools.utils.load_spec('mymodel.mlmodel')
        >>> image_sizes = [flexible_shape_utils.NeuralNetworkImageSize(128, 128)]
        >>> image_sizes.append(flexible_shape_utils.NeuralNetworkImageSize(256, 256))
        >>> flexible_shape_utils.add_enumerated_image_sizes(spec, feature_name='my_multiarray_featurename', sizes=image_sizes)

    :return:
        None. The spec object is updated
    """
    if not isinstance(sizes, list):
        sizes = [sizes]

    for size in sizes:
        if not isinstance(size, NeuralNetworkImageSize):
            raise Exception("Shape ranges should be of type NeuralNetworkImageSize")

    feature = _get_feature(spec, feature_name)
    if feature.type.WhichOneof("Type") != "imageType":
        raise Exception("Trying to add enumerated sizes to " "a non-image feature type")

    if feature.type.imageType.WhichOneof("SizeFlexibility") != "enumeratedSizes":
        feature.type.imageType.ClearField("SizeFlexibility")

    esizes_len = len(feature.type.imageType.enumeratedSizes.sizes)

    # Add default image size to list of enumerated sizes if enumerated sizes
    # field is currently empty
    if esizes_len == 0:
        fixed_height = feature.type.imageType.height
        fixed_width = feature.type.imageType.width
        sizes.append(NeuralNetworkImageSize(fixed_height, fixed_width))

    for size in sizes:
        s = feature.type.imageType.enumeratedSizes.sizes.add()
        s.height = size.height
        s.width = size.width

    # Bump up specification version
    spec.specificationVersion = max(
        _MINIMUM_FLEXIBLE_SHAPES_SPEC_VERSION, spec.specificationVersion
    )


def update_image_size_range(spec, feature_name, size_range):
    """
    Annotate an input or output Image feature in a Neural Network spec to
    to accommodate a range of image sizes

    :param spec: MLModel
        The MLModel spec containing the feature

    :param feature_name: str
        The name of the Image feature for which to add shape information.
        If the feature is not found in the input or output descriptions then
        an exception is thrown

    :param size_range: NeuralNetworkImageSizeRange
        A NeuralNetworkImageSizeRange object with the populated image size
        range information.

    Examples
    --------
    .. sourcecode:: python

        >>> import coremltools
        >>> from coremltools.models.neural_network import flexible_shape_utils
        >>> spec = coremltools.utils.load_spec('mymodel.mlmodel')
        >>> img_size_ranges = flexible_shape_utils.NeuralNetworkImageSizeRange()
        >>> img_size_ranges.add_height_range(64, 128)
        >>> img_size_ranges.add_width_range(128, -1)
        >>> flexible_shape_utils.update_image_size_range(spec, feature_name='my_multiarray_featurename', size_range=img_size_ranges)

    :return:
        None. The spec object is updated
    """
    if not isinstance(size_range, NeuralNetworkImageSizeRange):
        raise Exception("Shape ranges should be of type NeuralNetworkImageSizeRange")

    feature = _get_feature(spec, feature_name)
    if feature.type.WhichOneof("Type") != "imageType":
        raise Exception("Trying to add size ranges for " "a non-image feature type")

    feature.type.imageType.ClearField("SizeFlexibility")
    feature.type.imageType.imageSizeRange.heightRange.lowerBound = (
        size_range.get_height_range().lowerBound
    )
    feature.type.imageType.imageSizeRange.heightRange.upperBound = (
        size_range.get_height_range().upperBound
    )

    feature.type.imageType.imageSizeRange.widthRange.lowerBound = (
        size_range.get_width_range().lowerBound
    )
    feature.type.imageType.imageSizeRange.widthRange.upperBound = (
        size_range.get_width_range().upperBound
    )

    # Bump up specification version
    spec.specificationVersion = max(
        _MINIMUM_FLEXIBLE_SHAPES_SPEC_VERSION, spec.specificationVersion
    )


def update_multiarray_shape_range(spec, feature_name, shape_range):
    """
    Annotate an input or output MLMultiArray feature in a Neural Network spec
    to accommodate a range of shapes

    :param spec: MLModel
        The MLModel spec containing the feature

    :param feature_name: str
        The name of the feature for which to add shape range
        information. If the feature is not found in the input or output
        descriptions then an exception is thrown

    :param shape_range: NeuralNetworkMultiArrayShapeRange
        A NeuralNetworkMultiArrayShapeRange object with the populated shape
        range information. The shape_range object must either contain only
        shape information for channel or channel, height and width. If
        the object is invalid then an exception is thrown

    Examples
    --------
    .. sourcecode:: python

        >>> import coremltools
        >>> from coremltools.models.neural_network import flexible_shape_utils
        >>> spec = coremltools.utils.load_spec('mymodel.mlmodel')
        >>> shape_range = flexible_shape_utils.NeuralNetworkMultiArrayShapeRange()
        >>> shape_range.add_channel_range((1, 3))
        >>> shape_range.add_width_range((128, 256))
        >>> shape_range.add_height_range((128, 256))
        >>> flexible_shape_utils.update_multiarray_shape_range(spec, feature_name='my_multiarray_featurename', shape_range=shape_range)

    :return:
        None. The spec is updated
    """
    if not isinstance(shape_range, NeuralNetworkMultiArrayShapeRange):
        raise Exception("Shape range should be of type MultiArrayShapeRange")

    shape_range.validate_array_shape_range()
    feature = _get_feature(spec, feature_name)

    if feature.type.WhichOneof("Type") != "multiArrayType":
        raise Exception(
            "Trying to update shape range for " "a non-multiArray feature type"
        )

    # Add channel range
    feature.type.multiArrayType.ClearField("ShapeFlexibility")
    s = feature.type.multiArrayType.shapeRange.sizeRanges.add()
    s.lowerBound = shape_range.get_channel_range().lowerBound
    s.upperBound = shape_range.get_channel_range().upperBound

    if shape_range.get_shape_range_dims() > 1:
        # Add height range
        s = feature.type.multiArrayType.shapeRange.sizeRanges.add()
        s.lowerBound = shape_range.get_height_range().lowerBound
        s.upperBound = shape_range.get_height_range().upperBound
        # Add width range
        s = feature.type.multiArrayType.shapeRange.sizeRanges.add()
        s.lowerBound = shape_range.get_width_range().lowerBound
        s.upperBound = shape_range.get_width_range().upperBound

    # Bump up specification version
    spec.specificationVersion = max(
        _MINIMUM_FLEXIBLE_SHAPES_SPEC_VERSION, spec.specificationVersion
    )


def set_multiarray_ndshape_range(spec, feature_name, lower_bounds, upper_bounds):
    """
    Annotate an input or output MLMultiArray feature in a Neural Network spec
    to accommodate a range of shapes.
    This is different from "update_multiarray_shape_range", which works with rank 5
    SBCHW mapping.

    :param spec: MLModel
        The MLModel spec containing the feature

    :param feature_name: str
        The name of the feature for which to add shape range
        information. If the feature is not found in the input or output
        descriptions then an exception is thrown

    :param lower_bounds: List[int]
        list of integers specifying the lower bounds of each dimension.
        Length must be same as the rank (length of shape) of the feature_name.

    :param upper_bounds: List[int]
        list of integers specifying the upper bounds of each dimension.
        -1 corresponds to unbounded range.
        Length must be same as the rank (length of shape) of the feature_name.


    Examples
    --------
    .. sourcecode:: python

        >>> import coremltools
        >>> from coremltools.models.neural_network import flexible_shape_utils
        >>> spec = coremltools.utils.load_spec('mymodel.mlmodel')
        >>> # say, the default shape of "my_multiarray_featurename" is (2,3)
        >>> flexible_shape_utils.set_multiarray_ndshape_range(spec, feature_name='my_multiarray_featurename', lower_bounds=[1,2], upper_bounds=[10,-1])

    :return:
        None. The spec is updated
    """
    if not isinstance(lower_bounds, list):
        raise Exception("lower_bounds must be a list")
    if not isinstance(upper_bounds, list):
        raise Exception("upper_bounds must be a list")

    feature = _get_feature(spec, feature_name)

    if feature.type.WhichOneof("Type") != "multiArrayType":
        raise Exception(
            "Trying to update shape range for " "a non-multiArray feature type"
        )

    shape = feature.type.multiArrayType.shape

    if len(shape) != len(lower_bounds):
        raise Exception(
            "Length of lower_bounds is not equal to the number of dimensions in the default shape"
        )
    if len(shape) != len(upper_bounds):
        raise Exception(
            "Length of upper_bounds is not equal to the number of dimensions in the default shape"
        )

    feature.type.multiArrayType.ClearField("ShapeFlexibility")

    for i in range(len(lower_bounds)):
        if shape[i] < lower_bounds[i]:
            raise Exception(
                "Default shape in %d-th dimension, which is %d, is smaller"
                " than the lower bound of %d" % (i, int(shape[i]), lower_bounds[i])
            )
        if upper_bounds[i] != -1:
            if shape[i] > upper_bounds[i]:
                raise Exception(
                    "Default shape in %d-th dimension, which is %d, is greater"
                    " than the upper bound of %d" % (i, int(shape[i]), upper_bounds[i])
                )

        s = feature.type.multiArrayType.shapeRange.sizeRanges.add()
        s.lowerBound = lower_bounds[i]
        s.upperBound = upper_bounds[i]

    # Bump up specification version
    spec.specificationVersion = max(
        _MINIMUM_NDARRAY_SPEC_VERSION, spec.specificationVersion
    )


def add_multiarray_ndshape_enumeration(spec, feature_name, enumerated_shapes):
    """
    Annotate an input or output MLMultiArray feature in a Neural Network spec
    to accommodate a range of shapes.
    Add provided enumerated shapes to the list of shapes already present.
    This method is different from "add_enumerated_multiarray_shapes", which is applicable
    for rank 5 mapping, SBCHW, arrays.

    :param spec: MLModel
        The MLModel spec containing the feature

    :param feature_name: str
        The name of the feature for which to add shape range
        information. If the feature is not found in the input or output
        descriptions then an exception is thrown

    :param enumerated_shapes: List[Tuple(int)]
        list of shapes, where each shape is specified as a tuple of integers.


    Examples
    --------
    .. sourcecode:: python

        >>> import coremltools
        >>> from coremltools.models.neural_network import flexible_shape_utils
        >>> spec = coremltools.utils.load_spec('mymodel.mlmodel')
        >>> # say, the default shape of "my_multiarray_featurename" is (2,3)
        >>> flexible_shape_utils.add_multiarray_ndshape_enumeration(spec, feature_name='my_multiarray_featurename', enumerated_shapes=[(2,4), (2,6)])

    :return:
        None. The spec is updated
    """
    if not isinstance(enumerated_shapes, list):
        raise Exception("enumerated_shapes must be a list")
    if len(enumerated_shapes) == 0:
        raise Exception("enumerated_shapes is empty")

    feature = _get_feature(spec, feature_name)
    if feature.type.WhichOneof("Type") != "multiArrayType":
        raise Exception(
            "Trying to update shape range for " "a non-multiArray feature type"
        )

    shape = feature.type.multiArrayType.shape

    if feature.type.multiArrayType.WhichOneof("ShapeFlexibility") != "enumeratedShapes":
        feature.type.multiArrayType.ClearField("ShapeFlexibility")

    eshape_len = len(feature.type.multiArrayType.enumeratedShapes.shapes)

    # Add default array shape to list of enumerated shapes if enumerated shapes
    # field is currently empty
    if eshape_len == 0:
        fixed_shape = feature.type.multiArrayType.shape
        s = feature.type.multiArrayType.enumeratedShapes.shapes.add()
        s.shape.extend(fixed_shape)

    for shape in enumerated_shapes:
        if not isinstance(shape, tuple):
            raise Exception("An element in 'enumerated_shapes' is not a tuple")
        s = feature.type.multiArrayType.enumeratedShapes.shapes.add()
        s.shape.extend(list(shape))

    # Bump up specification version
    spec.specificationVersion = max(
        _MINIMUM_NDARRAY_SPEC_VERSION, spec.specificationVersion
    )
