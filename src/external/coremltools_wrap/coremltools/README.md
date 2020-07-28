[![Build Status](https://img.shields.io/gitlab/pipeline/zach_nation/coremltools/master)](#)
[![PyPI Release](https://img.shields.io/pypi/v/coremltools.svg)](#)
[![Python Versions](https://img.shields.io/pypi/pyversions/coremltools.svg)](#)

[Core ML Tools](https://coremltools.readme.io/docs)
=======================

Core ML is an Apple framework to integrate machine learning models into your
app. Core ML provides a unified representation for all models. Your app uses
Core ML APIs and user data to make predictions, and to fine-tune models, all on
the user’s device. Core ML optimizes on-device performance by leveraging the
CPU, GPU, and Neural Engine while minimizing its memory footprint and power
consumption. Running a model strictly on the user’s device removes any need for
a network connection, which helps keep the user’s data private and your app
responsive.

[Core ML tools](https://coremltools.readme.io/docs#what-is-coremltools) contains all supporting tools for [Core ML model
conversion](https://coremltools.readme.io/docs), editing and validation. This includes deep learning frameworks like
TensorFlow, PyTorch, Keras, Caffe as well as classical machine learning
frameworks like LIBSVM, scikit-learn, and XGBoost.

With coremltools, you can do the following:

- [Convert trained models](https://coremltools.readme.io/docs) from frameworks like TensorFlow and PyTorch to the
  Core ML format.
- Read, write, and optimize Core ML models.
- Verify conversion/creation (on macOS) by making predictions using Core ML.

To get the latest version of coremltools:

```shell
pip install coremltools==4.0b2
```

For the latest changes please see the [release notes](https://github.com/apple/coremltools/releases/).

# Documentation

* [User Guides and Examples](https://coremltools.readme.io/)
* [Core ML Specification](https://mlmodel.readme.io/)
* [API Reference](https://coremltools.readme.io/reference/convertersconvert)
