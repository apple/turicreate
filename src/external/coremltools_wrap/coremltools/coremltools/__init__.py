# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
Core ML is an Apple framework which allows developers to simply and easily integrate machine
learning (ML) models into apps running on Apple devices (including iOS, watchOS, macOS, and
tvOS). Core ML introduces a public file format (.mlmodel) for a broad set of ML methods
including deep neural networks (both convolutional and recurrent), tree ensembles with boosting,
and generalized linear models. Models in this format can be directly integrated into apps
through Xcode.

Core MLTools in a python package for creating, examining, and testing models in the .mlmodel
format. In particular, it can be used to:

* Convert existing models to .mlmodel format from popular machine learning tools including:
     Keras, Caffe, scikit-learn, libsvm, and XGBoost.
* Express models in .mlmodel format through a simple API.
* Make predictions with an .mlmodel (on select platforms for testing purposes).

For more information: http://developer.apple.com/documentation/coreml
"""

# This is the basic Core ML specification format understood by iOS 11.0
SPECIFICATION_VERSION = 1

# New versions for iOS 11.2 features. Models which use these features should have these
# versions, but models created from this coremltools which do not use the features can
# still have the basic version.
_MINIMUM_CUSTOM_LAYER_SPEC_VERSION = 2
_MINIMUM_FP16_SPEC_VERSION = 2

# New versions for iOS 12.0 features. Models which use these features should have these
# versions, but models created from this coremltools which do not use the features can
# still have the basic version.
_MINIMUM_CUSTOM_MODEL_SPEC_VERSION = 3
_MINIMUM_QUANTIZED_MODEL_SPEC_VERSION = 3
_MINIMUM_FLEXIBLE_SHAPES_SPEC_VERSION = 3

# expose sub packages as directories
from . import converters
from . import proto
from . import models
from .models import utils

from ._scripts.converter import _main
