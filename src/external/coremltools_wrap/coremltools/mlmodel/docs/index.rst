##################################
Core ML Model Format Specification
##################################

This directory contains the protobuf message definitions
that comprise the Core ML model document (``.mlmodel``) format.

The top-level message is `Model`, which is defined in :file:`Model.proto`.
Other message types describe data structures, feature types,
feature engineering model types, and predictive model types.

.. toctree::
    :maxdepth: 3
    :hidden:

    sections/Model.rst
    sections/NeuralNetwork.rst
    sections/TextClassifier.rst
    sections/WordTagger.rst
    sections/VisionFeaturePrint.rst
    sections/TreeEnsembles.rst
    sections/GLM.rst
    sections/SVM.rst
    sections/FeatureEngineering.rst
    sections/CustomModel.rst
    sections/DataStructuresAndFeatureTypes.rst