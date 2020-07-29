# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
Basic Data Types.
"""
from six import integer_types as _integer_types
import numpy as _np

from ..proto import FeatureTypes_pb2 as _FeatureTypes_pb2
from ..proto import Model_pb2


class _DatatypeBase(object):
    def __init__(self, type_tag, full_tag, num_elements):
        self.type_tag, self.full_tag = type_tag, full_tag
        self.num_elements = num_elements

    def __eq__(self, other):
        return hasattr(other, "full_tag") and self.full_tag == other.full_tag

    def __ne__(self, other):
        return not self.__eq__(other)

    def __hash__(self):
        return hash(self.full_tag)

    def __repr__(self):
        return self.full_tag


class Int64(_DatatypeBase):
    """
    Int64 Data Type
    """

    def __init__(self):
        _DatatypeBase.__init__(self, "Int64", "Int64", 1)


class Double(_DatatypeBase):
    """
    Double Data Type
    """

    def __init__(self):
        _DatatypeBase.__init__(self, "Double", "Double", 1)


class String(_DatatypeBase):
    """
    String Data Type
    """

    def __init__(self):
        _DatatypeBase.__init__(self, "String", "String", 1)


class Array(_DatatypeBase):
    """
    Array Data Type
    """

    def __init__(self, *dimensions):
        """
        Constructs a Array, given its dimensions

        Parameters
        ----------
        dimensions: ints | longs

        Examples
        --------
        # Create a single dimensions array of length five
        >>> arr = coremltools.models.datatypes.Array(5)

        # Create a multi dimension array five by two by ten.
        >>> multi_arr = coremltools.models.datatypes.Array(5, 2, 10)
        """
        assert len(dimensions) >= 1
        assert all(
            isinstance(d, _integer_types + (_np.int64, _np.int32)) for d in dimensions
        ), "Dimensions must be ints, not {}".format(str(dimensions))
        self.dimensions = dimensions

        num_elements = 1
        for d in self.dimensions:
            num_elements *= d

        _DatatypeBase.__init__(
            self,
            "Array",
            "Array({%s})" % (",".join("%d" % d for d in self.dimensions)),
            num_elements,
        )


class Dictionary(_DatatypeBase):
    """
    Dictionary Data Type
    """

    def __init__(self, key_type=None):
        """
        Constructs a Dictionary, given its key type

        Parameters
        ----------
        key_type: Int64 | String

        Examples
        --------
        >>> from coremltools.models.datatypes import Dictionary, Int64, String

        # Create a dictionary with string keys
        >>> str_key_dict = Dictionary(key_type=String)

        # Create a dictionary with int keys
        >>> int_key_dict = Dictionary(Int64)
        """
        # Resolve it to a class if it's
        global _simple_type_remap
        if key_type in _simple_type_remap:
            key_type = _simple_type_remap[key_type]

        if not isinstance(key_type, (Int64, String)):
            raise TypeError("Key type for dictionary must be either string or integer.")

        self.key_type = key_type

        _DatatypeBase.__init__(
            self, "Dictionary", "Dictionary(%s)" % repr(self.key_type), None
        )


_simple_type_remap = {
    int: Int64(),
    str: String(),
    float: Double(),
    Double: Double(),
    Int64: Int64(),
    String: String(),
    "Double": Double(),
    "Int64": Int64(),
    "String": String(),
}


def _is_valid_datatype(datatype_instance):
    """
    Returns true if datatype_instance is a valid datatype object and false otherwise.
    """

    # Remap so we can still use the python types for the simple cases
    global _simple_type_remap
    if datatype_instance in _simple_type_remap:
        return True

    # Now set the protobuf from this interface.
    if isinstance(datatype_instance, (Int64, Double, String, Array)):
        return True

    elif isinstance(datatype_instance, Dictionary):
        kt = datatype_instance.key_type

        if isinstance(kt, (Int64, String)):
            return True

    return False


def _normalize_datatype(datatype_instance):
    """
    Translates a user specified datatype to an instance of the ones defined above.

    Valid data types are passed through, and the following type specifications
    are translated to the proper instances:

    str, "String" -> String()
    int, "Int64" -> Int64()
    float, "Double" -> Double()

    If a data type is not recognized, then an error is raised.
    """
    global _simple_type_remap
    if datatype_instance in _simple_type_remap:
        return _simple_type_remap[datatype_instance]

    # Now set the protobuf from this interface.
    if isinstance(datatype_instance, (Int64, Double, String, Array)):
        return datatype_instance

    elif isinstance(datatype_instance, Dictionary):
        kt = datatype_instance.key_type

        if isinstance(kt, (Int64, String)):
            return datatype_instance

    raise ValueError("Datatype instance not recognized.")


def _set_datatype(
    proto_type_obj, datatype_instance, array_datatype=Model_pb2.ArrayFeatureType.DOUBLE
):
    # Remap so we can still use the python types for the simple cases
    global _simple_type_remap
    if datatype_instance in _simple_type_remap:
        datatype_instance = _simple_type_remap[datatype_instance]

    # Now set the protobuf from this interface.
    if isinstance(datatype_instance, Int64):
        proto_type_obj.int64Type.MergeFromString(b"")

    elif isinstance(datatype_instance, Double):
        proto_type_obj.doubleType.MergeFromString(b"")

    elif isinstance(datatype_instance, String):
        proto_type_obj.stringType.MergeFromString(b"")

    elif isinstance(datatype_instance, Array):
        proto_type_obj.multiArrayType.MergeFromString(b"")
        proto_type_obj.multiArrayType.dataType = array_datatype

        for n in datatype_instance.dimensions:
            proto_type_obj.multiArrayType.shape.append(n)

    elif isinstance(datatype_instance, Dictionary):
        proto_type_obj.dictionaryType.MergeFromString(b"")

        kt = datatype_instance.key_type

        if isinstance(kt, Int64):
            proto_type_obj.dictionaryType.int64KeyType.MergeFromString(b"")
        elif isinstance(kt, String):
            proto_type_obj.dictionaryType.stringKeyType.MergeFromString(b"")
        else:
            raise ValueError("Dictionary key type must be either string or int.")

    else:
        raise TypeError(
            "Datatype parameter not recognized; must be an instance "
            "of datatypes.{Double, Int64, String, Dictionary, Array}, or "
            "python int, float, or str types."
        )
