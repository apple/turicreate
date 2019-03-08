Core ML Community Tools
=======================

Core ML community tools contains all supporting tools for CoreML model
conversion and validation. This includes Scikit Learn, LIBSVM, Caffe,
Keras and XGBoost.

API
---
[Example Code snippets](docs/APIExamples.md)
[CoreMLTools Documentation](https://apple.github.io/coremltools)
[CoreML Specification Documentation](https://apple.github.io/coremltools/coremlspecification/)
[IPython Notebooks](https://github.com/apple/coremltools/tree/master/examples)

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

# Create a Python virtual environment for your CoreML project
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

- Keras (1.2.2, 2.0.4+) with corresponding Tensorflow version
- Xgboost (0.7+)
- scikit-learn (0.17+)
- libSVM


Building from source
--------------------
To build the project, you need [CMake](https://cmake.org) to configure the project

```shell
cmake .
```

When several python virtual environments are installed,
it may be useful to use the following command instead,
to point to the correct intended version of python:

```shell
cmake . -DPYTHON=$(which python) -DPYTHON_CONFIG=$(which python-config)
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
In order to run unit tests, you need pytest, pandas, and h5py.

```shell
pip install pytest pandas h5py
```

To add a new unit test, add it to the `coremltools/test` folder. Make sure you
name the file with a 'test' as the prefix.

Additionally, running unit-tests would require more packages (like
libsvm)

```shell
pip install -r test_requirements.pip
```

To install libsvm

```shell
git clone https://github.com/cjlin1/libsvm.git
cd libsvm/
make
cd python/
make
```

To make sure you can run libsvm python bindings everywhere, you need the
following command, replacing `<LIBSVM_PATH>` with the path to the root of
your repository.

```shell
export PYTHONPATH=${PYTHONPATH}:<LIBSVM_PATH>/python
```

To install xgboost

```shell
git clone --recursive https://github.com/dmlc/xgboost
cd xgboost; cp make/minimum.mk ./config.mk; make
cd python-package; python setup.py develop
```

To install keras (Version >= 2.0)

```shell
pip install keras tensorflow
```

If you'd like to use the old keras version, you can:

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
pip install Sphinx==1.5.3 sphinx-rtd-theme==0.2.4 numpydoc
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
In addition to the conversion tools in this package, TensorFlow and MXNet have their own conversion tools:

- [TensorFlow](https://pypi.python.org/pypi/tfcoreml)
- [MXNet](https://github.com/apache/incubator-mxnet/tree/master/tools/coreml)
