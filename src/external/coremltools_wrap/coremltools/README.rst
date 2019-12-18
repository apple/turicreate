.. -*- mode: rst -*-

coremltools
===========

`Core ML <http://developer.apple.com/documentation/coreml>`_
is an Apple framework that allows developers to easily integrate
machine learning (ML) models into apps. Core ML is available on iOS, iPadOS,
watchOS, macOS, and tvOS. Core ML introduces a public file format (.mlmodel)
for a broad set of ML methods including deep neural networks (convolutional
and recurrent), tree ensembles (boosted trees, random forest, decision trees),
and generalized linear models. Core ML models can be directly integrated into
apps within Xcode.

:code:`coremltools` is a python package for creating, examining, and testing models in
the .mlmodel format. In particular, it can be used to:

- Convert trained models from popular machine learning tools into Core ML format
  (.mlmodel).
- Write models to Core ML format with a simple API.
- Making predictions using the Core ML framework (on select platforms) to
  verify conversion.

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

- Keras (1.2.2, 2.0.4+) with corresponding TensorFlow version
- XGBoost (0.7+)
- scikit-learn (0.17+)
- LIBSVM

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
