# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Class definition and utilities for the image classification toolkit.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import turicreate as _tc
import time as _time

from turicreate.toolkits._model import CustomModel as _CustomModel
import turicreate.toolkits._internal_utils as _tkutl
import turicreate.toolkits._private_utils as _pvtutl
from turicreate.toolkits import _coreml_utils
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._model import ExposeAttributesFromProxy as _ExposeAttributesFromProxy
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from turicreate import config as _tc_config
from .. import _mxnet_utils
from .. import _pre_trained_models
from .. import _image_feature_extractor
from turicreate.toolkits._internal_utils import (_raise_error_if_not_sframe,
                                                 _numeric_param_check_range)

def create(dataset, target, feature = None, model = 'resnet-50',
           max_iterations=10, verbose=True, seed=None):
    """
    Create a :class:`ImageClassifier` model.

    Parameters
    ----------
    dataset : SFrame
        Input data. The column named by the 'feature' parameter will be
        extracted for modeling.

    target : string, or int
        Name of the column containing the target variable. The values in this
        column must be of string or integer type. String target variables are
        automatically mapped to integers in the order in which they are provided.
        For example, a target variable with 'cat' and 'dog' as possible
        values is mapped to 0 and 1 respectively with 0 being the base class
        and 1 being the reference class. Use `model.classes` to retrieve
        the order in which the classes are mapped.

    feature : string, optional
        indicates that the SFrame has only column of Image type and that will
        Name of the column containing the input images. 'None' (the default)
        indicates the only image column in `dataset` should be used as the
        feature.

    model : string optional
        Uses a pretrained model to bootstrap an image classifier

           - "resnet-50" : Uses a pretrained resnet model.
           - "squeezenet_v1.1" : Uses a pretrained squeezenet model.

        Models are downloaded from the internet if not available locally. Once
        downloaded, the models are cached for future use.

    max_iterations : float, optional
        The maximum number of allowed passes through the data. More passes over
        the data can result in a more accurately trained model. Consider
        increasing this (the default value is 10) if the training accuracy is
        low and the *Grad-Norm* in the display is large.

    verbose : bool, optional
        If True, prints progress updates and model details.

    seed : int, optional
        Seed for random number generation. Set this value to ensure that the
        same model is created every time.

    Returns
    -------
    out : ImageClassifier
        A trained :class:`ImageClassifier` model.

    Examples
    --------
    .. sourcecode:: python

        >>> model = turicreate.image_classifier.create(data, target='is_expensive')

        # Make predictions (in various forms)
        >>> predictions = model.predict(data)      # predictions
        >>> predictions = model.classify(data)     # predictions with confidence
        >>> predictions = model.predict_topk(data) # Top-5 predictions (multiclass)

        # Evaluate the model with ground truth data
        >>> results = model.evaluate(data)

    See Also
    --------
    ImageClassifier
    """
    start_time = _time.time()

    # Check parameters
    _tkutl._check_categorical_option_type('model', model, _pre_trained_models.MODELS.keys())
    if len(dataset) == 0:
        raise _ToolkitError('Unable to train on empty dataset')
    if (feature is not None) and (feature not in dataset.column_names()):
        raise _ToolkitError("Image feature column '%s' does not exist" % feature)
    if target not in dataset.column_names():
        raise _ToolkitError("Target column '%s' does not exist" % target)

    if feature is None:
        feature = _tkutl._find_only_image_column(dataset)

    # Load pre-trained model & feature extractor
    ptModel = _pre_trained_models.MODELS[model]()
    feature_extractor = _image_feature_extractor.MXFeatureExtractor(ptModel)

    # Extract features
    extracted_features = _tc.SFrame({
        target: dataset[target],
        '__image_features__': feature_extractor.extract_features(dataset, feature, verbose=verbose),
        })

    # Train a classifier using the extracted features
    extracted_features[target] = dataset[target]
    lr_model = _tc.logistic_classifier.create(extracted_features,
                                              features=['__image_features__'],
                                              target=target,
                                              max_iterations=max_iterations,
                                              seed=seed,
                                              verbose=verbose)

    # Save the model
    state = {
        'classifier': lr_model,
        'model': model,
        'max_iterations': max_iterations,
        'feature_extractor': feature_extractor,
        'input_image_shape': ptModel.input_image_shape,
        'target': target,
        'feature': feature,
        'num_features': 1,
        'num_classes': lr_model.num_classes,
        'classes': lr_model.classes,
        'num_examples': lr_model.num_examples,
        'training_time': _time.time() - start_time,
        'training_loss': lr_model.training_loss,
    }
    return ImageClassifier(state)


class ImageClassifier(_CustomModel):
    """
    An trained model that is ready to use for classification, exported to
    CoreML, or for feature extraction.

    This model should not be constructed directly.
    """

    _PYTHON_IMAGE_CLASSIFIER_VERSION = 1

    def __init__(self, state):
        self.__proxy__ = _PythonProxy(state)

    @classmethod
    def _native_name(cls):
        return "image_classifier"

    def _get_version(self):
        return self._PYTHON_IMAGE_CLASSIFIER_VERSION

    def _get_native_state(self):
        """
        Save the model as a dictionary, which can be loaded with the
        :py:func:`~turicreate.load_model` method.
        """
        state = self.__proxy__.get_state()
        state['classifier'] = state['classifier'].__proxy__
        del state['feature_extractor']
        del state['classes']
        return state

    @classmethod
    def _load_version(cls, state, version):
        """
        A function to load a previously saved ImageClassifier
        instance.
        """
        _tkutl._model_version_check(version, cls._PYTHON_IMAGE_CLASSIFIER_VERSION)
        from turicreate.toolkits.classifier.logistic_classifier import LogisticClassifier
        state['classifier'] = LogisticClassifier(state['classifier'])
        state['classes'] = state['classifier'].classes

        # Load pre-trained model & feature extractor
        ptModel = _pre_trained_models.MODELS[state['model']]()
        feature_extractor = _image_feature_extractor.MXFeatureExtractor(ptModel)
        state['feature_extractor'] = feature_extractor
        state['input_image_shape'] = tuple([int(i) for i in state['input_image_shape']])
        return ImageClassifier(state)

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the ImageClassifier.
        """
        return self.__repr__()

    def __repr__(self):
        """
        Print a string description of the model when the model name is entered
        in the terminal.
        """

        width = 40
        key_str = "{:<{}}: {}"

        sections, section_titles = self._get_summary_struct()
        out = _tkutl._toolkit_repr_print(self, sections, section_titles,
                                         width=width)
        return out

    def _get_summary_struct(self):
        """
        Returns a structured description of the model, including (where
        relevant) the schema of the training data, description of the training
        data, training statistics, and model hyperparameters.

        Returns
        -------
        sections : list (of list of tuples)
            A list of summary sections.
              Each section is a list.
                Each item in a section list is a tuple of the form:
                  ('<label>','<field>')
        section_titles: list
            A list of section titles.
              The order matches that of the 'sections' object.
        """
        model_fields = [
            ('Number of classes', 'num_classes'),
            ('Number of feature columns', 'num_features'),
            ('Input image shape', 'input_image_shape'),
        ]
        training_fields = [
            ('Number of examples', 'num_examples'),
            ("Training loss", 'training_loss'),
            ("Training time (sec)", 'training_time'),
        ]

        section_titles = ['Schema', 'Training summary']
        return([model_fields, training_fields], section_titles)

    def predict(self, dataset, output_type='class'):
        """
        Return predictions for ``dataset``, using the trained logistic
        regression model. Predictions can be generated as class labels,
        probabilities that the target value is True, or margins (i.e. the
        distance of the observations from the hyperplane separating the
        classes). `probability_vector` returns a vector of probabilities by
        each class.

        For each new example in ``dataset``, the margin---also known as the
        linear predictor---is the inner product of the example and the model
        coefficients. The probability is obtained by passing the margin through
        the logistic function. Predicted classes are obtained by thresholding
        the predicted probabilities at 0.5. If you would like to threshold
        predictions at a different probability level, you can use the
        Turi Create evaluation toolkit.

        Parameters
        ----------
        dataset : SFrame | SArray | turicreate.Image
            The images to be classified.
            If dataset is an SFrame, it must have a columns with the same names as
            the features used for model training, but does not require a target
            column. Additional columns are ignored.

        output_type : {'probability', 'margin', 'class', 'probability_vector'}, optional
            Form of the predictions which are one of:

            - 'probability': Prediction probability associated with the True
              class (not applicable for multi-class classification)
            - 'probability_vector': Prediction probability associated with each
              class as a vector. The probability of the first class (sorted
              alphanumerically by name of the class in the training set) is in
              position 0 of the vector, the second in position 1 and so on.
            - 'class': Class prediction. For multi-class classification, this
              returns the class with maximum probability.

        Returns
        -------
        out : SArray
            An SArray with model predictions.

        See Also
        ----------
        create, evaluate, classify

        Examples
        ----------
        >>> probability_predictions = model.predict(data, output_type='probability')
        >>> margin_predictions = model.predict(data, output_type='margin')
        >>> class_predictions = model.predict(data, output_type='class')

        """
        if not isinstance(dataset, (_tc.SFrame, _tc.SArray, _tc.Image)):
            raise TypeError('dataset must be either an SFrame, SArray or turicreate.Image')

        if isinstance(dataset, _tc.SArray):
            dataset = _tc.SFrame({self.feature: dataset})
        elif isinstance(dataset, _tc.Image):
            dataset = _tc.SFrame({self.feature: [dataset]})

        extracted_features = self._extract_features(dataset)
        return self.classifier.predict(extracted_features, output_type=output_type)

    def classify(self, dataset):
        """
        Return a classification, for each example in the ``dataset``, using the
        trained logistic regression model. The output SFrame contains predictions
        as both class labels (0 or 1) as well as probabilities that the predicted
        value is the associated label.

        Parameters
        ----------
        dataset : SFrame | SArray | turicreate.Image
            Images to be classified.
            If dataset is an SFrame, it must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        Returns
        -------
        out : SFrame
            An SFrame with model predictions i.e class labels and probabilities.

        See Also
        ----------
        create, evaluate, predict

        Examples
        ----------
        >>> classes = model.classify(data)

        """
        if not isinstance(dataset, (_tc.SFrame, _tc.SArray, _tc.Image)):
            raise TypeError('dataset must be either an SFrame, SArray or turicreate.Image')

        if isinstance(dataset, _tc.SArray):
            dataset = _tc.SFrame({self.feature: dataset})
        elif isinstance(dataset, _tc.Image):
            dataset = _tc.SFrame({self.feature: [dataset]})

        extracted_features = self._extract_features(dataset)
        return self.classifier.classify(extracted_features)

    def predict_topk(self, dataset, output_type="probability", k=3):
        """
        Return top-k predictions for the ``dataset``, using the trained model.
        Predictions are returned as an SFrame with three columns: `row_id`,
        `class`, and `probability`, `margin`,  or `rank`, depending on the ``output_type``
        parameter. Input dataset size must be the same as for training of the model.

        Parameters
        ----------
        dataset : SFrame | SArray | turicreate.Image
            Images to be classified.
            If dataset is an SFrame, it must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        output_type : {'probability', 'rank', 'margin'}, optional
            Choose the return type of the prediction:

            - `probability`: Probability associated with each label in the prediction.
            - `rank`       : Rank associated with each label in the prediction.
            - `margin`     : Margin associated with each label in the prediction.

        k : int, optional
            Number of classes to return for each input example.

        Returns
        -------
        out : SFrame
            An SFrame with model predictions.

        See Also
        --------
        predict, classify, evaluate

        Examples
        --------
        >>> pred = m.predict_topk(validation_data, k=3)
        >>> pred
        +--------+-------+-------------------+
        | row_id | class |   probability     |
        +--------+-------+-------------------+
        |   0    |   4   |   0.995623886585  |
        |   0    |   9   |  0.0038311756216  |
        |   0    |   7   | 0.000301006948575 |
        |   1    |   1   |   0.928708016872  |
        |   1    |   3   |  0.0440889261663  |
        |   1    |   2   |  0.0176190119237  |
        |   2    |   3   |   0.996967732906  |
        |   2    |   2   |  0.00151345680933 |
        |   2    |   7   | 0.000637513934635 |
        |   3    |   1   |   0.998070061207  |
        |  ...   |  ...  |        ...        |
        +--------+-------+-------------------+
        [35688 rows x 3 columns]
        """
        if not isinstance(dataset, (_tc.SFrame, _tc.SArray, _tc.Image)):
            raise TypeError('dataset must be either an SFrame, SArray or turicreate.Image')

        if isinstance(dataset, _tc.SArray):
            dataset = _tc.SFrame({self.feature: dataset})
        elif isinstance(dataset, _tc.Image):
            dataset = _tc.SFrame({self.feature: [dataset]})

        extracted_features = self._extract_features(dataset)
        return self.classifier.predict_topk(extracted_features, output_type = output_type, k = k)

    def evaluate(self, dataset, metric='auto', verbose=True):
        """
        Evaluate the model by making predictions of target values and comparing
        these to actual values.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the target and features used for model training. Additional
            columns are ignored.

        metric : str, optional
            Name of the evaluation metric.  Possible values are:

            - 'auto'             : Returns all available metrics.
            - 'accuracy'         : Classification accuracy (micro average).
            - 'auc'              : Area under the ROC curve (macro average)
            - 'precision'        : Precision score (macro average)
            - 'recall'           : Recall score (macro average)
            - 'f1_score'         : F1 score (macro average)
            - 'log_loss'         : Log loss
            - 'confusion_matrix' : An SFrame with counts of possible prediction/true label combinations.
            - 'roc_curve'        : An SFrame containing information needed for an ROC curve

            For more flexibility in calculating evaluation metrics, use the
            :class:`~turicreate.evaluation` module.

        verbose : bool, optional
            If True, prints progress updates and model details.

        Returns
        -------
        out : dict
            Dictionary of evaluation results where the key is the name of the
            evaluation metric (e.g. `accuracy`) and the value is the evaluation
            score.

        See Also
        ----------
        create, predict, classify

        Examples
        ----------
        .. sourcecode:: python

          >>> results = model.evaluate(data)
          >>> print results['accuracy']
        """
        extracted_features = self._extract_features(dataset, verbose=verbose)
        extracted_features[self.target] = dataset[self.target]
        return self.classifier.evaluate(extracted_features, metric = metric)

    def _extract_features(self, dataset, verbose=False):
        return _tc.SFrame({
            '__image_features__': self.feature_extractor.extract_features(dataset, self.feature, verbose=verbose)
            })

    def export_coreml(self, filename):
        """
        Save the model in Core ML format.

        See Also
        --------
        save

        Examples
        --------
        >>> model.export_coreml('myModel.mlmodel')
        """
        coreml_model = self.feature_extractor.get_coreml_model()
        spec = coreml_model.get_spec()
        nn_spec = spec.neuralNetworkClassifier
        num_classes = self.num_classes

        # Replace the softmax layer with new coeffients
        fc_layer = nn_spec.layers[-2]
        fc_layer_params = fc_layer.innerProduct
        fc_layer_params.outputChannels = self.classifier.num_classes
        inputChannels = fc_layer_params.inputChannels
        fc_layer_params.hasBias = True

        coefs = self.classifier.coefficients
        weights = fc_layer_params.weights
        bias = fc_layer_params.bias
        del weights.floatValue[:]
        del bias.floatValue[:]

        import numpy as np
        W = np.array(coefs[coefs['index'] != None]['value'], ndmin = 2).reshape(
                                          inputChannels, num_classes - 1, order = 'F')
        b =  coefs[coefs['index'] == None]['value']
        Wa = np.hstack((np.zeros((inputChannels, 1)), W))
        weights.floatValue.extend(Wa.flatten(order = 'F'))
        bias.floatValue.extend([0.0] + list(b))

        # Replace the classifier with the new classes
        class_labels = self.classifier.classes

        probOutput = spec.description.output[0]
        classLabel = spec.description.output[1]
        probOutput.type.dictionaryType.MergeFromString(b'')
        if type(class_labels[0]) == int:
            nn_spec.ClearField('int64ClassLabels')
            probOutput.type.dictionaryType.int64KeyType.MergeFromString(b'')
            classLabel.type.int64Type.MergeFromString(b'')
            del nn_spec.int64ClassLabels.vector[:]
            for c in class_labels:
                nn_spec.int64ClassLabels.vector.append(c)
        else:
            nn_spec.ClearField('stringClassLabels')
            probOutput.type.dictionaryType.stringKeyType.MergeFromString(b'')
            classLabel.type.stringType.MergeFromString(b'')
            del nn_spec.stringClassLabels.vector[:]
            for c in class_labels:
                nn_spec.stringClassLabels.vector.append(c)

        import coremltools
        prob_name = self.target + 'Probability'
        label_name = self.target
        old_output_name = spec.neuralNetworkClassifier.layers[-1].name
        coremltools.models.utils.rename_feature(spec, 'classLabel', label_name)
        coremltools.models.utils.rename_feature(spec, old_output_name, prob_name)
        if spec.neuralNetworkClassifier.layers[-1].name == old_output_name:
            spec.neuralNetworkClassifier.layers[-1].name = prob_name
        if spec.neuralNetworkClassifier.labelProbabilityLayerName == old_output_name:
            spec.neuralNetworkClassifier.labelProbabilityLayerName = prob_name
        coremltools.models.utils.rename_feature(spec, 'data', self.feature)
        spec.neuralNetworkClassifier.preprocessing[0].featureName = self.feature

        mlmodel = coremltools.models.MLModel(spec)
        model_type = 'image classifier (%s)' % self.model
        mlmodel.short_description = _coreml_utils._mlmodel_short_description(model_type)
        mlmodel.input_description[self.feature] = u'Input image'
        mlmodel.output_description[prob_name] = 'Prediction probabilities'
        mlmodel.output_description[label_name] = 'Class label of top prediction'
        _coreml_utils._set_model_metadata(mlmodel, self.__class__.__name__, {
                'model': self.model,
                'target': self.target,
                'features': self.feature,
                'max_iterations': str(self.max_iterations),
            }, version=ImageClassifier._PYTHON_IMAGE_CLASSIFIER_VERSION)
        mlmodel.save(filename)
