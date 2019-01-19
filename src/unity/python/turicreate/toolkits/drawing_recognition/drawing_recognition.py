import turicreate as _tc
import urllib as _urllib
import os as _os
import glob as _glob
import numpy as _np
import mxnet as _mx
import time as _time
from mxnet import nd as _nd
from mxnet import gluon as _gluon
from mxnet.gluon import nn as _nn
from mxnet.gluon import HybridBlock as _HybridBlock
from mxnet.gluon.data.vision import datasets as _datasets
from mxnet.gluon.data.vision import transforms as _transforms
from turicreate.toolkits._model import CustomModel as _CustomModel
from ._sframe_loader import SFrameRecognitionIter as _SFrameRecognitionIter

class Model(_HybridBlock):
    def __init__(self, **kwargs):
        super(Model, self).__init__(**kwargs)
        with self.name_scope():
            # layers created in name_scope will inherit name space
            # from parent layer.
            self.conv1 = nn.Conv2D(channels=16, kernel_size=(3,3), padding=(1,1), activation='relu')
            self.pool1 = nn.MaxPool2D(pool_size=(2,2))
            self.conv2 = nn.Conv2D(channels=32, kernel_size=(3,3), padding=(1,1), activation='relu')
            self.pool2 = nn.MaxPool2D(pool_size=(2,2))
            self.conv3 = nn.Conv2D(channels=64, kernel_size=(3,3), padding=(1,1), activation='relu')
            self.pool3 = nn.MaxPool2D(pool_size=(2,2))
            self.flatten = nn.Flatten()
            self.fc1 = nn.Dense(units=128, activation='relu')
            self.fc2 = nn.Dense(units=NUM_CLASSES, activation=None) # NUM_CLASSES

    def hybrid_forward(self, F, x):
        x = self.pool1(self.conv1(x))
        x = self.pool2(self.conv2(x))
        x = self.pool3(self.conv3(x))
        x = self.flatten(x)
        x = self.fc1(x)
        x = self.fc2(x)
        return F.softmax(x)


def _accuracy_metric(output, label):
        # output: (batch, num_output) float32 ndarray
        # label: (batch, ) int32 ndarray
        # print("Measuring accuracy")
        return (output.argmax(axis=1) ==
                label.astype('float32')).mean().asscalar()


def create(dataset, annotations=None, num_epochs=25, feature=None, model=None,
           classes=None, batch_size=256, max_iterations=0, verbose=True,
           **kwargs):
    """
    Create a :class:`DrawingRecognition` model.
    """

    start_time = _time.time()

    column_names = ['Iteration', 'Loss', 'Elapsed Time']
    num_columns = len(column_names)
    column_width = max(map(lambda x: len(x), column_names)) + 2
    hr = '+' + '+'.join(['-' * column_width] * num_columns) + '+'

    progress = {'smoothed_loss': None, 'last_time': 0}
    iteration = 0

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

    loader = _SFrameDetectionIter(dataset, batch_size,
                 feature_column='bitmap',
                 annotation_column='label',
                 load_labels=True,
                 shuffle=True,
                 io_thread_buffer_size=0,
                 epochs=num_epochs,
                 iterations=None)
    model = Model()
    softmax_cross_entropy = gluon.loss.SoftmaxCrossEntropyLoss()
    
    model_params = model.collect_params()
    model_params.initialize(mx.init.Xavier(), ctx=mx.cpu(0))
    model.hybridize()
    trainer = mx.gluon.Trainer(model.collect_params(), 'adam')

    for batch in loader:
        data = _mx.gluon.utils.split_and_load(batch.data[0], ctx_list=ctx, batch_axis=0)
        label = _mx.gluon.utils.split_and_load(batch.label[0], ctx_list=ctx, batch_axis=0)

        with autograd.record():
            output = model(data)
            loss = softmax_cross_entropy(output, label)
        loss.backward()
        # update parameters
        trainer.step(1)
        # trainer.step(batch_size)
        # calculate training metrics
        train_loss += loss.mean().asscalar()
        train_acc += _accuracy_metric(output, label)

        update_progress(train_loss, batch.iteration)
        iteration = batch.iteration

    training_time = _time.time() - start_time
    if verbose:
        print(hr)   # progress table footer
    return DrawingRecognition(model)


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


class DrawingRecognition(_CustomModel):

    def __init__(self, mxnet_model):
        self._model = mxnet_model
    
    def evaluate(self, dataset, metric='auto', output_type='dict', verbose=True):
        pass

    def predict(self, dataset, confidence_threshold=0.25, verbose=True):
        pass
