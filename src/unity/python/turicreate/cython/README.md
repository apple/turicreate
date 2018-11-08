# Cython 

### Table of Contents
- The Extensions Mechanism

## The Extensions Mechanism

#### Motivation

The TuriCreate Framework uses Cython as the glue between the Python and the C++ library. Cython users, for example can create C++ interfaces to header files to call classes, and functions. A secondary Proxy, or wrapper class is used to expose functionality to Python. As you can imagine, this can be a very cumbersome process. Every time a new C++ class or function is created, a Cython interface must be written to make it’s functionality accessible in Python.

Fortunately the extensions mechanism is a unique solution to this problem. The extensions mechanism is a high level abstraction on top of Cython, that provides a simple way of exposing C++ functions, and classes to python without explicitly having to write any Cython glue code to do so.

#### How do I use it?

#### How does it work?

There are a number of different pieces that enable this extensions mechanism to work. Let’s start on the Python side and plumb our way through to the underlying Registration Code. The first piece of code we are interested in is shown below



```
class _extensions_wrapper(object):
  def __init__(self, wrapped):
    self._wrapped = wrapped
    self.__doc__ = wrapped.__doc__

  def __getattr__(self, name):
    try:
        return getattr(self._wrapped, name)
    except:
        pass
    turicreate.connect.main.get_unity()
    return getattr(self._wrapped, name)

import sys as _sys
_sys.modules["turicreate.extensions"] = _extensions_wrapper(_sys.modules["turicreate.extensions"])
# rewrite the import
extensions = _sys.modules["turicreate.extensions"]
```
file: `src/unity/python/turicreate__init__.py` 