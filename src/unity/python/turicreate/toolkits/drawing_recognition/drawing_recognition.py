import turicreate as _tc
import urllib as _urllib
import os as _os
import glob as _glob
import numpy as _np
import mxnet as _mx
import time as _time
import mxnet as _mx
from mxnet import autograd as _autograd
from mxnet import nd as _nd
from mxnet import gluon as _gluon
from mxnet.gluon import nn as _nn
from mxnet.gluon import HybridBlock as _HybridBlock
from mxnet.gluon.data.vision import datasets as _datasets
from mxnet.gluon.data.vision import transforms as _transforms
from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits import evaluation as _evaluation
import turicreate.toolkits._internal_utils as _tkutl
from ._sframe_loader import SFrameRecognitionIter as _SFrameRecognitionIter
from .. import _mxnet_utils

from turicreate import extensions as _extensions

class Model(_HybridBlock):
    def __init__(self, num_classes, **kwargs):
        super(Model, self).__init__(**kwargs)
        with self.name_scope():
            # layers created in name_scope will inherit name space
            # from parent layer.
            self.conv1 = _nn.Conv2D(channels=16, kernel_size=(3,3), padding=(1,1), activation='relu')
            self.pool1 = _nn.MaxPool2D(pool_size=(2,2))
            self.conv2 = _nn.Conv2D(channels=32, kernel_size=(3,3), padding=(1,1), activation='relu')
            self.pool2 = _nn.MaxPool2D(pool_size=(2,2))
            self.conv3 = _nn.Conv2D(channels=64, kernel_size=(3,3), padding=(1,1), activation='relu')
            self.pool3 = _nn.MaxPool2D(pool_size=(2,2))
            self.flatten = _nn.Flatten()
            self.fc1 = _nn.Dense(units=128, activation='relu')
            self.fc2 = _nn.Dense(units=num_classes, activation=None) # NUM_CLASSES

    def hybrid_forward(self, F, x):
        x = self.pool1(self.conv1(x))
        x = self.pool2(self.conv2(x))
        x = self.pool3(self.conv3(x))
        x = self.flatten(x)
        x = self.fc1(x)
        x = self.fc2(x)
        return F.softmax(x)

def create(dataset, annotations=None, num_epochs=100, feature=None, model=None,
           classes=None, batch_size=256, max_iterations=0, verbose=True,
           **kwargs):
    """
    Create a :class:`DrawingRecognition` model.
    """

    start_time = _time.time()

    # dataset = _extensions._drawing_recognition_prepare_data(
    #     dataset, "bitmap", "label", True)

    
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
            progress['smoothed_loss'] = 0.9 * progress['smoothed_loss'] + 0.1 * cur_loss
        cur_time = _time.time()

        # Printing of table header is deferred, so that start-of-training
        # warnings appear above the table
        if verbose and iteration == 0:
            # Print progress table header
            print(hr)
            print(('| {:<{width}}' * num_columns + '|').format(*column_names, width=column_width-1))
            print(hr)

        if verbose and (cur_time > progress['last_time'] + 10 or
                        iteration_base1 == max_iterations):
            # Print progress table row
            elapsed_time = cur_time - start_time
            print("| {cur_iter:<{width}}| {loss:<{width}.3f}| {time:<{width}.1f}|".format(
                cur_iter=iteration_base1, loss=progress['smoothed_loss'],
                time=elapsed_time , width=column_width-1))
            progress['last_time'] = cur_time

    loader = _SFrameRecognitionIter(dataset, batch_size,
                 feature_column='bitmap',
                 annotations_column='label',
                 class_to_index=class_to_index,
                 load_labels=True,
                 shuffle=True,
                 io_thread_buffer_size=0,
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
    # train_acc = 0.
    for batch in loader:
        data = _mx.gluon.utils.split_and_load(batch.data[0], ctx_list=ctx, batch_axis=0)[0]
        label = _mx.nd.array(
            _mx.gluon.utils.split_and_load(batch.label[0], ctx_list=ctx, batch_axis=0)[0]
            )

        with _autograd.record():
            # print(data)
            # import pdb; pdb.set_trace()
            output = model(data)
            # debug tomorrow (Saturday)
            # try:
            loss = softmax_cross_entropy(output, label)
            # except:
            #     import pdb; pdb.set_trace()
        loss.backward()
        # update parameters
        trainer.step(1)
        # trainer.step(batch_size)
        # calculate training metrics
        cur_loss = loss.mean().asscalar()
        # train_acc += _accuracy_metric(output, label)

        update_progress(cur_loss, batch.iteration)
        iteration = batch.iteration

    training_time = _time.time() - start_time
    if verbose:
        print(hr)   # progress table footer
    state = {
        '_model': model,
        '_class_to_index': class_to_index,
        '_classes': classes,
        '_batch_size': batch_size
    }
    return DrawingRecognition(state)


    # train_dataset = gluon.data.dataset.ArrayDataset(x_train_mx, y_train_mx)
    # train_data_iterator = gluon.data.DataLoader(
    #     train_dataset, batch_size=batch_size, shuffle=True, num_workers=4)
    
    # for epoch in range(num_epochs):
    #     train_loss, train_acc, valid_acc = 0., 0., 0.
    #     tic = time.time()
    #     for data, label in train_data_iterator:
    #         # forward + backward
    #         with autograd.record():
    #             output = model(data)
    #             loss = softmax_cross_entropy(output, label)
    #         loss.backward()
    #         # update parameters
    #         trainer.step(batch_size)
    #         # calculate training metrics
    #         train_loss += loss.mean().asscalar()
    #         train_acc += _accuracy_metric(output, label)
        # calculate validation accuracy
        # for data, label in valid_data:
        #     valid_acc += acc(model(data), label)
        # print("Epoch %d: loss %.3f, train acc %.3f, in %.1f sec" % (
        #         epoch, train_loss/len(train_data_iterator), train_acc/len(train_data_iterator), time.time()-tic))

# def _check_closeness(f1, f2):
#     return (abs(f1-f2) < .01)

class DrawingRecognition(_CustomModel):

    def __init__(self, state):
        self._model = state['_model']
        self._classes = state['_classes']
        self._class_to_index = state['_class_to_index']
        self.batch_size = state['_batch_size']

    @classmethod
    def _native_name(cls):
        return "drawing_recognition"

    def _get_native_state(self):
        state = self.__proxy__.get_state()
        mxnet_params = self._model.collect_params()
        state['_model'] = _mxnet_utils.get_gluon_net_params_state(mxnet_params)
        return state

    def _get_version(self):
        return 1

    @classmethod
    def _load_version(cls, state, version):
        _tkutl._model_version_check(version, 1)
        # from ._model import tiny_darknet as _tiny_darknet

        # num_anchors = len(state['anchors'])
        # num_classes = state['num_classes']
        # output_size = (num_classes + 5) * num_anchors

        # net = _tiny_darknet(output_size=output_size)
        ctx = _mxnet_utils.get_mxnet_context(max_devices=state['_batch_size'])

        net = Model(num_classes = len(state['_classes']))
        net_params = net.collect_params()
        _mxnet_utils.load_net_params_from_state(
            net_params, state['_model'], ctx=ctx 
            )
        state['_model'] = net
        # state['input_image_shape'] = tuple([int(i) for i in [state['input_image_shape']]])
        # state['_grid_shape'] = tuple([int(i) for i in state['_grid_shape']])
        return DrawingRecognition(state)

    def export_coreml(self, filename):
        import mxnet as _mx
        from .._mxnet_to_coreml import _mxnet_converter
        import coremltools
        from coremltools.models import datatypes, neural_network
        from copy import copy

        batch_size = 1
        image_shape = (batch_size,) + (1,28,28)
        s_image_uint8 = _mx.sym.Variable('bitmap', 
            shape=image_shape, dtype=_np.float32)
        s_image = s_image_uint8


        net = copy(self._model)
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
        # print('new_aux_params')
        # print(new_aux_params)
        # print('new_arg_params')
        # print(new_arg_params)
        mod.set_params(new_arg_params, new_aux_params)

        input_dim = (1,28,28)
        input_features = [('bitmap', datatypes.Array(*image_shape))]
        # output_features = [('probabilities', datatypes.Dictionary(self._label_type)),('classLabel', self._label_type)]
        
        # builder = neural_network.NeuralNetworkBuilder(input_features, 
        #     output_features, mode='classifier')

        coreml_model = _mxnet_converter.convert(mod, mode='classifier',
                                class_labels=self._classes,
                                input_shape=[('bitmap', image_shape)],
                                builder=None, verbose=False,
                                is_drawing_recognition=True)

        # self._coreml_model = coreml_model
        coreml_model.save(filename)

        # input_names = ['bitmap']
        # input_dims = [(1,28,28)]

        # output_names = ['probabilities', 'classLabel']
        # _mxnet_converter._set_input_output_layers(
        #     builder, input_names, output_names)
        # builder.set_input(input_names, input_dims)
        # builder.set_output(output_names, output_dims)
        # builder.set_pre_processing_parameters(image_input_names=self.feature)
        # model = builder.spec
        # iouThresholdString = '(optional) IOU Threshold override (default: {})'
        # confidenceThresholdString = ('(optional)' + 
        #     ' Confidence Threshold override (default: {})')
        # model_type = 'object detector (%s)' % self.model
        # if include_non_maximum_suppression:
        #     model_type += ' with non-maximum suppression'
        # model.description.metadata.shortDescription = \
        #     _coreml_utils._mlmodel_short_description(model_type)
        # model.description.input[0].shortDescription = 'Input image'
        # if include_non_maximum_suppression:
        #     iouThresholdString = '(optional) IOU Threshold override (default: {})'
        #     model.description.input[1].shortDescription = \
        #         iouThresholdString.format(iou_threshold)
        #     confidenceThresholdString = ('(optional)' + 
        #         ' Confidence Threshold override (default: {})')
        #     model.description.input[2].shortDescription = \
        #         confidenceThresholdString.format(confidence_threshold)
        # model.description.output[0].shortDescription = \
        #     u'Boxes \xd7 Class confidence (see user-defined metadata "classes")'
        # model.description.output[1].shortDescription = \
        #     u'Boxes \xd7 [x, y, width, height] (relative to image size)'
        # version = ObjectDetector._PYTHON_OBJECT_DETECTOR_VERSION
        # partial_user_defined_metadata = {
        #     'model': self.model,
        #     'max_iterations': str(self.max_iterations),
        #     'training_iterations': str(self.training_iterations),
        #     'include_non_maximum_suppression': str(
        #         include_non_maximum_suppression),
        #     'non_maximum_suppression_threshold': str(
        #         iou_threshold),
        #     'confidence_threshold': str(confidence_threshold),
        #     'iou_threshold': str(iou_threshold),
        #     'feature': self.feature,
        #     'annotations': self.annotations,
        #     'classes': ','.join(self.classes)
        # }
        # user_defined_metadata = _coreml_utils._get_model_metadata(
        #     self.__class__.__name__,
        #     partial_user_defined_metadata,
        #     version)
        # model.description.metadata.userDefined.update(user_defined_metadata)


        # import pdb; pdb.set_trace()
        # from coremltools.models.utils import save_spec as _save_spec
        # _save_spec(coreml_model, filename)
        return coreml_model

    def _predict_with_options(self, dataset, with_ground_truth=True, verbose=True):
        loader = _SFrameRecognitionIter(dataset, self.batch_size,
                    class_to_index=self._class_to_index,
                    feature_column='bitmap',
                    annotations_column='label',
                    load_labels=True,
                    shuffle=False,
                    io_thread_buffer_size=0,
                    epochs=1,
                    iterations=None,
                    want_to_print=True)

        '''
            TODO:
            - Replace append op with assign.
            - Change return type from tuple to SFrame.
            - Remove dead code.
        '''
        num_returns = 2 if with_ground_truth else 1

        dataset_size = len(dataset)
        ctx = _mxnet_utils.get_mxnet_context()
        done = False
        last_time = 0
        raw_results = []

        # TODO: figure out a real solution to this problem. 
        # Need classes_to_index maybe
        # assert dataset['label'].dtype == type(self._classes[0])
        all_gt = _tc.SArray(dtype=dataset['label'].dtype)
        all_predicted = _tc.SArray(dtype=dataset['label'].dtype)
        all_probabilities = _np.zeros((len(dataset['label']),len(dataset['label'].unique())), dtype=float)
        index = 0
        for batch in loader:
            if batch.pad is not None:
                size = self.batch_size - batch.pad
                b_data = _mx.nd.slice_axis(batch.data[0], axis=0, begin=0, end=size)
                b_gt = _mx.nd.slice_axis(batch.label[0], axis=0, begin=0, end=size).asnumpy()
            else:
                b_data = batch.data[0]
                b_gt = batch.label[0].asnumpy()
                size = self.batch_size

            if b_data.shape[0] < len(ctx):
                ctx0 = ctx[:b_data.shape[0]]
            else:
                ctx0 = ctx

            z = self._model(b_data).asnumpy()
            predicted = z.argmax(axis=1)
            classes = self._classes
            predicted_sa = _tc.SArray(predicted).apply(lambda x: classes[x])
            b_gt_sa = _tc.SArray(b_gt).apply(lambda x: classes[x])
            all_predicted = all_predicted.append(predicted_sa) #need to remove append
            all_gt = all_gt.append(b_gt_sa) #need to remove append
            all_probabilities[index:index+z.shape[0]] = z
            index += z.shape[0]
            
    
        return (_tc.SFrame({'label': _tc.SArray(all_predicted)}), 
            _tc.SFrame({'label': _tc.SArray(all_probabilities)}))

            
    def evaluate(self, dataset, metric='auto', output_type='dict', verbose=True):

        '''
            TODO: change return type of _predict_with_options to return a single SFrame
        '''
        pred, probs = self._predict_with_options(dataset, with_ground_truth=True,
                                              verbose=verbose)
        target = _tc.SFrame({'label': _tc.SArray(dataset['label'])}) # fix this
 
        '''
            Metrics listed in image classifier evaluate - excluded log loss
        '''
        avail_metrics = ['accuracy', 'auc', 'precision', 'recall',
                         'f1_score', 'confusion_matrix', 'roc_curve']

        _tkutl._check_categorical_option_type(
                        'metric', metric, avail_metrics + ['auto'])

        if metric == 'auto':
            metrics = avail_metrics
        else:
            metrics = [metric]

        '''
            Return type dict matches image_classifier returns type
        '''
        ret = {}
        if 'accuracy' in metrics:
            ret['accuracy'] = _evaluation.accuracy(target['label'], pred['label'])
        if 'auc' in metrics:
            ret['auc'] = _evaluation.auc(target['label'], probs['label'], index_map=self._class_to_index)
        if 'precision' in metrics:
            ret['precision'] = _evaluation.precision(target['label'], pred['label'])
        if 'recall' in metrics:
            ret['recall'] = _evaluation.recall(target['label'], pred['label'])
        if 'f1_score' in metrics:
            ret['f1_score'] = _evaluation.f1_score(target['label'], pred['label'])
        if 'confusion_matrix' in metrics:
            ret['confusion_matrix'] = _evaluation.confusion_matrix(target['label'], pred['label'])
        if 'roc_curve' in metrics:
            ret['roc_curve'] = _evaluation.roc_curve(target['label'], probs['label'], index_map=self._class_to_index)
        
        return ret

    def predict(self, dataset, verbose=True):
        pred, _ = self._predict_with_options(dataset, with_ground_truth=True,
                                              verbose=verbose)
        return pred['label']
