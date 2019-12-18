Core ML Community Tools
=======================

Core ML community tools contains all supporting tools for Core ML model
conversion and validation. This includes scikit-learn, LIBSVM, Caffe,
Keras and XGBoost.

coremltools 3.0
---------------
[Release notes](https://github.com/apple/coremltools/releases/)


👍👎 Please take this quick poll and let us know how you liked this release, [here](https://github.com/apple/coremltools/blob/master/release-feedback.md)!

```shell
# Install using pip
pip install coremltools==3.0
```

API
---
- [Example Code Snippets](docs/APIExamples.md)
- [coremltools Documentation](https://apple.github.io/coremltools)
- [Core ML Specification Documentation](https://apple.github.io/coremltools/coremlspecification/)
- [IPython Notebooks](https://github.com/apple/coremltools/tree/master/examples)

Installation
------------

We recommend using virtualenv to use, install, or build coremltools. Be
sure to install virtualenv using your system pip.

```shell
pip install virtualenv
```

The method for installing *coremltools* follows the
[standard python package installation steps](https://packaging.python.org/installing/).
To create a Python virtual environment called `pythonenv` follow these steps:

```shell
# Create a folder for your virtualenv
mkdir mlvirtualenv
cd mlvirtualenv

# Create a Python virtual environment for your Core ML project
virtualenv pythonenv
```

To activate your new virtual environment and install `coremltools` in this environment, follow these steps:
```
# Active your virtual environment
source pythonenv/bin/activate


# Install coremltools in the new virtual environment, pythonenv
(pythonenv) pip install -U coremltools
```

The package [documentation](https://apple.github.io/coremltools) contains
more details on how to use coremltools.

Dependencies
------------

*coremltools* has the following dependencies:

- numpy (1.10.0+)
- protobuf (3.1.0+)

In addition, it has the following soft dependencies that are only needed when
you are converting models of these formats:

- Keras (1.2.2, 2.0.4+) with corresponding TensorFlow version
- XGBoost (0.7+)
- scikit-learn (0.17+)
- LIBSVM


Building from source
--------------------
To build the project, you need [CMake](https://cmake.org) to configure the project

```shell
mkdir build
cd build
cmake ../
```

When several python virtual environments are installed,
it may be useful to use the following command instead,
to point to the correct intended version of python:

```shell
cmake \
  -DPYTHON_EXECUTABLE:FILEPATH=/Library/Frameworks/Python.framework/Versions/3.7/bin/python \
  -DPYTHON_INCLUDE_DIR=/Library/Frameworks/Python.framework/Versions/3.7/include/python3.7m/ \
  -DPYTHON_LIBRARY=/Library/Frameworks/Python.framework/Versions/3.7/lib/ \
  ../
```
after which you can use make to build the project

```shell
make
```

Building Installable Wheel
---------------------------
To make a wheel/egg that you can distribute, you can do the following

```shell
make dist
```

Running Unit Tests
-------------------
In order to run unit tests, you need `pytest`, `pandas`, and `h5py`.

```shell
pip install pytest pandas h5py
```

To add a new unit test, add it to the `coremltools/test` folder. Make sure you
name the file with a 'test' as the prefix.

Additionally, running unit-tests would require more packages (like
LIBSVM)

```shell
pip install -r test_requirements.pip
```

To install LIBSVM

```shell
git clone https://github.com/cjlin1/libsvm.git
cd libsvm/
make
cd python/
make
```

To make sure you can run LIBSVM python bindings everywhere, you need the
following command, replacing `<LIBSVM_PATH>` with the path to the root of
your repository.

```shell
export PYTHONPATH=${PYTHONPATH}:<LIBSVM_PATH>/python
```

To install XGBoost

```shell
git clone --recursive https://github.com/dmlc/xgboost
cd xgboost
git checkout v0.90
git submodule update
make config=make/config.mk -j8
cd python-package; python setup.py develop
```

To install Keras (Version >= 2.0)

```shell
pip install keras tensorflow
```

If you'd like to use the old Keras version, you can:

```shell
pip install keras==1.2.2 tensorflow
```

Finally, to run the most important unit tests, you can use:

```shell
pytest -rs
```
some tests are marked as slow because they test a lot of combinations.
If you want to run, all tests, you can use:

```shell
pytest
```

Building Documentation
----------------------
First install all external dependencies.

```shell
pip install Sphinx==1.8.5 sphinx-rtd-theme==0.4.3 numpydoc==0.9.1
pip install -e git+git://github.com/michaeljones/sphinx-to-github.git#egg=sphinx-to-github
```

You also must have the *coremltools* package install, see the *Building* section.

Then from the root of the repository:

```shell
cd docs
make html
open _build/html/index.html
```

External Tools
--------------
In addition to the conversion tools in this package, TensorFlow, ONNX, and MXNet have their own conversion tools:

- [TensorFlow](https://pypi.python.org/pypi/tfcoreml)
- [MXNet](https://github.com/apache/incubator-mxnet/tree/master/tools/coreml)
- [ONNX](https://github.com/onnx/onnx-coreml)
