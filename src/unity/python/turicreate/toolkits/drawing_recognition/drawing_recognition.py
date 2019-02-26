# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import turicreate as _tc
import numpy as _np
import time as _time
import mxnet as _mx
from mxnet import autograd as _autograd
from mxnet.gluon import nn as _nn
from mxnet.gluon import HybridBlock as _HybridBlock
from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from turicreate.toolkits import evaluation as _evaluation
import turicreate.toolkits._internal_utils as _tkutl
from ._sframe_loader import SFrameRecognitionIter as _SFrameRecognitionIter
from .. import _mxnet_utils
from turicreate import extensions as _extensions
from copy import copy as _copy

BITMAP_WIDTH = 28
BITMAP_HEIGHT = 28

class Model(_HybridBlock):
    def __init__(self, num_classes, **kwargs):
        super(Model, self).__init__(**kwargs)
        with self.name_scope():
            # layers created in name_scope will inherit name space
            # from parent layer.
            self.conv1 = _nn.Conv2D(channels=16, kernel_size=(3,3), 
                                    padding=(1,1), activation='relu')
            self.pool1 = _nn.MaxPool2D(pool_size=(2,2))
            self.conv2 = _nn.Conv2D(channels=32, kernel_size=(3,3), 
                                    padding=(1,1), activation='relu')
            self.pool2 = _nn.MaxPool2D(pool_size=(2,2))
            self.conv3 = _nn.Conv2D(channels=64, kernel_size=(3,3), 
                                    padding=(1,1), activation='relu')
            self.pool3 = _nn.MaxPool2D(pool_size=(2,2))
            self.flatten = _nn.Flatten()
            self.fc1 = _nn.Dense(units=128, activation='relu')
            self.fc2 = _nn.Dense(units=num_classes, activation=None)

    def hybrid_forward(self, F, x):
        x = self.pool1(self.conv1(x))
        x = self.pool2(self.conv2(x))
        x = self.pool3(self.conv3(x))
        x = self.flatten(x)
        x = self.fc1(x)
        x = self.fc2(x)
        return F.softmax(x)

def create(input_dataset, annotations=None, num_epochs=100, feature="bitmap", model=None,
           classes=None, batch_size=256, max_iterations=0, verbose=True, 
           **kwargs):
    """
    Create a :class:`DrawingRecognition` model.
    """
    start_time = _time.time()

    is_stroke_input = (input_dataset[feature].dtype != _tc.Image)

    if is_stroke_input:
        # This only works on macOS right now
        dataset = _extensions._drawing_recognition_prepare_data(
            input_dataset, "bitmap", "label")
    else:
        dataset = input_dataset

    column_names = ['Iteration', 'Loss', 'Elapsed Time']
    num_columns = len(column_names)
    column_width = max(map(lambda x: len(x), column_names)) + 2
    hr = '+' + '+'.join(['-' * column_width] * num_columns) + '+'

    progress = {'smoothed_loss': None, 'last_time': 0}
    iteration = 0

    if classes is None:
        classes = dataset['label'].unique()
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

    loader = _SFrameRecognitionIter(dataset, batch_size,
                 feature_column='bitmap',
                 target_column='label',
                 class_to_index=class_to_index,
                 load_labels=True,
                 shuffle=True,
                 epochs=num_epochs,
                 iterations=None)
    model = Model(num_classes = len(classes))
    softmax_cross_entropy = _mx.gluon.loss.SoftmaxCrossEntropyLoss()
    
    ctx = _mxnet_utils.get_mxnet_context(max_devices=batch_size)

    model_params = model.collect_params()
    model_params.initialize(_mx.init.Xavier(), ctx=ctx)
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
        'max_iterations': max_iterations
    }
    return DrawingRecognition(state)

class DrawingRecognition(_CustomModel):

    def __init__(self, state):
        self.__proxy__ = _PythonProxy(state)
        

    @classmethod
    def _native_name(cls):
        return "drawing_recognition"

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
        net = Model(num_classes = len(state['classes']), prefix = 'model0_')
        ctx = _mxnet_utils.get_mxnet_context(max_devices=state['batch_size'])
        net_params = net.collect_params()
        _mxnet_utils.load_net_params_from_state(
            net_params, state['_model'], ctx=ctx 
            )
        state['_model'] = net
        return DrawingRecognition(state)

    def export_coreml(self, filename, verbose=False):
        import mxnet as _mx
        from .._mxnet_to_coreml import _mxnet_converter
        import coremltools
        from coremltools.models import datatypes, neural_network

        batch_size = 1
        image_shape = (batch_size,) + (1, BITMAP_WIDTH, BITMAP_HEIGHT)
        s_image = _mx.sym.Variable('bitmap',
            shape=image_shape, dtype=_np.float32)

        net = _copy(self._model)
        s_ymap = net(s_image)
        
        mod = _mx.mod.Module(symbol=s_ymap, label_names=None, data_names=['bitmap'])
        mod.bind(for_training=False, data_shapes=[('bitmap', image_shape)])
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
                                input_shape=[('bitmap', image_shape)],
                                builder=None, verbose=verbose,
                                preprocessor_args={'image_input_names':['bitmap']},
                                is_drawing_recognition=True)

        from turicreate.toolkits import _coreml_utils
        model_type = "drawing classifier"
        coreml_model.short_description = _coreml_utils._mlmodel_short_description(model_type)
        coreml_model.input_description['bitmap'] = u'bitmap'
        coreml_model.output_description['probabilities'] = 'Prediction probabilities'
        coreml_model.output_description['classLabel'] = 'Class Label of Top Prediction'
        coreml_model.save(filename)
        return coreml_model

    def _predict_with_probabilities(self, input_dataset):
        """
        Predict with probabilities.
        """

        is_stroke_input = (input_dataset['bitmap'].dtype != _tc.Image)

        if is_stroke_input:
            # @TODO: Make it work for Linux
            # @TODO: Make it work if labels are not passed in.
            dataset = _extensions._drawing_recognition_prepare_data(
                input_dataset, "bitmap", "label")
        else:
            dataset = input_dataset

        loader = _SFrameRecognitionIter(dataset, self.batch_size,
                    class_to_index=self._class_to_index,
                    feature_column='bitmap',
                    target_column='label',
                    load_labels=True,
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
        
        return (_tc.SFrame({'label': _tc.SArray(all_predicted),
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
            ('Training time', 'training_time'),
            ('Training iterations', 'max_iterations'),
            ('Batch size', 'batch_size'),
            ('Final loss (specific to model)', 'training_loss'),
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

        predicted = self._predict_with_probabilities(dataset)

        target = _tc.SFrame({'label': _tc.SArray(dataset['label'])}) # fix this
        
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
            ret['accuracy'] = _evaluation.accuracy(target['label'], predicted['label'])
        if 'auc' in metrics:
            ret['auc'] = _evaluation.auc(target['label'], predicted['probability'], index_map=self._class_to_index)
        if 'precision' in metrics:
            ret['precision'] = _evaluation.precision(target['label'], predicted['label'])
        if 'recall' in metrics:
            ret['recall'] = _evaluation.recall(target['label'], predicted['label'])
        if 'f1_score' in metrics:
            ret['f1_score'] = _evaluation.f1_score(target['label'], predicted['label'])
        if 'confusion_matrix' in metrics:
            ret['confusion_matrix'] = _evaluation.confusion_matrix(target['label'], predicted['label'])
        if 'roc_curve' in metrics:
            ret['roc_curve'] = _evaluation.roc_curve(target['label'], predicted['probability'], index_map=self._class_to_index)
        
        return ret

    def predict(self, dataset):
        """ 
        Docfix coming soon
        """
        predicted = self._predict_with_probabilities(dataset)
        return predicted['label']

