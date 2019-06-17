# User Guide Overview

Turi Create simplifies the development of custom machine learning models. You
don't have to be a machine learning expert to add recommendations, object
detection, image classification, image similarity or activity classification to
your app.

* **Easy-to-use:** Focus on tasks instead of algorithms
* **Visual:** Built-in, streaming visualizations to explore your data
* **Flexible:** Supports text, images, audio, video and sensor data
* **Fast and Scalable:** Work with large datasets on a single machine
* **Ready To Deploy:** Export models to Core ML for use in iOS, macOS, watchOS, and tvOS apps

As you use Turi Create in your work, reference this guide to understand:

* Data ingestion and cleaning with **SFrames** (and their data-type-specific equivalents)
* Basics of predictive model development: algorithm- and application-based toolkits
* How to evaluate, visualize, and improve upon your model

If you haven’t already installed Turi Create, you can find instructions
[here](https://github.com/apple/turicreate).

## SFrame

SFrame is a scalable, tabular, column-mutable dataframe object. The data
in SFrame is stored column-wise, and is stored
on persistent storage (e.g. disk) to avoid being constrained by memory
size. Each column in an SFrame is a size-immutable SArray, but SFrames
are mutable in that columns can be added and subtracted with ease. An
SFrame essentially acts as an ordered dict of SArrays.

Currently, we support constructing an SFrame from the following data
formats: .csv (comma separated value) file, SFrame directory archive (A
directory where an Sframe was saved previously), general text file (with
csv parsing options; see read_csv()), Python dictionary,
pandas.DataFrame and JSON.

An SFrame can be constructed with data from your local file system, a
network file system mounted locally, HDFS, Amazon S3, or HTTP(S).

See [Working with data](sframe/README.md) for more guidance on
data structures and the [API
docs](https://apple.github.io/turicreate/docs/api/turicreate.data_structures.html)
for a more complete reference.

## Models
 
Turi Create offers a broad set of packaged application based toolkits as
well as algorithms for model creation.
 
### Start with the task you want to accomplish

Application-oriented toolkits in Turi Create offer default parameters, building
blocks and baseline models that help you get started quickly with their
dataset without sacrificing the ability to go back and customize models
later. Each incorporates automatic feature engineering and model
selection.

Using these toolkits, you can tackle a number of common scenarios:
* [Recommender systems](recommender/README.md)
* [Image classification](image_classifier/README.md)
* [Drawing classification](drawing_classifier/README.md)
* [Sound classification](sound_classifier/README.md)
* [Image similarity](image_similarity/README.md)
* [Object detection](object_detection/README.md)
* [One Shot object detection](one_shot_object_detection/README.md)
* [Style transfer](style_transfer/README.md)
* [Activity classifier](activity_classifier/README.md)
* [Text classifier](text_classifier/README.md)

### Machine learning essentials

Essential machine learning models, organized into algorithm-based
toolkits:

* [Classifiers](supervised-learning/classifier.md)
* [Regression](supervised-learning/regression.md)
* [Graph analytics](graph_analytics/README.md)
* [Clustering](clustering/README.md)
* [Nearest Neighbors](nearest_neighbors/nearest_neighbors.md)
* [Topic models](text/README.md)

Refer to the [Machine Learning API
documentation](https://apple.github.io/turicreate/docs/api/turicreate.toolkits.html)
for complete algorithms and toolkits.

### Deployment to Core ML

With Core ML, you can integrate machine learning models into your macOS,
iOS, watchOS, and tvOS app. Many models created  in Turi Create can be
exported for use in Core ML.

