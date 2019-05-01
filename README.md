Quick Links: [Installation](#supported-platforms) | [Documentation](#documentation) | [WWDC 2018 Talk](https://developer.apple.com/videos/play/wwdc2018/712/)

[![Build Status](https://travis-ci.com/apple/turicreate.svg?branch=master)](#)
[![PyPI Release](https://img.shields.io/pypi/v/turicreate.svg)](#)
[![Python Versions](https://img.shields.io/pypi/pyversions/turicreate.svg)](#)

[<img align="right" src="https://docs-assets.developer.apple.com/turicreate/turi-dog.svg" alt="Turi Create" width="100">](#)

# Turi Create 

[Click here to check out our talk at WWDC 2018!](https://developer.apple.com/videos/play/wwdc2018/712/)

Turi Create simplifies the development of custom machine learning models. You
don't have to be a machine learning expert to add recommendations, object
detection, image classification, image similarity or activity classification to
your app.

* **Easy-to-use:** Focus on tasks instead of algorithms
* **Visual:** Built-in, streaming visualizations to explore your data
* **Flexible:** Supports text, images, audio, video and sensor data
* **Fast and Scalable:** Work with large datasets on a single machine
* **Ready To Deploy:** Export models to Core ML for use in iOS, macOS, watchOS, and tvOS apps

With Turi Create, you can accomplish many common ML tasks:

| ML Task                 | Description                      |
|:------------------------:|:--------------------------------:|
| [Recommender](https://apple.github.io/turicreate/docs/userguide/recommender/)             | Personalize choices for users    |
| [Image Classification](https://apple.github.io/turicreate/docs/userguide/image_classifier/)    | Label images                     |
| [Drawing Classification](https://apple.github.io/turicreate/docs/userguide/drawing_classifier)  | Recognize Pencil/Touch Drawings and Gestures                     |
| [Sound Classification](https://apple.github.io/turicreate/docs/userguide/sound_classifier)  | Classify sounds                     |
| [Object Detection](https://apple.github.io/turicreate/docs/userguide/object_detection/)        | Recognize objects within images  |
| [Style Transfer](https://apple.github.io/turicreate/docs/userguide/style_transfer/)        | Stylize images |
| [Activity Classification](https://apple.github.io/turicreate/docs/userguide/activity_classifier/) | Detect an activity using sensors |
| [Image Similarity](https://apple.github.io/turicreate/docs/userguide/image_similarity/)        | Find similar images              |
| [Classifiers](https://apple.github.io/turicreate/docs/userguide/supervised-learning/classifier.html)             | Predict a label           |
| [Regression](https://apple.github.io/turicreate/docs/userguide/supervised-learning/regression.html)              | Predict numeric values           |
| [Clustering](https://apple.github.io/turicreate/docs/userguide/clustering/)              | Group similar datapoints together|
| [Text Classifier](https://apple.github.io/turicreate/docs/userguide/text_classifier/)         | Analyze sentiment of messages    |


Example: Image classifier with a few lines of code
--------------------------------------------------

If you want your app to recognize specific objects in images, you can build your own model with just a few lines of code:

```python
import turicreate as tc

# Load data 
data = tc.SFrame('photoLabel.sframe')

# Create a model
model = tc.image_classifier.create(data, target='photoLabel')

# Make predictions
predictions = model.predict(data)

# Export to Core ML
model.export_coreml('MyClassifier.mlmodel')
```
 
It's easy to use the resulting model in an [iOS application](https://developer.apple.com/documentation/vision/classifying_images_with_vision_and_core_ml):

<p align="center"><img src="https://docs-assets.developer.apple.com/published/a2c37bce1f/689f61a6-1087-4112-99d9-bbfb326e3138.png" alt="Turi Create" width="600"></p>

Supported Platforms
-------------------

Turi Create supports:

* macOS 10.12+
* Linux (with glibc 2.10+)
* Windows 10 (via WSL)

System Requirements
-------------------

Turi Create requires:

* Python 2.7, 3.5, 3.6
* x86\_64 architecture
* At least 4 GB of RAM

Installation
------------

For detailed instructions for different varieties of Linux see [LINUX\_INSTALL.md](LINUX_INSTALL.md).
For common installation issues see [INSTALL\_ISSUES.md](INSTALL_ISSUES.md).

We recommend using virtualenv to use, install, or build Turi Create. 

```shell
pip install virtualenv
```

The method for installing *Turi Create* follows the
[standard python package installation steps](https://packaging.python.org/installing/).
To create and activate a Python virtual environment called `venv` follow these steps:

```shell
# Create a Python virtual environment
cd ~
virtualenv venv

# Activate your virtual environment
source ~/venv/bin/activate
```
Alternatively, if you are using [Anaconda](https://www.anaconda.com/what-is-anaconda/), you may use its virtual environment:
```shell
conda create -n venv python=2.7 anaconda
source activate venv
```

To install `Turi Create` within your virtual environment:
```shell
(venv) pip install -U turicreate
```

Version 5.0 (New)
-----------------

Turi Create 5.0 includes:
* GPU Acceleration on Macs for:
  * Image Classification (macOS 10.13+)
  * Image Similarity (macOS 10.13+)
  * Object Detection (macOS 10.14+)
  * Activity Classification (macOS 10.14+)
* New Task: Style Transfer
* Recommender model deployment
* Vision Feature Print model deployment


Documentation
-------------

The package [User Guide](https://apple.github.io/turicreate/docs/userguide) and [API Docs](https://apple.github.io/turicreate/docs/api) contain
more details on how to use Turi Create.

GPU Support
-----------

Turi Create **does not require a GPU**, but certain models can be accelerated 9-13x when utilizing a GPU. 

Turi Create automatically utilizes Mac GPUs for the following tasks:
* Image Classification (macOS 10.13+)
* Image Similarity (macOS 10.13+)
* Object Detection (macOS 10.14+, discrete GPU only)
* Activity Classification (macOS 10.14+, discrete GPU only)

For linux GPU support, see [LinuxGPU.md](LinuxGPU.md).

Building From Source
---------------------

If you want to build Turi Create from source, see [BUILD.md](BUILD.md).

Contributing
------------

Prior to contributing, please review [CONTRIBUTING.md](CONTRIBUTING.md) and do
not provide any contributions unless you agree with the terms and conditions
set forth in [CONTRIBUTING.md](CONTRIBUTING.md).

We want the Turi Create community to be as welcoming and inclusive as possible, and have adopted a [Code of Conduct](CODE_OF_CONDUCT.md) that we expect all community members, including contributors, to read and observe.
