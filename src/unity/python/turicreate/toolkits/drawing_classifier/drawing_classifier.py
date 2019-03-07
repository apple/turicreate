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
from .. import _mxnet_utils
from turicreate import extensions as _extensions
from .. import _pre_trained_models

BITMAP_WIDTH = 28
BITMAP_HEIGHT = 28

def _raise_error_if_not_drawing_classifier_input_sframe(
    dataset, feature, target):
    from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe
    _raise_error_if_not_sframe(dataset)
    if feature not in dataset.column_names():
        raise _ToolkitError("Feature column '%s' does not exist" % feature)
    if (dataset[feature].dtype != _tc.Image 
        and dataset[feature].dtype != list):
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

def create(input_dataset, feature="bitmap", target="label", 
            pretrained_model_url=None, classes=None, batch_size=256, 
            num_epochs=100, max_iterations=0, verbose=True, **kwargs):
    """
    Create a :class:`DrawingClassifier` model.
    """
    import mxnet as _mx
    from mxnet import autograd as _autograd
    from ._model_architecture import Model as _Model
    from ._sframe_loader import SFrameClassifierIter as _SFrameClassifierIter
    
    start_time = _time.time()

    if max_iterations == 0:
        max_iterations = num_epochs * len(input_dataset) / batch_size
    else:
        num_epochs = max_iterations * batch_size / len(input_dataset)

    _raise_error_if_not_drawing_classifier_input_sframe(
        input_dataset, feature, target)

    is_stroke_input = (input_dataset[feature].dtype != _tc.Image)
    dataset = _extensions._drawing_classifier_prepare_data(
        input_dataset, feature) if is_stroke_input else input_dataset

    column_names = ['Iteration', 'Loss', 'Elapsed Time']
    num_columns = len(column_names)
    column_width = max(map(lambda x: len(x), column_names)) + 2
    hr = '+' + '+'.join(['-' * column_width] * num_columns) + '+'

    progress = {'smoothed_loss': None, 'last_time': 0}
    iteration = 0

    if classes is None:
        classes = dataset[target].unique()
    classes = sorted(classes)
    class_to_index = {name: index for index, name in enumerate(classes)}

    def update_progress(cur_loss, iteration):
        iteration_base1 = iteration + 1
        if progress['smoothed_loss'] is None:
            progress['smoothed_loss'] = cur_loss
        else:
            progress['smoothed_loss'] = (0.9 * progress['smoothed_loss'] 
                + 0.1 * cur_loss)
        cur_time = _time.time()

        # Printing of table header is deferred, so that start-of-training
        # warnings appear above the table
        if verbose and iteration == 0:
            # Print progress table header
            print(hr)
            print(('| {:<{width}}' * num_columns + '|').format(*column_names, 
                width=column_width-1))
            print(hr)

        if verbose and (cur_time > progress['last_time'] + 10 or
                        iteration_base1 == max_iterations):
            # Print progress table row
            elapsed_time = cur_time - start_time
            print(
                "| {cur_iter:<{width}}| {loss:<{width}.3f}| {time:<{width}.1f}|".format(
                cur_iter=iteration_base1, loss=progress['smoothed_loss'],
                time=elapsed_time , width=column_width-1))
            progress['last_time'] = cur_time

    loader = _SFrameClassifierIter(dataset, batch_size,
                 feature_column=feature,
                 target_column=target,
                 class_to_index=class_to_index,
                 load_labels=True,
                 shuffle=True,
                 epochs=num_epochs,
                 iterations=None)

    ctx = _mxnet_utils.get_mxnet_context(max_devices=batch_size)
    model = _Model(num_classes = len(classes), prefix="drawing_")
    model_params = model.collect_params()
    model_params.initialize(_mx.init.Xavier(), ctx=ctx)

    if pretrained_model_url is not None:
        pretrained_model = _pre_trained_models.DrawingClassifierPreTrainedModel(pretrained_model_url)
        pretrained_model_params_path = pretrained_model.get_model_path()
        model.load_params(pretrained_model_params_path, 
            ctx=ctx, 
            allow_missing=True)
    softmax_cross_entropy = _mx.gluon.loss.SoftmaxCrossEntropyLoss()
    model.hybridize()
    trainer = _mx.gluon.Trainer(model.collect_params(), 'adam')

    train_loss = 0.
    for batch in loader:
        data = _mx.gluon.utils.split_and_load(batch.data[0], 
            ctx_list=ctx, batch_axis=0)[0]
        label = _mx.nd.array(
            _mx.gluon.utils.split_and_load(batch.label[0], 
                ctx_list=ctx, batch_axis=0)[0]
            )

        with _autograd.record():
            output = model(data)
            loss = softmax_cross_entropy(output, label)
        loss.backward()
        # update parameters
        trainer.step(1)
        # calculate training metrics
        cur_loss = loss.mean().asscalar()
        
        update_progress(cur_loss, batch.iteration)
        iteration = batch.iteration

    training_time = _time.time() - start_time
    if verbose:
        print(hr)   # progress table footer
    state = {
        '_model': model,
        '_class_to_index': class_to_index,
        'num_classes': len(classes),
        'classes': classes,
        'input_image_shape': (1, BITMAP_WIDTH, BITMAP_HEIGHT),
        'batch_size': batch_size,
        'training_loss': cur_loss,
        'training_time': training_time,
        'max_iterations': max_iterations,
        'target': target,
        'feature': feature,
        'num_examples': len(input_dataset)
    }
    return DrawingClassifier(state)

class DrawingClassifier(_CustomModel):

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
        return 1

    @classmethod
    def _load_version(cls, state, version):
        _tkutl._model_version_check(version, 1)
        from ._model_architecture import Model as _Model
        net = _Model(num_classes = len(state['classes']), prefix = 'drawing_')
        ctx = _mxnet_utils.get_mxnet_context(max_devices=state['batch_size'])
        net_params = net.collect_params()
        _mxnet_utils.load_net_params_from_state(
            net_params, state['_model'], ctx=ctx 
            )
        state['_model'] = net
        return DrawingClassifier(state)

    def export_coreml(self, filename, verbose=False):
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

        coreml_model = _mxnet_converter.convert(mod, mode = 'classifier',
                                class_labels = self.classes,
                                input_shape = [(self.feature, image_shape)],
                                builder = None, verbose=verbose,
                                preprocessor_args = {
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


    def _predict_with_probabilities(self, input_dataset):
        """
        Predict with probabilities.
        """

        import mxnet as _mx
        from ._sframe_loader import SFrameClassifierIter as _SFrameClassifierIter

        is_stroke_input = (input_dataset[self.feature].dtype != _tc.Image)

        if is_stroke_input:
            # @TODO: Make it work if labels are not passed in.
            dataset = _extensions._drawing_classifier_prepare_data(
                input_dataset, self.feature)
        else:
            dataset = input_dataset

        loader = _SFrameClassifierIter(dataset, self.batch_size,
                    class_to_index=self._class_to_index,
                    feature_column=self.feature,
                    target_column=self.target,
                    load_labels=False,
                    shuffle=False,
                    epochs=1,
                    iterations=None)

        dataset_size = len(dataset)
        ctx = _mxnet_utils.get_mxnet_context()
        
        all_predicted = ['']*dataset_size
        all_probabilities = _np.zeros((dataset_size, len(self.classes)), 
            dtype=float)
        
        index = 0
        for batch in loader:
            if batch.pad is not None:
                size = self.batch_size - batch.pad
                batch_data = _mx.nd.slice_axis(batch.data[0], 
                    axis=0, begin=0, end=size)
            else:
                batch_data = batch.data[0]
                size = self.batch_size

            if batch_data.shape[0] < len(ctx):
                ctx0 = ctx[:batch_data.shape[0]]
            else:
                ctx0 = ctx

            z = self._model(batch_data).asnumpy()
            predicted = z.argmax(axis=1)
            classes = self.classes
            
            predicted_sa = _tc.SArray(predicted).apply(lambda x: classes[x])
            
            all_predicted[index:index+len(predicted_sa)] = predicted_sa
            all_probabilities[index:index+z.shape[0]] = z
            index += z.shape[0]
        
        return (_tc.SFrame({self.target: _tc.SArray(all_predicted),
            'probability': _tc.SArray(all_probabilities)}))
     
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
        @TODO: Make this function better, more informative, and hide the 
               network architecture.
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
            ('Model', '_model')
        ]
        training_fields = [
            ('Training Time', 'training_time'),
            ('Training Iterations', 'max_iterations'),
            ('Number of Examples', 'num_examples'),
            ('Batch Size', 'batch_size'),
            ('Final Loss (specific to model)', 'training_loss'),
        ]

        section_titles = ['Schema', 'Training summary']
        return([model_fields, training_fields], section_titles)

    def evaluate(self, dataset, metric='auto', output_type='dict'):
        """
            Evaluate the model by making predictions of target values and comparing
            these to actual values.
            
            Parameters
            ----------
            dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the session_id, target and features used for model training.
            Additional columns are ignored.
            
            metric : str, optional
            Name of the evaluation metric.  Possible values are:
            
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
            >>> print results['accuracy']
        """

        if self.target not in dataset.column_names():
            raise _ToolkitError("Dataset provided to evaluate does not have " 
                + "ground truth in the " + self.target + " column.")

        predicted = self._predict_with_probabilities(dataset)

        target = _tc.SFrame({self.target: _tc.SArray(dataset[self.target])}) # fix this
        
        avail_metrics = ['accuracy', 'auc', 'precision', 'recall',
                         'f1_score', 'confusion_matrix', 'roc_curve']

        _tkutl._check_categorical_option_type(
                        'metric', metric, avail_metrics + ['auto'])

        if metric == 'auto':
            metrics = avail_metrics
        else:
            metrics = [metric]

        ret = {}
        if 'accuracy' in metrics:
            ret['accuracy'] = _evaluation.accuracy(target[self.target], predicted[self.target])
        if 'auc' in metrics:
            ret['auc'] = _evaluation.auc(target[self.target], predicted['probability'], index_map=self._class_to_index)
        if 'precision' in metrics:
            ret['precision'] = _evaluation.precision(target[self.target], predicted[self.target])
        if 'recall' in metrics:
            ret['recall'] = _evaluation.recall(target[self.target], predicted[self.target])
        if 'f1_score' in metrics:
            ret['f1_score'] = _evaluation.f1_score(target[self.target], predicted[self.target])
        if 'confusion_matrix' in metrics:
            ret['confusion_matrix'] = _evaluation.confusion_matrix(target[self.target], predicted[self.target])
        if 'roc_curve' in metrics:
            ret['roc_curve'] = _evaluation.roc_curve(target[self.target], predicted['probability'], index_map=self._class_to_index)
        
        return ret

    def predict(self, data):
        """ 
        Docfix coming soon
        """
        if type(data) == _tc.SArray:
            predicted = self._predict_with_probabilities(_tc.SFrame({
                self.feature: data
                })
            )
        else:
            predicted = self._predict_with_probabilities(data)
        return predicted[self.target]
