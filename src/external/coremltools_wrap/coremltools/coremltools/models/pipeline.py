# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
Pipeline utils for this package.
"""
from .. import SPECIFICATION_VERSION
from ..proto import Model_pb2 as _Model_pb2
from . import _feature_management
from . import model as _model

from ._interface_management import set_regressor_interface_params
from ._interface_management import set_classifier_interface_params
from ._interface_management import set_transform_interface_params, set_training_features

class Pipeline(object):
    """ 
    A pipeline model that exposes a sequence of models as a single model, 
    It requires a set of inputs, a sequence of other models and a set of outputs. 
    
    This class is the base class for :py:class:`PipelineClassifier` and 
    :py:class:`PipelineRegressor`, which contain a sequence ending in a classifier 
    or regressor and themselves behave like a classifier or regressor.  This class
    may be used directly for a sequence of feature transformer objects.

    """

    def __init__(self, input_features, output_features, training_features=None):
        """
        Create a pipeline of models to be executed sequentially.

        Parameters
        ----------
        
        input_features: [list of 2-tuples]
            Name(s) of the input features, given as a list of `('name', datatype)`
            tuples.  The datatypes entry can be any of the data types defined in the 
            :py:mod:`models.datatypes` module.

        output_features: [list of features]
            Name(s) of the output features, given as a list of
            `('name',datatype)` tuples.  The datatypes entry can be any of the
            data types defined in the :py:mod:`models.datatypes` module.  All features
            must be either defined in the inputs or be produced by one of the 
            contained models. 

        """
        spec = _Model_pb2.Model()
        spec.specificationVersion = SPECIFICATION_VERSION
        
        # Access this to declare it as a pipeline
        spec.pipeline

        spec = set_transform_interface_params(spec, input_features, output_features, training_features)

        # Save the spec as a member variable.
        self.spec = spec

    def _validate_updatable_pipeline_on_add_model(self, spec):
        if spec.isUpdatable:
            raise ValueError("New sub-models cannot be added after the pipeline has been marked as updatable")

    def add_model(self, spec):
        """
        Add a protobuf spec or :py:class:`models.MLModel` instance to the pipeline. 

        All input features of this model must either match the input_features 
        of the pipeline, or match the outputs of a previous model. 

        Parameters
        ----------
        spec: [MLModel, Model_pb2]
            A protobuf spec or MLModel instance containing a model.
        """

        self._validate_updatable_pipeline_on_add_model(self.spec)

        if isinstance(spec, _model.MLModel):
            spec = spec._spec

        pipeline = self.spec.pipeline
        step_spec = pipeline.models.add()
        step_spec.CopyFrom(spec)

    def _validate_sub_models_and_make_updatable(self, pipeline, spec):

        num_models = len(pipeline.models)
        if num_models < 1:
            raise ValueError("Pipeline does not seem to have any models. It should be marked as updatable only after adding all sub-models.")

        for model in pipeline.models[:-1]:
            if model.isUpdatable:
                raise ValueError("Only the last model can be updatable in an updatable pipeline.")

        last_model = pipeline.models[num_models - 1]
        if not last_model.isUpdatable:
            raise ValueError("A pipeline can be made updatable only if the last model is updatable.")

        spec.isUpdatable = True

    def make_updatable(self):
        self._validate_sub_models_and_make_updatable(self.spec.pipeline, self.spec)

    def set_training_input(self, training_input):
        """
        Set the training inputs of the network spec.

        Parameters
        ----------
        training_input: [tuple]
            List of training input names and type of the network.
        """
        spec = self.spec
        set_training_features(spec, training_input)


class PipelineRegressor(Pipeline):
    """ 
    A pipeline model that exposes a sequence of models as a single model, 
    It requires a set of inputs, a sequence of other models and a set of outputs.
    In this case the pipeline itself behaves as a regression model by designating
    a real valued output feature as its 'predicted feature'.
    """

    def __init__(self, input_features, output_features, training_features=None):
        """
        Create a set of pipeline models given a set of model specs.  The final 
        output model must be a regression model. 

        Parameters
        ----------
        
        input_features: [list of 2-tuples]
            Name(s) of the input features, given as a list of `('name', datatype)`
            tuples.  The datatypes entry can be any of the data types defined in the 
            :py:mod:`models.datatypes` module.

        output_features: [list of features]
            Name(s) of the output features, given as a list of
            `('name',datatype)` tuples.  The datatypes entry can be any of the
            data types defined in the :py:mod:`models.datatypes` module.  All features
            must be either defined in the inputs or be produced by one of the
            contained models.

        """
        spec = _Model_pb2.Model()
        spec.specificationVersion = SPECIFICATION_VERSION
        
        # Access this to declare it as a pipeline
        spec.pipelineRegressor
        spec = set_regressor_interface_params(spec, input_features, output_features, training_features)

        # Save as a member variable
        self.spec = spec

    def add_model(self, spec):
        """
        Add a protobuf spec or :py:class:`models.MLModel` instance to the pipeline. 

        All input features of this model must either match the input_features 
        of the pipeline, or match the outputs of a previous model. 

        Parameters
        ----------
        spec: [MLModel, Model_pb2]
            A protobuf spec or MLModel instance containing a model.
        """

        super(PipelineRegressor, self)._validate_updatable_pipeline_on_add_model(self.spec)

        if isinstance(spec, _model.MLModel):
            spec = spec._spec

        pipeline = self.spec.pipelineRegressor.pipeline
        step_spec = pipeline.models.add()
        step_spec.CopyFrom(spec)

    def make_updatable(self):
        super(PipelineRegressor, self)._validate_sub_models_and_make_updatable(self.spec.pipelineRegressor.pipeline, self.spec)

    def set_training_input(self, training_input):
        """
        Set the training inputs of the network spec.

        Parameters
        ----------
        training_input: [tuple]
            List of training input names and type of the network.
        """
        spec = self.spec
        set_training_features(spec, training_input)

class PipelineClassifier(Pipeline):
    """ 
    A pipeline model that exposes a sequence of models as a single model, 
    It requires a set of inputs, a sequence of other models and a set of outputs.
    In this case the pipeline itself behaves as a classification model by designating
    a discrete categorical output feature as its 'predicted feature'.
    """

    def __init__(self, input_features, class_labels, output_features=None, training_features=None):
        """
        Create a set of pipeline models given a set of model specs.  The last 
        model in this list must be a classifier model. 

        Parameters
        ----------
        input_features: [list of 2-tuples]
            Name(s) of the input features, given as a list of `('name', datatype)`
            tuples.  The datatypes entry can be any of the data types defined in the 
            :py:mod:`models.datatypes` module.

        class_labels: [list]
            A list of string or integer class labels to use in making predictions. 
            This list must match the class labels in the model outputting the categorical
            predictedFeatureName

        output_features: [list]
            A string or a list of two strings specifying the names of the two 
            output features, the first being a class label corresponding 
            to the class with the highest predicted score, and the second being 
            a dictionary mapping each class to its score. If `output_features` 
            is a string, it specifies the predicted class label and the class 
            scores is set to the default value of `"classProbability."` 
 
        """

        output_features = _feature_management.process_or_validate_classifier_output_features(
                output_features, class_labels)

        spec = _Model_pb2.Model()
        spec.specificationVersion = SPECIFICATION_VERSION
        spec = set_classifier_interface_params(spec, input_features,
                class_labels, 'pipelineClassifier', output_features, training_features)

        # Access this to declare it as a pipeline
        spec.pipelineClassifier

        # Save as a member variable
        self.spec = spec

    def add_model(self, spec):
        """
        Add a protobuf spec or :py:class:`models.MLModel` instance to the pipeline. 

        All input features of this model must either match the input_features 
        of the pipeline, or match the outputs of a previous model. 

        Parameters
        ----------
        spec: [MLModel, Model_pb2]
            A protobuf spec or MLModel instance containing a model.
        """

        super(PipelineClassifier, self)._validate_updatable_pipeline_on_add_model(self.spec)

        if isinstance(spec, _model.MLModel):
            spec = spec._spec
        pipeline = self.spec.pipelineClassifier.pipeline
        step_spec = pipeline.models.add()
        step_spec.CopyFrom(spec)

    def make_updatable(self):
        super(PipelineClassifier, self)._validate_sub_models_and_make_updatable(self.spec.pipelineClassifier.pipeline, self.spec)

    def set_training_input(self, training_input):
        """
        Set the training inputs of the network spec.

        Parameters
        ----------
        training_input: [tuple]
            List of training input names and type of the network.
        """
        spec = self.spec
        set_training_features(spec, training_input)