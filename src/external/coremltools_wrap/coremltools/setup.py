#!/usr/bin/env python
#
# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import imp
import os
from setuptools import setup, find_packages

# Get the coremltools version string
coremltools_dir = os.path.join(os.path.dirname(__file__), "coremltools")
version_module = imp.load_source(
    "coremltools.version", os.path.join(coremltools_dir, "version.py")
)
__version__ = version_module.__version__

README = os.path.join(os.getcwd(), "README.md")


long_description = """coremltools
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

More Information
----------------

- `coremltools user guide and examples <https://coremltools.readme.io/>`_
- `Core ML framework documentation <http://developer.apple.com/documentation/coreml>`_
- `Machine learning at Apple <https://developer.apple.com/machine-learning>`_

License
-------
Copyright (c) 2020, Apple Inc. All rights reserved.

Use of this source code is governed by the
`3-Clause BSD License <https://opensource.org/licenses/BSD-3-Clause>`_
that can be found in the LICENSE.txt file.
"""

setup(
    name="coremltools",
    version=__version__,
    description="Community Tools for Core ML",
    long_description=long_description,
    author="Apple Inc.",
    author_email="coremltools@apple.com",
    url="https://github.com/apple/coremltools",
    packages=find_packages(),
    package_data={
        "": ["LICENSE.txt", "README.md", "libcaffeconverter.so", "libcoremlpython.so"]
    },
    install_requires=[
        "numpy >= 1.14.5",
        "protobuf >= 3.1.0",
        "six>=1.10.0",
        "attr",
        "attrs",
        "sympy",
        "scipy",
        'enum34;python_version < "3.4"',
        "tqdm",
    ],
    entry_points={"console_scripts": ["coremlconverter = coremltools:_main"]},
    classifiers=[
        "Development Status :: 4 - Beta",
        "Intended Audience :: End Users/Desktop",
        "Intended Audience :: Developers",
        "Operating System :: MacOS :: MacOS X",
        "Operating System :: POSIX :: Linux",
        "Programming Language :: Python :: 2.7",
        "Programming Language :: Python :: 3.5",
        "Programming Language :: Python :: 3.6",
        "Programming Language :: Python :: 3.7",
        "Programming Language :: Python :: 3.8",
        "Topic :: Scientific/Engineering",
        "Topic :: Software Development",
    ],
    license="BSD",
)
