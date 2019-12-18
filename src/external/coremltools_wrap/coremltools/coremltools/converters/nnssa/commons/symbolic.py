import sympy as sm
import numpy as np


def is_symbolic_or_known(val):
    return (np.isscalar(val) and val != -1) or issubclass(type(val), sm.Basic)


def is_symbolic_or_unknown(val):
    return (np.isscalar(val) and val == -1) or issubclass(type(val), sm.Basic)


def is_symbolic(val):
    return issubclass(type(val), sm.Basic)


def any_symbolic_or_unknown(val):
    if is_symbolic_or_unknown(val):
        return True
    elif isinstance(val, np.ndarray) and np.issctype(val.dtype):
        return False
    elif hasattr(val, '__iter__'):
        return any(any_symbolic_or_unknown(i) for i in val)
    else:
        return is_symbolic_or_unknown(val)


def isscalar(val):
    return np.isscalar(val) or issubclass(type(val), sm.Basic)
