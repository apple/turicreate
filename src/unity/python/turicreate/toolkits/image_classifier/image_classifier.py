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
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from turicreate import config as _tc_config
from .._internal_utils import _mac_ver
from .. import _mxnet_utils
from .. import _pre_trained_models
from .. import _image_feature_extractor
from turicreate.toolkits._internal_utils import (_raise_error_if_not_sframe,
                                                 _numeric_param_check_range)

_DEFAULT_SOLVER_OPTIONS = {
'convergence_threshold': 1e-2,
'step_size': 1.0,
'lbfgs_memory_level': 11,
'max_iterations': 10}

def create(dataset, target, features=None,
    l2_penalty=0.01, l1_penalty=0.0,
    solver='auto', feature_rescaling=True,
    convergence_threshold = _DEFAULT_SOLVER_OPTIONS['convergence_threshold'],
    step_size = _DEFAULT_SOLVER_OPTIONS['step_size'],
    lbfgs_memory_level = _DEFAULT_SOLVER_OPTIONS['lbfgs_memory_level'],
    max_iterations = _DEFAULT_SOLVER_OPTIONS['max_iterations'],
    class_weights = None,
    validation_set = 'auto',
    verbose=True,
    seed=None):
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
        Uses a pretrained model to bootstrap an image classifier:

           - "resnet-50" : Uses a pretrained resnet model.
                           Exported Core ML model will be ~90M.

           - "squeezenet_v1.1" : Uses a pretrained squeezenet model.
                                 Exported Core ML model will be ~4.7M.

           - "VisionFeaturePrint_Screen": Uses an OS internal feature extractor.
                                          Only on available on iOS 12.0+,
                                          macOS 10.14+ and tvOS 12.0+.
                                          Exported Core ML model will be ~41K.

        Models are downloaded from the internet if not available locally. Once
        downloaded, the models are cached for future use.

    validation_set : SFrame, optional
        A dataset for monitoring the model's generalization performance.
        The format of this SFrame must be the same as the training set.
        By default this argument is set to 'auto' and a validation set is
        automatically sampled and used for progress printing. If
        validation_set is set to None, then no additional metrics
        are computed. The default value is 'auto'.

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

    batch_size : int, optional
        If you are getting memory errors, try decreasing this value. If you
        have a powerful computer, increasing this value may improve performance.

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

    # Check model parameter
    allowed_models = list(_pre_trained_models.MODELS.keys())
    if _mac_ver() >= (10,14):
        allowed_models.append('VisionFeaturePrint_Screen')
    _tkutl._check_categorical_option_type('model', model, allowed_models)

    # Check dataset parameter
    if len(dataset) == 0:
        raise _ToolkitError('Unable to train on empty dataset')
    if (feature is not None) and (feature not in dataset.column_names()):
        raise _ToolkitError("Image feature column '%s' does not exist" % feature)
    if target not in dataset.column_names():
        raise _ToolkitError("Target column '%s' does not exist" % target)

    if(batch_size < 1):
        raise ValueError("'batch_size' must be greater than or equal to 1")

    if not (isinstance(validation_set, _tc.SFrame) or validation_set == 'auto' or validation_set is None):
        raise TypeError("Unrecognized value for 'validation_set'.")

    if feature is None:
        feature = _tkutl._find_only_image_column(dataset)

    feature_extractor = _image_feature_extractor._create_feature_extractor(model)

    # Extract features
    extracted_features = _tc.SFrame({
        target: dataset[target],
        '__image_features__': feature_extractor.extract_features(dataset, feature, verbose=verbose, batch_size=batch_size),
        })
    if isinstance(validation_set, _tc.SFrame):
        extracted_features_validation = _tc.SFrame({
            target: validation_set[target],
            '__image_features__': feature_extractor.extract_features(validation_set, feature, verbose=verbose, batch_size=batch_size),
        })
    else:
        extracted_features_validation = validation_set

    # Train a classifier using the extracted features
    extracted_features[target] = dataset[target]
    lr_model = _tc.logistic_classifier.create(extracted_features,
                                              features=['__image_features__'],
                                              target=target,
                                              max_iterations=max_iterations,
                                              validation_set=extracted_features_validation,
                                              seed=seed,
                                              verbose=verbose)

    # set input image shape
    if model in _pre_trained_models.MODELS:
        input_image_shape = _pre_trained_models.MODELS[model].input_image_shape
    else:    # model == VisionFeaturePrint_Screen
        input_image_shape = (3, 299, 299)

    # Save the model
    state = {
        'classifier': lr_model,
        'model': model,
        'max_iterations': max_iterations,
        'feature_extractor': feature_extractor,
        'input_image_shape': input_image_shape,
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
        model_name = state['model']
        if model_name == "VisionFeaturePrint_Screen" and _mac_ver() < (10,14):
            raise ToolkitError("Can not load model on this operating system. This model uses VisionFeaturePrint_Screen, "
                               "which is only supported on macOS 10.14 and higher.")
        state['feature_extractor'] = _image_feature_extractor._create_feature_extractor(model_name)
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

    def _canonize_input(self, dataset):
        """
        Takes input and returns tuple of the input in canonical form (SFrame)
        along with an unpack callback function that can be applied to
        prediction results to "undo" the canonization.
        """
        unpack = lambda x: x
        if isinstance(dataset, _tc.SArray):
            dataset = _tc.SFrame({self.feature: dataset})
        elif isinstance(dataset, _tc.Image):
            dataset = _tc.SFrame({self.feature: [dataset]})
            unpack = lambda x: x[0]
        return dataset, unpack

    def predict(self, dataset, output_type='class', batch_size=64):
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
            If dataset is an SFrame, it must have columns with the same names as
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

        batch_size : int, optional
            If you are getting memory errors, try decreasing this value. If you
            have a powerful computer, increasing this value may improve performance.

        Returns
        -------
        out : SArray
            An SArray with model predictions. If `dataset` is a single image, the
            return value will be a single prediction.

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
        if(batch_size < 1):
            raise ValueError("'batch_size' must be greater than or equal to 1")

        dataset, unpack = self._canonize_input(dataset)

        extracted_features = self._extract_features(dataset, batch_size=batch_size)
        return unpack(self.classifier.predict(extracted_features, output_type=output_type))

    def classify(self, dataset, batch_size=64):
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

        batch_size : int, optional
            If you are getting memory errors, try decreasing this value. If you
            have a powerful computer, increasing this value may improve performance.

        Returns
        -------
        out : SFrame
            An SFrame with model predictions i.e class labels and
            probabilities. If `dataset` is a single image, the return will be a
            single row (dict).

        See Also
        ----------
        create, evaluate, predict

        Examples
        ----------
        >>> classes = model.classify(data)

        """
        if not isinstance(dataset, (_tc.SFrame, _tc.SArray, _tc.Image)):
            raise TypeError('dataset must be either an SFrame, SArray or turicreate.Image')
        if(batch_size < 1):
            raise ValueError("'batch_size' must be greater than or equal to 1")

        dataset, unpack = self._canonize_input(dataset)

        extracted_features = self._extract_features(dataset, batch_size=batch_size)
        return unpack(self.classifier.classify(extracted_features))

    def predict_topk(self, dataset, output_type="probability", k=3, batch_size=64):
        """
        Return top-k predictions for the ``dataset``, using the trained model.
        Predictions are returned as an SFrame with three columns: `id`,
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
        +----+-------+-------------------+
        | id | class |   probability     |
        +----+-------+-------------------+
        | 0  |   4   |   0.995623886585  |
        | 0  |   9   |  0.0038311756216  |
        | 0  |   7   | 0.000301006948575 |
        | 1  |   1   |   0.928708016872  |
        | 1  |   3   |  0.0440889261663  |
        | 1  |   2   |  0.0176190119237  |
        | 2  |   3   |   0.996967732906  |
        | 2  |   2   |  0.00151345680933 |
        | 2  |   7   | 0.000637513934635 |
        | 3  |   1   |   0.998070061207  |
        | .. |  ...  |        ...        |
        +----+-------+-------------------+
        [35688 rows x 3 columns]
        """
        if not isinstance(dataset, (_tc.SFrame, _tc.SArray, _tc.Image)):
            raise TypeError('dataset must be either an SFrame, SArray or turicreate.Image')
        if(batch_size < 1):
            raise ValueError("'batch_size' must be greater than or equal to 1")

        dataset, _ = self._canonize_input(dataset)

        extracted_features = self._extract_features(dataset)
        return self.classifier.predict_topk(extracted_features, output_type = output_type, k = k)

    def evaluate(self, dataset, metric='auto', verbose=True, batch_size=64):
        """
        Evaluate the model by making predictions of target values and comparing
        these to actual values.
        
        
# Regression model names.




   model_name = "classifier_image_regression"


   solver = solver.lower()







   model = _sl.create(dataset, target, model_name, features=features,


                       validation_set = validation_set, verbose = verbose,


                       l2_penalty=l2_penalty, l1_penalty = l1_penalty,


                       feature_rescaling = feature_rescaling,


                       convergence_threshold = convergence_threshold,


                       step_size = step_size,


                       solver = solver,


                       lbfgs_memory_level = lbfgs_memory_level,


                       max_iterations = max_iterations,


                       class_weights = class_weights,


                       seed=seed)







   return ImageClassifier(model.__proxy__)







class ImageClassifier(_Classifier):


   """


   Image regression models a discrete target variable as a function of


   several feature variables.






   The :class:`~turicreate.image_classifier.imageClassifier` uses


   a discrete target variable :math:`y` instead of a scalar. For each


   observation, the probability that :math:`y=1` (instead of 0) is modeled as


   the image function of a linear combination of the feature values.






   Given a set of features :math:`x_i`, and a label :math:`y_i \in \{0,1\}`,


   image regression interprets the probability that the label is in one class


   as a image function of a linear combination of the features.






       .. math::


         f_i(\\theta) =  p(y_i = 1 | x) = \\frac{1}{1 + \exp(-\\theta^T x)}






   An intercept term is added by appending a column of 1's to the features.


   Regularization is often required to prevent over fitting by penalizing


   models with extreme parameter values. The image regression module


   supports l1 and l2 regularization, which are added to the loss function.






   The composite objective being optimized for is the following;






       .. math::


          \min_{\\theta} \sum_{i = 1}^{n} f_i(\\theta) + \lambda_1 ||\\theta||_1 + \lambda_2 ||\\theta||^{2}_{2}






   where :math:`\lambda_1` is the ``l1_penalty`` and :math:`\lambda_2` is the


   ``l2_penalty``.






   For multi-class models, we perform multinomial image regression, which


   is an extension of the binary image regression model discussed above.






   This model cannot be constructed directly.  Instead, use


   :func:`turicreate.image_classifier.create` to create an instance of this


   model. A detailed list of parameter options and code samples are available


   in the documentation for the create function.






   Examples


   --------


   .. sourcecode:: python






       # Load the data (From an S3 bucket)


       >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')






       # Make sure the target is discrete


       >>> data['is_expensive'] = data['price'] > 30000






       # Make a image regression model


       >>> model = turicreate.image_classifier.create(data, target='is_expensive', features=['bath', 'bedroom', 'size'])






       # Extract the coefficients


       >>> coefficients = model.coefficients






       # Make predictions (as margins, probability, or class)


       >>> predictions = model.predict(data)


       >>> predictions = model.predict(data, output_type='probability')


       >>> predictions = model.predict(data, output_type='margin')






       # Evaluate the model


       >>> results = model.evaluate(data)






   See Also


   --------


   create










   """


   def __init__(self, model_proxy):







       self.__proxy__ = model_proxy


       self.__name__ = self.__class__._native_name()







   @classmethod


   def _native_name(cls):


       return "classifier_image_regression"







   def __str__(self):


       """


       Return a string description of the model to the ``print`` method.






       Returns


       -------


       out : string


           A description of the model.


       """


       return self.__repr__()







   def _get_summary_struct(self):


       """


       Returns a structured description of the model, including (where relevant)


       the schema of the training data, description of the training data,


       training statistics, and model hyperparameters.






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


           ('Number of coefficients', 'num_coefficients'),


           ('Number of examples', 'num_examples'),


           ('Number of classes', 'num_classes'),


           ('Number of feature columns', 'num_features'),


           ('Number of unpacked features', 'num_unpacked_features')]







       hyperparam_fields = [


           ("L1 penalty", 'l1_penalty'),


           ("L2 penalty", 'l2_penalty')


       ]







       solver_fields = [


           ("Solver", 'solver'),


           ("Solver iterations", 'training_iterations'),


           ("Solver status", 'training_solver_status'),


           ("Training time (sec)", 'training_time')]







       training_fields = [


           ("Log-likelihood", 'training_loss')]







       coefs = self.coefficients


       top_coefs, bottom_coefs = _toolkit_get_topk_bottomk(coefs,k=5)







       (coefs_list, titles_list) = _summarize_coefficients(top_coefs, \


                                                                   bottom_coefs)







       return ([ model_fields, hyperparam_fields, \


                       solver_fields, training_fields ] + coefs_list, \


                       [ 'Schema', 'Hyperparameters', \


                       'Training Summary', 'Settings' ] + titles_list )







   def __repr__(self):


       """


       Print a string description of the model, when the model name is entered


       in the terminal.


       """







       (sections, section_titles) = self._get_summary_struct()







       return _toolkit_repr_print(self, sections, section_titles, width=30)







   def export_coreml(self, filename):


       """


       Export the model in Core ML format.






       Parameters


       ----------


       filename: str


         A valid filename where the model can be saved.






       Examples


       --------


       >>> model.export_coreml("MyModel.mlmodel")


       """


       from turicreate.extensions import _image_classifier_export_as_model_asset


       from turicreate.toolkits import _coreml_utils


       display_name = "image classifier"


       short_description = _coreml_utils._mlmodel_short_description(display_name)


       context = {"class": self.__class__.__name__,


                  "version": _turicreate.__version__,


                  "short_description": short_description,


                  'user_defined':{


                   'turicreate_version': _turicreate.__version__


                  }


               }


       _image_classifier_export_as_model_asset(self.__proxy__, filename, context)







   def _get(self, field):


       """


       Return the value of a given field. The list of all queryable fields is


       detailed below, and can be obtained programmatically with the


       :func:`~turicreate.image_classifier.ImageClassifier._list_fields`


       method.






       +------------------------+-------------------------------------------------------------+


       |      Field             | Description                                                 |


       +========================+=============================================================+


       | coefficients           | Regression coefficients                                     |


       +------------------------+-------------------------------------------------------------+


       | convergence_threshold  | Desired solver accuracy                                     |


       +------------------------+-------------------------------------------------------------+


       | feature_rescaling      | Bool indicating l2-rescaling of features                    |


       +------------------------+-------------------------------------------------------------+


       | features               | Feature column names                                        |


       +------------------------+-------------------------------------------------------------+


       | l1_penalty             | l1 regularization weight                                    |


       +------------------------+-------------------------------------------------------------+


       | l2_penalty             | l2 regularization weight                                    |


       +------------------------+-------------------------------------------------------------+


       | lbfgs_memory_level     | LBFGS memory level                                          |


       +------------------------+-------------------------------------------------------------+


       | max_iterations         | Maximum number of solver iterations                         |


       +------------------------+-------------------------------------------------------------+


       | num_coefficients       | Number of coefficients in the model                         |


       +------------------------+-------------------------------------------------------------+


       | num_examples           | Number of examples used for training                        |


       +------------------------+-------------------------------------------------------------+


       | num_features           | Number of dataset columns used for training                 |


       +------------------------+-------------------------------------------------------------+


       | num_unpacked_features  | Number of features (including expanded list/dict features)  |


       +------------------------+-------------------------------------------------------------+


       | solver                 | Type of solver                                              |


       +------------------------+-------------------------------------------------------------+


       | step_size              | Initial step size for the solver                            |


       +------------------------+-------------------------------------------------------------+


       | target                 | Target column name                                          |


       +------------------------+-------------------------------------------------------------+


       | training_iterations    | Number of solver iterations                                 |


       +------------------------+-------------------------------------------------------------+


       | training_loss          | Maximized Log-likelihood                                    |


       +------------------------+-------------------------------------------------------------+


       | training_solver_status | Solver status after training                                |


       +------------------------+-------------------------------------------------------------+


       | training_time          | Training time (excludes preprocessing)                      |


       +------------------------+-------------------------------------------------------------+


       | unpacked_features      | Feature names (including expanded list/dict features)       |


       +------------------------+-------------------------------------------------------------+






       Parameters


       ----------


       field : string


           Name of the field to be retrieved.






       Returns


       -------


       out


           Value of the requested fields.


       """


       return super(_Classifier, self)._get(field)







   def predict(self, dataset, output_type='class',


               missing_value_action='auto'):


       """


       Return predictions for ``dataset``, using the trained image


       regression model. Predictions can be generated as class labels,


       probabilities that the target value is True, or margins (i.e. the


       distance of the observations from the hyperplane separating the


       classes). `probability_vector` returns a vector of probabilities by


       each class.






       For each new example in ``dataset``, the margin---also known as the


       linear predictor---is the inner product of the example and the model


       coefficients. The probability is obtained by passing the margin through


       the image function. Predicted classes are obtained by thresholding


       the predicted probabilities at 0.5. If you would like to threshold


       predictions at a different probability level, you can use the


       Turi Create evaluation toolkit.






       Parameters


       ----------


       dataset : SFrame


           Dataset of new observations. Must include columns with the same


           names as the features used for model training, but does not require


           a target column. Additional columns are ignored.






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






       missing_value_action : str, optional


           Action to perform when missing values are encountered. Can be


           one of:






           - 'auto': Default to 'impute'


           - 'impute': Proceed with evaluation by filling in the missing


             values with the mean of the training data. Missing


             values are also imputed if an entire column of data is


             missing during evaluation.


           - 'error': Do not proceed with evaluation and terminate with


             an error message.






       Returns


       -------


       out : SArray


           An SArray with model predictions.






       See Also


       ----------


       create, evaluate, classify






       Examples


       ----------


       >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')






       >>> data['is_expensive'] = data['price'] > 30000


       >>> model = turicreate.image_classifier.create(data,


                                            target='is_expensive',


                                            features=['bath', 'bedroom', 'size'])






       >>> probability_predictions = model.predict(data, output_type='probability')


       >>> margin_predictions = model.predict(data, output_type='margin')


       >>> class_predictions = model.predict(data, output_type='class')






       """







       return super(_Classifier, self).predict(dataset,


                                               output_type=output_type,


                                               missing_value_action=missing_value_action)







   def classify(self, dataset, missing_value_action='auto'):


       """


       Return a classification, for each example in the ``dataset``, using the


       trained image regression model. The output SFrame contains predictions


       as both class labels (0 or 1) as well as probabilities that the predicted


       value is the associated label.






       Parameters


       ----------


       dataset : SFrame


           Dataset of new observations. Must include columns with the same


           names as the features used for model training, but does not require


           a target column. Additional columns are ignored.






       missing_value_action : str, optional


           Action to perform when missing values are encountered. This can be


           one of:






           - 'auto': Default to 'impute'


           - 'impute': Proceed with evaluation by filling in the missing


             values with the mean of the training data. Missing


             values are also imputed if an entire column of data is


             missing during evaluation.


           - 'error': Do not proceed with evaluation and terminate with


             an error message.






       Returns


       -------


       out : SFrame


           An SFrame with model predictions i.e class labels and probabilities.






       See Also


       ----------


       create, evaluate, predict






       Examples


       ----------


       >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')






       >>> data['is_expensive'] = data['price'] > 30000


       >>> model = turicreate.image_classifier.create(data,


                                            target='is_expensive',


                                            features=['bath', 'bedroom', 'size'])






       >>> classes = model.classify(data)






       """







       return super(ImageClassifier, self).classify(dataset,


                                                       missing_value_action=missing_value_action)







   def predict_topk(self, dataset, output_type="probability", k=3, missing_value_action='auto'):


       """


       Return top-k predictions for the ``dataset``, using the trained model.


       Predictions are returned as an SFrame with three columns: `id`,


       `class`, and `probability`, `margin`,  or `rank`, depending on the ``output_type``


       parameter. Input dataset size must be the same as for training of the model.






       Parameters


       ----------


       dataset : SFrame


           A dataset that has the same columns that were used during training.


           If the target column exists in ``dataset`` it will be ignored


           while making predictions.






       output_type : {'probability', 'rank', 'margin'}, optional


           Choose the return type of the prediction:






           - `probability`: Probability associated with each label in the prediction.


           - `rank`       : Rank associated with each label in the prediction.


           - `margin`     : Margin associated with each label in the prediction.






       k : int, optional


           Number of classes to return for each input example.






       missing_value_action : str, optional


           Action to perform when missing values are encountered. Can be


           one of:






           - 'auto': Default to 'impute'


           - 'impute': Proceed with evaluation by filling in the missing


             values with the mean of the training data. Missing


             values are also imputed if an entire column of data is


             missing during evaluation.


           - 'error': Do not proceed with evaluation and terminate with


             an error message.






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


       | id     | class |   probability     |


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


       _check_categorical_option_type('output_type', output_type,


                                      ['rank', 'margin', 'probability'])


       _check_categorical_option_type('missing_value_action', missing_value_action,


                                      ['auto', 'impute', 'error'])


       if missing_value_action == 'auto':


           missing_value_action = 'impute'







       # Low latency path


       if isinstance(dataset, list):


           return self.__proxy__.fast_predict_topk(


               dataset, missing_value_action, output_type, k)


       if isinstance(dataset, dict):


           return self.__proxy__.fast_predict_topk(


               [dataset], missing_value_action, output_type, k)


       # Fast path


       _raise_error_if_not_sframe(dataset, "dataset")


       options = dict()


       if (missing_value_action == 'auto'):


           missing_value_action = _sl.select_default_missing_value_policy(


                                                             self, 'predict')


       return self.__proxy__.predict_topk(


           dataset, missing_value_action, output_type, k)







  


   def evaluate(self, dataset, metric='auto', missing_value_action='auto'):


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






       missing_value_action : str, optional


           Action to perform when missing values are encountered. This can be


           one of:






           - 'auto': Default to 'impute'


           - 'impute': Proceed with evaluation by filling in the missing


             values with the mean of the training data. Missing


             values are also imputed if an entire column of data is


             missing during evaluation.


           - 'error': Do not proceed with evaluation and terminate with


             an error message.






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






         >>> data =  turicreate.SFrame('https://static.turi.com/datasets/regression/houses.csv')


         >>> data['is_expensive'] = data['price'] > 30000


         >>> model = turicreate.image_classifier.create(data,


         ...                            target='is_expensive',


         ...                            features=['bath', 'bedroom', 'size'])


         >>> results = model.evaluate(data)


         >>> print results['accuracy']


       """







       _raise_error_evaluation_metric_is_valid(metric,


               ['auto', 'accuracy', 'confusion_matrix', 'roc_curve', 'auc',


                'log_loss', 'precision', 'recall', 'f1_score'])


       return super(_Classifier, self).evaluate(dataset,


                                missing_value_action=missing_value_action,


                                metric=metric)



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

        batch_size : int, optional
            If you are getting memory errors, try decreasing this value. If you
            have a powerful computer, increasing this value may improve performance.

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
        if(batch_size < 1):
            raise ValueError("'batch_size' must be greater than or equal to 1")

        extracted_features = self._extract_features(dataset, verbose=verbose, batch_size=batch_size)
        extracted_features[self.target] = dataset[self.target]
        return self.classifier.evaluate(extracted_features, metric = metric)

    def _extract_features(self, dataset, verbose=False, batch_size=64):
        return _tc.SFrame({
            '__image_features__': self.feature_extractor.extract_features(dataset, self.feature, verbose=verbose, batch_size=batch_size)
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
        import coremltools
        # First define three internal helper functions


        # Internal helper function
        def _create_vision_feature_print_screen():
            prob_name = self.target + 'Probability'

            #
            # Setup the top level (pipeline classifier) spec
            #
            top_spec = coremltools.proto.Model_pb2.Model()
            top_spec.specificationVersion = 3

            desc = top_spec.description
            desc.output.add().name = prob_name
            desc.output.add().name = self.target

            desc.predictedFeatureName = self.target
            desc.predictedProbabilitiesName = prob_name

            input = desc.input.add()
            input.name = self.feature
            input.type.imageType.width = 299
            input.type.imageType.height = 299
            BGR_VALUE = coremltools.proto.FeatureTypes_pb2.ImageFeatureType.ColorSpace.Value('BGR')
            input.type.imageType.colorSpace = BGR_VALUE

            #
            # VisionFeaturePrint extractor
            #
            pipelineClassifier = top_spec.pipelineClassifier
            scene_print = pipelineClassifier.pipeline.models.add()
            scene_print.specificationVersion = 3
            scene_print.visionFeaturePrint.scene.version = 1

            input = scene_print.description.input.add()
            input.name = self.feature
            input.type.imageType.width = 299
            input.type.imageType.height = 299
            input.type.imageType.colorSpace = BGR_VALUE

            output = scene_print.description.output.add()
            output.name = "output_name"
            DOUBLE_ARRAY_VALUE = coremltools.proto.FeatureTypes_pb2.ArrayFeatureType.ArrayDataType.Value('DOUBLE')
            output.type.multiArrayType.dataType = DOUBLE_ARRAY_VALUE
            output.type.multiArrayType.shape.append(2048)

            #
            # Neural Network Classifier, which is just logistic regression, in order to use GPUs
            #
            temp = top_spec.pipelineClassifier.pipeline.models.add()
            temp.specificationVersion = 3

            # Empty inner product layer
            nn_spec = temp.neuralNetworkClassifier
            feature_layer = nn_spec.layers.add()
            feature_layer.name = "feature_layer"
            feature_layer.input.append("output_name")
            feature_layer.output.append("softmax_input")
            fc_layer_params = feature_layer.innerProduct
            fc_layer_params.inputChannels = 2048

            # Softmax layer
            softmax = nn_spec.layers.add()
            softmax.name = "softmax"
            softmax.softmax.MergeFromString(b'')
            softmax.input.append("softmax_input")
            softmax.output.append(prob_name)

            input = temp.description.input.add()
            input.name = "output_name"
            input.type.multiArrayType.dataType = DOUBLE_ARRAY_VALUE
            input.type.multiArrayType.shape.append(2048)

            # Set outputs
            desc = temp.description
            prob_output = desc.output.add()
            prob_output.name = prob_name
            label_output = desc.output.add()
            label_output.name = self.target

            if type(self.classifier.classes[0]) == int:
                prob_output.type.dictionaryType.int64KeyType.MergeFromString(b'')
                label_output.type.int64Type.MergeFromString(b'')
            else:
                prob_output.type.dictionaryType.stringKeyType.MergeFromString(b'')
                label_output.type.stringType.MergeFromString(b'')

            temp.description.predictedFeatureName = self.target
            temp.description.predictedProbabilitiesName = prob_name

            return top_spec


        # Internal helper function
        def _update_last_two_layers(nn_spec):
            # Replace the softmax layer with new coeffients
            num_classes = self.num_classes
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

        # Internal helper function
        def _set_inputs_outputs_and_metadata(spec, nn_spec):
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

            prob_name = self.target + 'Probability'
            label_name = self.target
            old_output_name = nn_spec.layers[-1].name
            coremltools.models.utils.rename_feature(spec, 'classLabel', label_name)
            coremltools.models.utils.rename_feature(spec, old_output_name, prob_name)
            if nn_spec.layers[-1].name == old_output_name:
                nn_spec.layers[-1].name = prob_name
            if nn_spec.labelProbabilityLayerName == old_output_name:
                nn_spec.labelProbabilityLayerName = prob_name
            coremltools.models.utils.rename_feature(spec, 'data', self.feature)
            if len(nn_spec.preprocessing) > 0:
                nn_spec.preprocessing[0].featureName = self.feature

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

            return mlmodel


        # main part of the export_coreml function
        if self.model in _pre_trained_models.MODELS:
            ptModel = _pre_trained_models.MODELS[self.model]()
            feature_extractor = _image_feature_extractor.MXFeatureExtractor(ptModel)

            coreml_model = feature_extractor.get_coreml_model()
            spec = coreml_model.get_spec()
            nn_spec = spec.neuralNetworkClassifier
        else:     # model == VisionFeaturePrint_Screen
            spec = _create_vision_feature_print_screen()
            nn_spec = spec.pipelineClassifier.pipeline.models[1].neuralNetworkClassifier

        _update_last_two_layers(nn_spec)
        mlmodel = _set_inputs_outputs_and_metadata(spec, nn_spec)
        mlmodel.save(filename)


