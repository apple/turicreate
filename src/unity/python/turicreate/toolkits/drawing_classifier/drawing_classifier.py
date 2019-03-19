# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import turicreate as _tc
import numpy as _np
import time as _time
from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from turicreate.toolkits import evaluation as _evaluation
import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from .. import _mxnet_utils
from turicreate import extensions as _extensions
from .. import _pre_trained_models

BITMAP_WIDTH = 28
BITMAP_HEIGHT = 28
TRAIN_VALIDATION_SPLIT = .95

def _raise_error_if_not_drawing_classifier_input_sframe(
    dataset, feature, target):
    """
    Performs some sanity checks on the SFrame provided as input to 
    `turicreate.drawing_classifier.create` and raises a ToolkitError
    if something in the dataset is missing or wrong.
    """
    from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe
    _raise_error_if_not_sframe(dataset)
    if feature not in dataset.column_names():
        raise _ToolkitError("Feature column '%s' does not exist" % feature)
    if target not in dataset.column_names():
        raise _ToolkitError("Target column '%s' does not exist" % target)
    if (dataset[feature].dtype != _tc.Image and dataset[feature].dtype != list):
        raise _ToolkitError("Feature column must contain images" 
            + " or stroke-based drawings encoded as lists of strokes" 
            + " where each stroke is a list of points and" 
            + " each point is stored as a dictionary")
    if dataset[target].dtype != int and dataset[target].dtype != str:
        raise _ToolkitError("Target column contains " + str(dataset[target].dtype)
            + " but it must contain strings or integers to represent" 
            + " labels for drawings.")
    if len(dataset) == 0:
        raise _ToolkitError("Input Dataset is empty!")

def create(input_dataset, target, feature=None, validation_set='auto',
            warm_start='auto', batch_size=256, 
            max_iterations=100, verbose=True):
    """
    Create a :class:`DrawingClassifier` model.

    Parameters
    ----------
    dataset : SFrame
        Input data. The columns named by the ``feature`` and ``target``
        parameters will be extracted for training the drawing classifier.

    target : string
        Name of the column containing the target variable. The values in this
        column must be of string or integer type.

    feature : string optional
        Name of the column containing the input drawings. 'None' (the default)
        indicates the column in `dataset` named "drawing" should be used as the
        feature.
        The feature column can contain both bitmap-based drawings as well as
        stroke-based drawings. Bitmap-based drawing input can be a grayscale
        tc.Image of any size.
        Stroke-based drawing input must be in the following format:
        Every drawing must be represented by a list of strokes, where each
        stroke must be a list of points in the order in which they were drawn
        on the canvas.
        Each point must be a dictionary with two keys, "x" and "y", and their
        respective values must be numerical, i.e. either integer or float.

    validation_set : SFrame optional
        A dataset for monitoring the model's generalization performance.
        The format of this SFrame must be the same as the training set.
        By default this argument is set to 'auto' and a validation set is
        automatically sampled and used for progress printing. If
        validation_set is set to None, then no additional metrics
        are computed. The default value is 'auto'.

    warm_start : string optional
        A string to denote which pretrained model to use. Set to "auto"
        by default which uses a model trained on 245 of the 345 classes in the
        Quick, Draw! dataset. Here is a list of all the pretrained models that
        can be passed in as this argument:
        "auto": Uses quickdraw_245_v0
        "quickdraw_245_v0": Uses a model trained on 245 of the 345 classes in the
                         Quick, Draw! dataset.

    batch_size: int optional
        The number of drawings per training step. If not set, a default
        value of 256 will be used. If you are getting memory errors,
        try decreasing this value. If you have a powerful computer, increasing
        this value may improve performance.

    max_iterations : int optional
        The maximum number of allowed passes through the data. More passes over
        the data can result in a more accurately trained model. 

    verbose : bool optional
        If True, print progress updates and model details.

    Returns
    -------
    out : DrawingClassifier
        A trained :class:`DrawingClassifier` model.

    See Also
    --------
    DrawingClassifier

    Examples
    --------
    .. sourcecode:: python

        # Train a drawing classifier model
        >>> model = turicreate.drawing_classifier.create(data)

        # Make predictions on the training set and as column to the SFrame
        >>> data['predictions'] = model.predict(data)

    """
    
    import mxnet as _mx
    from mxnet import autograd as _autograd
    from ._model_architecture import Model as _Model
    from ._sframe_loader import SFrameClassifierIter as _SFrameClassifierIter
    
    start_time = _time.time()

    # @TODO: Should be able to automatically choose number of iterations
    # based on data size: Tracked in Github Issue #1576

    # automatically infer feature column
    if feature is None:
        feature = _tkutl._find_only_drawing_column(input_dataset)

    _raise_error_if_not_drawing_classifier_input_sframe(
        input_dataset, feature, target)

    is_stroke_input = (input_dataset[feature].dtype != _tc.Image)
    dataset = _extensions._drawing_classifier_prepare_data(
        input_dataset, feature) if is_stroke_input else input_dataset

    iteration = 0

    classes = dataset[target].unique()
    classes = sorted(classes)
    class_to_index = {name: index for index, name in enumerate(classes)}

    validation_set_corrective_string = ("'validation_set' parameter must be "
        + "an SFrame, or None, or must be set to 'auto' for the toolkit to " 
        + "automatically create a validation set.")
    if isinstance(validation_set, _tc.SFrame):
        _raise_error_if_not_drawing_classifier_input_sframe(
            validation_set, feature, target)
        is_validation_stroke_input = (validation_set[feature].dtype != _tc.Image)
        validation_dataset = _extensions._drawing_classifier_prepare_data(
            validation_set, feature) if is_validation_stroke_input else validation_set
    elif isinstance(validation_set, str):
        if validation_set == 'auto':
            if dataset.num_rows() >= 100:
                if verbose:
                    print ( "PROGRESS: Creating a validation set from 5 percent of training data. This may take a while.\n"
                            "          You can set ``validation_set=None`` to disable validation tracking.\n")
                dataset, validation_dataset = dataset.random_split(
                    TRAIN_VALIDATION_SPLIT)
            else:
                validation_set = None
                validation_dataset = _tc.SFrame()
        else:
            raise _ToolkitError("Unrecognized value for 'validation_set'. "
                + validation_set_corrective_string)
    elif validation_set is None:
        validation_dataset = _tc.SFrame()
    else:
        raise TypeError("Unrecognized type for 'validation_set'."
            + validation_set_corrective_string)

    train_loader = _SFrameClassifierIter(dataset, batch_size,
                 feature_column=feature,
                 target_column=target,
                 class_to_index=class_to_index,
                 load_labels=True,
                 shuffle=True,
                 iterations=max_iterations)
    train_loader_to_compute_accuracy = _SFrameClassifierIter(dataset, batch_size,
                 feature_column=feature,
                 target_column=target,
                 class_to_index=class_to_index,
                 load_labels=True,
                 shuffle=True,
                 iterations=1)
    validation_loader = _SFrameClassifierIter(validation_dataset, batch_size,
                 feature_column=feature,
                 target_column=target,
                 class_to_index=class_to_index,
                 load_labels=True,
                 shuffle=True,
                 iterations=1)
    if verbose and iteration == 0:
        column_names = ['iteration', 'train_loss', 'train_accuracy', 'time']
        column_titles = ['Iteration', 'Training Loss', 'Training Accuracy', 'Elapsed Time (seconds)']
        if validation_set is not None:
            column_names.insert(3, 'validation_accuracy')
            column_titles.insert(3, 'Validation Accuracy')
        table_printer = _tc.util._ProgressTablePrinter(
            column_names, column_titles)

    ctx = _mxnet_utils.get_mxnet_context(max_devices=batch_size)
    model = _Model(num_classes = len(classes), prefix="drawing_")
    model_params = model.collect_params()
    model_params.initialize(_mx.init.Xavier(), ctx=ctx)

    if warm_start is not None:
        pretrained_model = _pre_trained_models.DrawingClassifierPreTrainedModel(
            warm_start)
        pretrained_model_params_path = pretrained_model.get_model_path()
        model.load_params(pretrained_model_params_path, 
            ctx=ctx, 
            allow_missing=True)
    softmax_cross_entropy = _mx.gluon.loss.SoftmaxCrossEntropyLoss()
    model.hybridize()
    trainer = _mx.gluon.Trainer(model.collect_params(), 'adam')

    train_accuracy = _mx.metric.Accuracy()
    validation_accuracy = _mx.metric.Accuracy()

    def get_data_and_label_from_batch(batch):
        if batch.pad is not None:
            size = batch_size - batch.pad
            batch_data  = (
                [_mx.nd.slice_axis(batch.data[0], axis=0, begin=0, end=size)] 
                + [None] * (len(ctx)-1)
                )
            batch_label = (
                [_mx.nd.slice_axis(batch.label[0], axis=0, begin=0, end=size)] 
                + [None] * (len(ctx)-1)
                )
        else:
            batch_data = _mx.gluon.utils.split_and_load(batch.data[0], ctx_list=ctx, batch_axis=0)
            batch_label = _mx.gluon.utils.split_and_load(batch.label[0], ctx_list=ctx, batch_axis=0)
        return batch_data, batch_label

    def compute_accuracy(accuracy_metric, batch_loader):
        batch_loader.reset()
        accuracy_metric.reset()
        for batch in batch_loader:
            batch_data, batch_label = get_data_and_label_from_batch(batch)
            outputs = []
            for x, y in zip(batch_data, batch_label):
                if x is None or y is None: continue
                z = model(x)
                outputs.append(z)
            accuracy_metric.update(batch_label, outputs)

    for train_batch in train_loader:
        train_batch_data, train_batch_label = get_data_and_label_from_batch(train_batch)
        with _autograd.record():
            # Inside training scope
            for x, y in zip(train_batch_data, train_batch_label):
                z = model(x)
                # Computes softmax cross entropy loss.
                loss = softmax_cross_entropy(z, y)
                # Backpropagate the error for one iteration.
                loss.backward()

        # Make one step of parameter update. Trainer needs to know the
        # batch size of data to normalize the gradient by 1/batch_size.
        trainer.step(train_batch.data[0].shape[0])
        # calculate training metrics
        train_loss = loss.mean().asscalar()
        train_time = _time.time() - start_time

        if train_batch.iteration > iteration:
            # Compute training accuracy
            compute_accuracy(train_accuracy, train_loader_to_compute_accuracy)
            # Compute validation accuracy
            if validation_set is not None:
                compute_accuracy(validation_accuracy, validation_loader)
            iteration = train_batch.iteration
            if verbose:
                kwargs = {  "iteration": iteration,
                            "train_loss": float(train_loss),
                            "train_accuracy": train_accuracy.get()[1],
                            "time": train_time}
                if validation_set is not None:
                    kwargs["validation_accuracy"] = validation_accuracy.get()[1]
                table_printer.print_row(**kwargs)

    state = {
        '_model': model,
        '_class_to_index': class_to_index,
        'num_classes': len(classes),
        'classes': classes,
        'input_image_shape': (1, BITMAP_WIDTH, BITMAP_HEIGHT),
        'batch_size': batch_size,
        'training_loss': train_loss,
        'training_accuracy': train_accuracy.get()[1],
        'training_time': train_time,
        'validation_accuracy': validation_accuracy.get()[1], 
        # nan if validation_set=None
        'max_iterations': max_iterations,
        'target': target,
        'feature': feature,
        'num_examples': len(input_dataset)
    }
    return DrawingClassifier(state)

class DrawingClassifier(_CustomModel):
    """
    A trained model that is ready to use for classification, and to be 
    exported to Core ML.

    This model should not be constructed directly.
    """

    _PYTHON_DRAWING_CLASSIFIER_VERSION = 1
    def __init__(self, state):
        self.__proxy__ = _PythonProxy(state)

    @classmethod
    def _native_name(cls):
        return "drawing_classifier"

    def _get_native_state(self):
        state = self.__proxy__.get_state()
        mxnet_params = state['_model'].collect_params()
        state['_model'] = _mxnet_utils.get_gluon_net_params_state(mxnet_params)
        return state

    def _get_version(self):
        return self._PYTHON_DRAWING_CLASSIFIER_VERSION

    @classmethod
    def _load_version(cls, state, version):
        _tkutl._model_version_check(version, 
            cls._PYTHON_DRAWING_CLASSIFIER_VERSION)
        from ._model_architecture import Model as _Model
        net = _Model(num_classes = len(state['classes']), prefix = 'drawing_')
        ctx = _mxnet_utils.get_mxnet_context(max_devices=state['batch_size'])
        net_params = net.collect_params()
        _mxnet_utils.load_net_params_from_state(
            net_params, state['_model'], ctx=ctx 
            )
        state['_model'] = net
        # For a model trained on integer classes, when saved and loaded back,
        # the classes are loaded as floats. The following if statement casts
        # the loaded "float" classes back to int.
        if len(state['classes']) > 0 and isinstance(state['classes'][0], float):
            state['classes'] = list(map(int, state['classes']))
        return DrawingClassifier(state)

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the DrawingClassifier.
        """
        return self.__repr__()

    def __repr__(self):
        """
        Returns a string description of the model when the model name is 
        entered in the terminal.
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
            ('Feature column', 'feature'),
            ('Target column', 'target')
        ]
        training_fields = [
            ('Training Iterations', 'max_iterations'),
            ('Training Accuracy', 'training_accuracy'),
            ('Validation Accuracy', 'validation_accuracy'),
            ('Training Time', 'training_time'),
            ('Number of Examples', 'num_examples'),
            ('Batch Size', 'batch_size'),
            ('Final Loss (specific to model)', 'training_loss')
        ]

        section_titles = ['Schema', 'Training summary']
        return([model_fields, training_fields], section_titles)

    def export_coreml(self, filename, verbose=False):
        """
        Save the model in Core ML format. The Core ML model takes a grayscale 
        drawing of fixed size as input and produces two outputs: 
        `classLabel` and `labelProbabilities`.

        The first one, `classLabel` is an integer or string (depending on the
        classes the model was trained on) to store the label of the top 
        prediction by the model.

        The second one, `labelProbabilities`, is a dictionary with all the 
        class labels in the dataset as the keys, and their respective 
        probabilities as the values.

        See Also
        --------
        save

        Parameters
        ----------
        filename : string
            The path of the file where we want to save the Core ML model.

        verbose : bool optional
            If True, prints export progress.


        Examples
        --------
        >>> model.export_coreml('drawing_classifier.mlmodel')
        """
        import mxnet as _mx
        from .._mxnet_to_coreml import _mxnet_converter
        import coremltools as _coremltools

        batch_size = 1
        image_shape = (batch_size,) + (1, BITMAP_WIDTH, BITMAP_HEIGHT)
        s_image = _mx.sym.Variable(self.feature,
            shape=image_shape, dtype=_np.float32)

        from copy import copy as _copy
        net = _copy(self._model)
        s_ymap = net(s_image)
        
        mod = _mx.mod.Module(symbol=s_ymap, label_names=None, data_names=[self.feature])
        mod.bind(for_training=False, data_shapes=[(self.feature, image_shape)])
        mod.init_params()
        
        arg_params, aux_params = mod.get_params()
        net_params = net.collect_params()

        new_arg_params = {}
        for k, param in arg_params.items():
            new_arg_params[k] = net_params[k].data(net_params[k].list_ctx()[0])
        new_aux_params = {}
        for k, param in aux_params.items():
            new_aux_params[k] = net_params[k].data(net_params[k].list_ctx()[0])
        mod.set_params(new_arg_params, new_aux_params)

        coreml_model = _mxnet_converter.convert(mod, mode='classifier',
                                class_labels=self.classes,
                                input_shape=[(self.feature, image_shape)],
                                builder=None, verbose=verbose,
                                preprocessor_args={
                                    'image_input_names': [self.feature],
                                    'image_scale': 1.0/255
                                })

        DESIRED_OUTPUT_NAME = self.target + "Probabilities"
        spec = coreml_model._spec
        class_label_output_index = 0 if spec.description.output[0].name == "classLabel" else 1
        probabilities_output_index = 1-class_label_output_index
        spec.neuralNetworkClassifier.labelProbabilityLayerName = DESIRED_OUTPUT_NAME
        spec.neuralNetworkClassifier.layers[-1].name = DESIRED_OUTPUT_NAME
        spec.neuralNetworkClassifier.layers[-1].output[0] = DESIRED_OUTPUT_NAME
        spec.description.predictedProbabilitiesName = DESIRED_OUTPUT_NAME
        spec.description.output[probabilities_output_index].name = DESIRED_OUTPUT_NAME
        from turicreate.toolkits import _coreml_utils
        model_type = "drawing classifier"
        spec.description.metadata.shortDescription = _coreml_utils._mlmodel_short_description(model_type)
        spec.description.input[0].shortDescription = self.feature
        spec.description.output[probabilities_output_index].shortDescription = 'Prediction probabilities'
        spec.description.output[class_label_output_index].shortDescription = 'Class Label of Top Prediction'
        from coremltools.models.utils import save_spec as _save_spec
        _save_spec(spec, filename)


    def _predict_with_probabilities(self, input_dataset, batch_size=None, 
        verbose=True):
        """
        Predict with probabilities. The core prediction part that both 
        `evaluate` and `predict` share.

        Returns an SFrame with two columns, self.target, and "probabilities".

        The column with column name, self.target, contains the predictions made
        by the model for the provided dataset.

        The "probabilities" column contains the probabilities for each class 
        that the model predicted for the data provided to the function.
        """

        import mxnet as _mx
        from ._sframe_loader import SFrameClassifierIter as _SFrameClassifierIter

        is_stroke_input = (input_dataset[self.feature].dtype != _tc.Image)
        dataset = _extensions._drawing_classifier_prepare_data(
                input_dataset, self.feature) if is_stroke_input else input_dataset
    
        batch_size = self.batch_size if batch_size is None else batch_size
        loader = _SFrameClassifierIter(dataset, batch_size,
                    class_to_index=self._class_to_index,
                    feature_column=self.feature,
                    target_column=self.target,
                    load_labels=False,
                    shuffle=False,
                    iterations=1)

        dataset_size = len(dataset)
        ctx = _mxnet_utils.get_mxnet_context()
        
        all_predicted = ['']*dataset_size
        all_probabilities = _np.zeros((dataset_size, len(self.classes)), 
            dtype=float)

        index = 0
        last_time = 0
        done = False
        for batch in loader:
            if batch.pad is not None:
                size = batch_size - batch.pad
                batch_data = _mx.nd.slice_axis(batch.data[0], 
                    axis=0, begin=0, end=size)
            else:
                batch_data = batch.data[0]
                size = batch_size

            if batch_data.shape[0] < len(ctx):
                ctx0 = ctx[:batch_data.shape[0]]
            else:
                ctx0 = ctx

            z = self._model(batch_data).asnumpy()
            predicted = z.argmax(axis=1)
            classes = self.classes
            
            predicted_sa = _tc.SArray(predicted).apply(lambda x: classes[x])
            
            all_predicted[index : index + len(predicted_sa)] = predicted_sa
            all_probabilities[index : index + z.shape[0]] = z
            index += z.shape[0]
            if index == dataset_size - 1:
                done = True

            cur_time = _time.time()
            # Do not print process if only a few samples are predicted
            if verbose and (dataset_size >= 5 
                and cur_time > last_time + 10 or done):
                print('Predicting {cur_n:{width}d}/{max_n:{width}d}'.format(
                    cur_n = index,
                    max_n = dataset_size,
                    width = len(str(dataset_size))))
                last_time = cur_time
        
        return (_tc.SFrame({self.target: _tc.SArray(all_predicted),
            'probability': _tc.SArray(all_probabilities)}))

    def evaluate(self, dataset, metric='auto', batch_size=None, verbose=True):
        """
        Evaluate the model by making predictions of target values and comparing
        these to actual values.
        
        Parameters
        ----------
        dataset : SFrame
        Dataset of new observations. Must include columns with the same
        names as the feature and target columns used for model training.
        Additional columns are ignored.
        
        metric : str optional
        Name of the evaluation metric. Possible values are:
        
        - 'auto'             : Returns all available metrics.
        - 'accuracy'         : Classification accuracy (micro average).
        - 'auc'              : Area under the ROC curve (macro average)
        - 'precision'        : Precision score (macro average)
        - 'recall'           : Recall score (macro average)
        - 'f1_score'         : F1 score (macro average)
        - 'confusion_matrix' : An SFrame with counts of possible 
                               prediction/true label combinations.
        - 'roc_curve'        : An SFrame containing information needed for an
                               ROC curve
        
        verbose : bool optional
        If True, prints prediction progress.

        Returns
        -------
        out : dict
        Dictionary of evaluation results where the key is the name of the
        evaluation metric (e.g. `accuracy`) and the value is the evaluation
        score.
        
        See Also
        ----------
        create, predict
        
        Examples
        ----------
        .. sourcecode:: python
        
        >>> results = model.evaluate(data)
        >>> print(results['accuracy'])
        """

        if self.target not in dataset.column_names():
            raise _ToolkitError("Dataset provided to evaluate does not have " 
                + "ground truth in the " + self.target + " column.")

        predicted = self._predict_with_probabilities(dataset, batch_size, verbose)

        avail_metrics = ['accuracy', 'auc', 'precision', 'recall',
                         'f1_score', 'confusion_matrix', 'roc_curve']

        _tkutl._check_categorical_option_type(
                        'metric', metric, avail_metrics + ['auto'])

        metrics = avail_metrics if metric == 'auto' else [metric]
        
        ret = {}
        if 'accuracy' in metrics:
            ret['accuracy'] = _evaluation.accuracy(
                dataset[self.target], predicted[self.target])
        if 'auc' in metrics:
            ret['auc'] = _evaluation.auc(
                dataset[self.target], predicted['probability'], 
                index_map=self._class_to_index)
        if 'precision' in metrics:
            ret['precision'] = _evaluation.precision(
                dataset[self.target], predicted[self.target])
        if 'recall' in metrics:
            ret['recall'] = _evaluation.recall(
                dataset[self.target], predicted[self.target])
        if 'f1_score' in metrics:
            ret['f1_score'] = _evaluation.f1_score(
                dataset[self.target], predicted[self.target])
        if 'confusion_matrix' in metrics:
            ret['confusion_matrix'] = _evaluation.confusion_matrix(
                dataset[self.target], predicted[self.target])
        if 'roc_curve' in metrics:
            ret['roc_curve'] = _evaluation.roc_curve(
                dataset[self.target], predicted['probability'], 
                index_map=self._class_to_index)
        
        return ret

    def predict_topk(self, dataset, output_type="probability", k=3,
        batch_size=None):
        """
        Return top-k predictions for the ``dataset``, using the trained model.
        Predictions are returned as an SFrame with three columns: `id`,
        `class`, and `probability` or `rank`, depending on the ``output_type``
        parameter.

        Parameters
        ----------
        dataset : SFrame | SArray | turicreate.Image | list
            Drawings to be classified.
            If dataset is an SFrame, it must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        output_type : {'probability', 'rank'}, optional
            Choose the return type of the prediction:
            - `probability`: Probability associated with each label in the 
                             prediction.
            - `rank`       : Rank associated with each label in the prediction.
            
        k : int, optional
            Number of classes to return for each input example.

        batch_size : int, optional
            If you are getting memory errors, try decreasing this value. If you
            have a powerful computer, increasing this value may improve
            performance.

        Returns
        -------
        out : SFrame
            An SFrame with model predictions.

        See Also
        --------
        predict, evaluate

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
        _tkutl._check_categorical_option_type("output_type", output_type, 
            ["probability", "rank"])
        
        if not isinstance(k, int): 
            raise TypeError("'k' must be an integer >= 1")
        if k <= 0: 
            raise ValueError("'k' must be >= 1")
        if batch_size is not None and not isinstance(batch_size, int):
            raise TypeError("'batch_size' must be an integer >= 1")
        if batch_size is not None and batch_size < 1:
            raise ValueError("'batch_size' must be >= 1")

        prob_vector = self.predict(
            dataset, output_type='probability_vector', batch_size=batch_size)

        classes = self.classes
        if output_type == 'probability':
            results = prob_vector.apply(lambda p: [
                        {'class': classes[i], 'probability': p[i]}
                        for i in reversed(_np.argsort(p)[-k:])]
                      )
        else:
            assert(output_type == 'rank')
            results = prob_vector.apply(lambda p: [
                        {'class': classes[index], 'rank': rank}
                        for rank, index in enumerate(reversed(_np.argsort(p)[-k:]))]
                      )

        results = _tc.SFrame({'X': results})
        results = results.add_row_number()
        results = results.stack('X', new_column_name='X')
        results = results.unpack('X', column_name_prefix='')
        return results
        

    def predict(self, data, output_type='class', batch_size=None, verbose=True):
        """
        Predict on an SFrame or SArray of drawings, or on a single drawing.

        Parameters
        ----------
        data : SFrame | SArray | tc.Image | list
            The drawing(s) on which to perform drawing classification.
            If dataset is an SFrame, it must have a column with the same name
            as the feature column during training. Additional columns are
            ignored.
            If the data is a single drawing, it can be either of type tc.Image,
            in which case it is a bitmap-based drawing input,
            or of type list, in which case it is a stroke-based drawing input.

        output_type : {'probability', 'class', 'probability_vector'} optional
            Form of the predictions which are one of:
            - 'class': Class prediction. For multi-class classification, this
              returns the class with maximum probability.
            - 'probability': Prediction probability associated with the True
              class (not applicable for multi-class classification)
            - 'probability_vector': Prediction probability associated with each
              class as a vector. Label ordering is dictated by the ``classes``
              member variable.

        batch_size : int, optional
            If you are getting memory errors, try decreasing this value. If you
            have a powerful computer, increasing this value may improve
            performance.

        verbose : bool optional
            If True, prints prediction progress.

        Returns
        -------
        out : SArray
            An SArray with model predictions. Each element corresponds to
            a drawing and contains a single value corresponding to the
            predicted label. Each prediction will have type integer or string
            depending on the type of the classes the model was trained on.
            If `data` is a single drawing, the return value will be a single
            prediction.

        See Also
        --------
        evaluate

        Examples
        --------
        .. sourcecode:: python

            # Make predictions
            >>> pred = model.predict(data)

            # Print predictions, for a better overview
            >>> print(pred)
            dtype: int
            Rows: 10
            [3, 4, 3, 3, 4, 5, 8, 8, 8, 4]
        """
        _tkutl._check_categorical_option_type("output_type", output_type, 
            ["probability", "class", "probability_vector"])
        if isinstance(data, _tc.SArray):
            predicted = self._predict_with_probabilities(
                _tc.SFrame({
                    self.feature: data
                }),
                batch_size,
                verbose
            )
        elif isinstance(data, _tc.SFrame):
            predicted = self._predict_with_probabilities(data, batch_size, verbose)
        else:
            # single input
            predicted = self._predict_with_probabilities(
                _tc.SFrame({
                    self.feature: [data]
                }),
                batch_size,
                verbose
            )
        if output_type == "class":
            return predicted[self.target]
        elif output_type == "probability":
            _class_to_index = self._class_to_index
            target = self.target
            return predicted.apply(
                lambda row: row["probability"][_class_to_index[row[target]]])
        else:
            assert (output_type == "probability_vector")
            return predicted["probability"]
