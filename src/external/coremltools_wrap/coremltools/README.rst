.. -*- mode: rst -*-

coremltools
===========

`Core ML <http://developer.apple.com/documentation/coreml>`_
is an Apple framework which allows developers to simply and easily integrate
machine learning (ML) models into apps running on Apple devices (including iOS,
watchOS, macOS, and tvOS).  Core ML introduces a public file format (.mlmodel)
for a broad set of ML methods including deep neural networks (both
convolutional and recurrent), tree ensembles with boosting, and generalized
linear models. Models in this format can be directly integrated into apps
through Xcode.

:code:`coremltools` is a python package for creating, examining, and testing models in
the .mlmodel format.  In particular, it can be used to:

- Convert existing models to .mlmodel format from popular machine learning tools including Keras, Caffe, scikit-learn, libsvm, and XGBoost.
- Express models in .mlmodel format through a simple API.
- Make predictions with an .mlmodel (on select platforms for testing purposes).

Installation
------------

The method for installing :code:`coremltools` follows the
`standard python package installation steps <https://packaging.python.org/installing/>`_.
Once you have set up a python environment, run::

    pip install -U coremltools

The package `documentation <https://apple.github.io/coremltools/>`_ contains
more details on how to use coremltools.

Dependencies
------------

:code:`coremltools` has the following dependencies:

- numpy (1.10.0+)
- protobuf (3.1.0+)

In addition, it has the following soft dependencies that are only needed when
you are converting models of these formats:

- Keras (1.2.2, 2.0.4+) with corresponding Tensorflow version
- Xgboost (0.7+)
- scikit-learn (0.17+)
- libSVM

More Information
----------------

- `Core ML framework documentation <http://developer.apple.com/documentation/coreml>`_
- `Core ML model specification <https://apple.github.io/coremltools/coremlspecification>`_
- `Machine learning at Apple <https://developer.apple.com/machine-learning>`_

License
-------
Copyright (c) 2018, Apple Inc. All rights reserved.

Use of this source code is governed by the
`3-Clause BSD License <https://opensource.org/licenses/BSD-3-Clause>`_
that can be found in the LICENSE.txt file.
