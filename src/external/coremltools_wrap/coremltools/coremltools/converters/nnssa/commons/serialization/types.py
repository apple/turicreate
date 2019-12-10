from enum import Enum, unique
import numpy as np


# This should line up with C++'s src/numeric/primitive_type.hpp
@unique
class np_types(Enum):
    bool_ = 0
    int8 = 1
    int16 = 2
    int32 = 3
    int64 = 4
    uint8 = 5
    uint16 = 6
    uint32 = 7
    uint64 = 8
    float32 = 9
    float64 = 10


@unique
class py_types(Enum):
    int = 0
    double = 1
    str = 2
    list = 4
    dict = 5
    ndarray = 9


def dump_np_types(npt):
    if npt == np.bool:
        return np_types.bool_.value
    elif npt == np.int8:
        return np_types.int8.value
    elif npt == np.int16:
        return np_types.int16.value
    elif npt == np.int32:
        return np_types.int32.value
    elif npt == np.int64:
        return np_types.int64.value
    elif npt == np.uint8:
        return np_types.uint8.value
    elif npt == np.uint16:
        return np_types.uint16.value
    elif npt == np.uint32:
        return np_types.uint32.value
    elif npt == np.uint64:
        return np_types.uint64.value
    elif npt == np.float32:
        return np_types.float32.value
    elif npt == np.float64:
        return np_types.float64.value
    else:
        raise ValueError("Cannot dump type %s" % (type(npt)))
