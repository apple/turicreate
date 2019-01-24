# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Class definition and utilities for the object detection toolkit.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import time as _time
import itertools as _itertools
from datetime import datetime as _datetime

import six as _six
import turicreate as _tc
import numpy as _np
from threading import Thread as _Thread
from six.moves.queue import Queue as _Queue

from turicreate.toolkits._model import CustomModel as _CustomModel
import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits import _coreml_utils
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from turicreate.toolkits._internal_utils import (_raise_error_if_not_sframe,
                                                 _numeric_param_check_range)
from turicreate import config as _tc_config
from .. import _mxnet_utils
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from .. import _pre_trained_models
from ._evaluation import average_precision as _average_precision
from .._mps_utils import (use_mps as _use_mps,
                          mps_device_memory_limit as _mps_device_memory_limit,
                          MpsGraphAPI as _MpsGraphAPI,
                          MpsGraphNetworkType as _MpsGraphNetworkType,
                          MpsGraphMode as _MpsGraphMode,
                          mps_to_mxnet as _mps_to_mxnet,
                          mxnet_to_mps as _mxnet_to_mps)


_MXNET_MODEL_FILENAME = "mxnet_model.params"


def _get_mps_od_net(input_image_shape, batch_size, output_size, anchors,
                    config, weights={}):
    """
    Initializes an MpsGraphAPI for object detection.
    """
    network = _MpsGraphAPI(network_id=_MpsGraphNetworkType.kODGraphNet)

    c_in, h_in, w_in =  input_image_shape
    c_out = output_size
    h_out = h_in // 32
    w_out = w_in // 32

    c_view = c_in
    h_view = h_in
    w_view = w_in

    network.init(batch_size, c_in, h_in, w_in, c_out, h_out, w_out,
                 weights=weights, config=config)
    return network


# Standard lib functions would be great here, but the formatting options of
# timedelta are not great
def _seconds_as_string(seconds):
    """
    Returns seconds as a human-friendly string, e.g. '1d 4h 47m 41s'
    """
    TIME_UNITS = [('s', 60), ('m', 60), ('h', 24), ('d', None)]
    unit_strings = []
    cur = max(int(seconds), 1)
    for suffix, size in TIME_UNITS:
        if size is not None:
            cur, rest = divmod(cur, size)
        else:
            rest = cur
        if rest > 0:
            unit_strings.insert(0, '%d%s' % (rest, suffix))
    return ' '.join(unit_strings)


def _raise_error_if_not_detection_sframe(dataset, feature, annotations, require_annotations):
    _raise_error_if_not_sframe(dataset, 'datset')
    if feature not in dataset.column_names():
        raise _ToolkitError("Feature column '%s' does not exist" % feature)
    if dataset[feature].dtype != _tc.Image:
        raise _ToolkitError("Feature column must contain images")

    if require_annotations:
        if annotations not in dataset.column_names():
            raise _ToolkitError("Annotations column '%s' does not exist" % annotations)
        if dataset[annotations].dtype not in [list, dict]:
            raise _ToolkitError("Annotations column must be of type dict or list")


def create(dataset, annotations=None, feature=None, model='darknet-yolo',
           classes=None, batch_size=0, max_iterations=0, verbose=True,
           **kwargs):
    """
    Create a :class:`ObjectDetector` model.

    Parameters
    ----------
    dataset : SFrame
        Input data. The columns named by the ``feature`` and ``annotations``
        parameters will be extracted for training the detector.

    annotations : string
        Name of the column containing the object detection annotations.  This
        column should be a list of dictionaries (or a single dictionary), with
        each dictionary representing a bounding box of an object instance. Here
        is an example of the annotations for a single image with two object
        instances::

            [{'label': 'dog',
              'type': 'rectangle',
              'coordinates': {'x': 223, 'y': 198,
                              'width': 130, 'height': 230}},
             {'label': 'cat',
              'type': 'rectangle',
              'coordinates': {'x': 40, 'y': 73,
                              'width': 80, 'height': 123}}]

        The value for `x` is the horizontal center of the box paired with
        `width` and `y` is the vertical center of the box paired with `height`.
        'None' (the default) indicates the only list column in `dataset` should
        be used for the annotations.

    feature : string
        Name of the column containing the input images. 'None' (the default)
        indicates the only image column in `dataset` should be used as the
        feature.

    model : string optional
        Object detection model to use:

           - "darknet-yolo" : Fast and medium-sized model

    classes : list optional
        List of strings containing the names of the classes of objects.
        Inferred from the data if not provided.

    batch_size: int
        The number of images per training iteration. If 0, then it will be
        automatically determined based on resource availability.

    max_iterations : int
        The number of training iterations. If 0, then it will be automatically
        be determined based on the amount of data you provide.

    verbose : bool, optional
        If True, print progress updates and model details.

    Returns
    -------
    out : ObjectDetector
        A trained :class:`ObjectDetector` model.

    See Also
    --------
    ObjectDetector

    Examples
    --------
    .. sourcecode:: python

        # Train an object detector model
        >>> model = turicreate.object_detector.create(data)

        # Make predictions on the training set and as column to the SFrame
        >>> data['predictions'] = model.predict(data)

        # Visualize predictions by generating a new column of marked up images
        >>> data['image_pred'] = turicreate.object_detector.util.draw_bounding_boxes(data['image'], data['predictions'])
    """
    _raise_error_if_not_sframe(dataset, "dataset")
    from ._mx_detector import YOLOLoss as _YOLOLoss
    from ._model import tiny_darknet as _tiny_darknet
    from ._sframe_loader import SFrameDetectionIter as _SFrameDetectionIter
    from ._manual_scheduler import ManualScheduler as _ManualScheduler
    import mxnet as _mx
    if len(dataset) == 0:
        raise _ToolkitError('Unable to train on empty dataset')

    _numeric_param_check_range('max_iterations', max_iterations, 0, _six.MAXSIZE)
    start_time = _time.time()

    supported_detectors = ['darknet-yolo']

    if feature is None:
        feature = _tkutl._find_only_image_column(dataset)
        if verbose:
            print("Using '%s' as feature column" % feature)
    if annotations is None:
        annotations = _tkutl._find_only_column_of_type(dataset,
                                                       target_type=[list, dict],
                                                       type_name='list',
                                                       col_name='annotations')
        if verbose:
            print("Using '%s' as annotations column" % annotations)

    _raise_error_if_not_detection_sframe(dataset, feature, annotations,
                                         require_annotations=True)
    is_annotations_list = dataset[annotations].dtype == list

    _tkutl._check_categorical_option_type('model', model,
            supported_detectors)

    base_model = model.split('-', 1)[0]
    ref_model = _pre_trained_models.OBJECT_DETECTION_BASE_MODELS[base_model]()

    params = {
        'anchors': [
            (1.0, 2.0), (1.0, 1.0), (2.0, 1.0),
            (2.0, 4.0), (2.0, 2.0), (4.0, 2.0),
            (4.0, 8.0), (4.0, 4.0), (8.0, 4.0),
            (8.0, 16.0), (8.0, 8.0), (16.0, 8.0),
            (16.0, 32.0), (16.0, 16.0), (32.0, 16.0),
        ],
        'grid_shape': [13, 13],
        'aug_resize': 0,
        'aug_rand_crop': 0.9,
        'aug_rand_pad': 0.9,
        'aug_rand_gray': 0.0,
        'aug_aspect_ratio': 1.25,
        'aug_hue': 0.05,
        'aug_brightness': 0.05,
        'aug_saturation': 0.05,
        'aug_contrast': 0.05,
        'aug_horizontal_flip': True,
        'aug_min_object_covered': 0,
        'aug_min_eject_coverage': 0.5,
        'aug_area_range': (.15, 2),
        'aug_pca_noise': 0.0,
        'aug_max_attempts': 20,
        'aug_inter_method': 2,
        'lmb_coord_xy': 10.0,
        'lmb_coord_wh': 10.0,
        'lmb_obj': 100.0,
        'lmb_noobj': 5.0,
        'lmb_class': 2.0,
        'non_maximum_suppression_threshold': 0.45,
        'rescore': True,
        'clip_gradients': 0.025,
        'weight_decay': 0.0005,
        'sgd_momentum': 0.9,
        'learning_rate': 1.0e-3,
        'shuffle': True,
        'mps_loss_mult': 8,
        # This large buffer size (8 batches) is an attempt to mitigate against
        # the SFrame shuffle operation that can occur after each epoch.
        'io_thread_buffer_size': 8,
    }

    if '_advanced_parameters' in kwargs:
        # Make sure no additional parameters are provided
        new_keys = set(kwargs['_advanced_parameters'].keys())
        set_keys = set(params.keys()) 
        unsupported = new_keys - set_keys
        if unsupported:
            raise _ToolkitError('Unknown advanced parameters: {}'.format(unsupported))

        params.update(kwargs['_advanced_parameters'])

    anchors = params['anchors']
    num_anchors = len(anchors)

    if batch_size < 1:
        batch_size = 32  # Default if not user-specified
    cuda_gpus = _mxnet_utils.get_gpus_in_use(max_devices=batch_size)
    num_mxnet_gpus = len(cuda_gpus)
    use_mps = _use_mps() and num_mxnet_gpus == 0
    batch_size_each = batch_size // max(num_mxnet_gpus, 1)
    if use_mps and _mps_device_memory_limit() < 4 * 1024 * 1024 * 1024:
        # Reduce batch size for GPUs with less than 4GB RAM
        batch_size_each = 16
    # Note, this may slightly alter the batch size to fit evenly on the GPUs
    batch_size = max(num_mxnet_gpus, 1) * batch_size_each
    if verbose:
        print("Setting 'batch_size' to {}".format(batch_size))


    # The IO thread also handles MXNet-powered data augmentation. This seems
    # to be problematic to run independently of a MXNet-powered neural network
    # in a separate thread. For this reason, we restrict IO threads to when
    # the neural network backend is MPS.
    io_thread_buffer_size = params['io_thread_buffer_size'] if use_mps else 0

    if verbose:
        # Estimate memory usage (based on experiments)
        cuda_mem_req = 550 + batch_size_each * 85

        _tkutl._print_neural_compute_device(cuda_gpus=cuda_gpus, use_mps=use_mps,
                                            cuda_mem_req=cuda_mem_req)

    grid_shape = params['grid_shape']
    input_image_shape = (3,
                         grid_shape[0] * ref_model.spatial_reduction,
                         grid_shape[1] * ref_model.spatial_reduction)

    try:
        if is_annotations_list:
            instances = (dataset.stack(annotations, new_column_name='_bbox', drop_na=True)
                            .unpack('_bbox', limit=['label']))
        else:
            instances = dataset.rename({annotations: '_bbox'}).dropna('_bbox')
            instances = instances.unpack('_bbox', limit=['label'])

    except (TypeError, RuntimeError):
        # If this fails, the annotation format isinvalid at the coarsest level
        raise _ToolkitError("Annotations format is invalid. Must be a list of "
           "dictionaries or single dictionary containing 'label' and 'coordinates'.")

    num_images = len(dataset)
    num_instances = len(instances)
    if classes is None:
        classes = instances['_bbox.label'].unique()
    classes = sorted(classes)

    # Make a class-to-index look-up table
    class_to_index = {name: index for index, name in enumerate(classes)}
    num_classes = len(classes)

    if max_iterations == 0:
        # Set number of iterations through a heuristic
        num_iterations_raw = 5000 * _np.sqrt(num_instances) / batch_size
        num_iterations = 1000 * max(1, int(round(num_iterations_raw / 1000)))
        if verbose:
            print("Setting 'max_iterations' to {}".format(num_iterations))
    else:
        num_iterations = max_iterations

    # Create data loader
    loader = _SFrameDetectionIter(dataset,
                                  batch_size=batch_size,
                                  input_shape=input_image_shape[1:],
                                  output_shape=grid_shape,
                                  anchors=anchors,
                                  class_to_index=class_to_index,
                                  aug_params=params,
                                  shuffle=params['shuffle'],
                                  loader_type='augmented',
                                  feature_column=feature,
                                  annotations_column=annotations,
                                  io_thread_buffer_size=io_thread_buffer_size,
                                  iterations=num_iterations)

    # Predictions per anchor box: x/y + w/h + object confidence + class probs
    preds_per_box = 5 + num_classes
    output_size = preds_per_box * num_anchors
    ymap_shape = (batch_size_each,) + tuple(grid_shape) + (num_anchors, preds_per_box)

    net = _tiny_darknet(output_size=output_size)

    loss = _YOLOLoss(input_shape=input_image_shape[1:],
                     output_shape=grid_shape,
                     batch_size=batch_size_each,
                     num_classes=num_classes,
                     anchors=anchors,
                     parameters=params)

    base_lr = params['learning_rate']
    steps = [num_iterations // 2, 3 * num_iterations // 4, num_iterations]
    steps_and_factors = [(step, 10**(-i)) for i, step in enumerate(steps)]

    steps, factors = zip(*steps_and_factors)
    lr_scheduler = _ManualScheduler(step=steps, factor=factors)

    ctx = _mxnet_utils.get_mxnet_context(max_devices=batch_size)

    net_params = net.collect_params()
    net_params.initialize(_mx.init.Xavier(), ctx=ctx)
    net_params['conv7_weight'].initialize(_mx.init.Xavier(factor_type='avg'), ctx=ctx, force_reinit=True)
    net_params['conv8_weight'].initialize(_mx.init.Uniform(0.00005), ctx=ctx, force_reinit=True)
    # Initialize object confidence low, preventing an unnecessary adjustment
    # period toward conservative estimates
    bias = _np.zeros(output_size, dtype=_np.float32)
    bias[4::preds_per_box] -= 6
    from ._mx_detector import ConstantArray
    net_params['conv8_bias'].initialize(ConstantArray(bias), ctx, force_reinit=True)

    # Take a subset and then load the rest of the parameters. It is possible to
    # do allow_missing=True directly on net_params. However, this will more
    # easily hide bugs caused by names getting out of sync.
    ref_model.available_parameters_subset(net_params).load(ref_model.model_path, ctx)

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

    if use_mps:
        # Force initialization of net_params
        # TODO: Do not rely on MXNet to initialize MPS-based network
        net.forward(_mx.nd.uniform(0, 1, (batch_size_each,) + input_image_shape))
        mps_net_params = {}
        keys = list(net_params)
        for k in keys:
            mps_net_params[k] = net_params[k].data().asnumpy()

        # Multiplies the loss to move the fp16 gradients away from subnormals
        # and gradual underflow. The learning rate is correspondingly divided
        # by the same multiple to make training mathematically equivalent. The
        # update is done in fp32, which is why this trick works. Does not
        # affect how loss is presented to the user.
        mps_loss_mult = params['mps_loss_mult']

        mps_config = {
            'mode': _MpsGraphMode.Train,
            'use_sgd': True,
            'learning_rate': base_lr / params['mps_loss_mult'],
            'gradient_clipping': params.get('clip_gradients', 0.0) * mps_loss_mult,
            'weight_decay': params['weight_decay'],
            'od_include_network': True,
            'od_include_loss': True,
            'od_scale_xy': params['lmb_coord_xy'] * mps_loss_mult,
            'od_scale_wh': params['lmb_coord_wh'] * mps_loss_mult,
            'od_scale_no_object': params['lmb_noobj'] * mps_loss_mult,
            'od_scale_object': params['lmb_obj'] * mps_loss_mult,
            'od_scale_class': params['lmb_class'] * mps_loss_mult,
            'od_max_iou_for_no_object': 0.3,
            'od_min_iou_for_object': 0.7,
            'od_rescore': params['rescore'],
        }

        mps_net = _get_mps_od_net(input_image_shape=input_image_shape,
                                  batch_size=batch_size,
                                  output_size=output_size,
                                  anchors=anchors,
                                  config=mps_config,
                                  weights=mps_net_params)

        # Use worker threads to isolate different points of synchronization
        # and/or waiting for non-Python tasks to finish. The
        # sframe_worker_thread will spend most of its time waiting for SFrame
        # operations, largely image I/O and decoding, along with scheduling
        # MXNet data augmentation. The numpy_worker_thread will spend most of
        # its time waiting for MXNet data augmentation to complete, along with
        # copying the results into NumPy arrays. Finally, the main thread will
        # spend most of its time copying NumPy data into MPS and waiting for the
        # results. Note that using three threads here only makes sense because
        # each thread spends time waiting for non-Python code to finish (so that
        # no thread hogs the global interpreter lock).
        mxnet_batch_queue = _Queue(1)
        numpy_batch_queue = _Queue(1)
        def sframe_worker():
            # Once a batch is loaded into NumPy, pass it immediately to the
            # numpy_worker so that we can start I/O and decoding for the next
            # batch.
            for batch in loader:
                mxnet_batch_queue.put(batch)
            mxnet_batch_queue.put(None)
        def numpy_worker():
            while True:
                batch = mxnet_batch_queue.get()
                if batch is None:
                    break

                for x, y in zip(batch.data, batch.label):
                    # Convert to NumPy arrays with required shapes. Note that
                    # asnumpy waits for any pending MXNet operations to finish.
                    input_data = _mxnet_to_mps(x.asnumpy())
                    label_data = y.asnumpy().reshape(y.shape[:-2] + (-1,))

                    # Convert to packed 32-bit arrays.
                    input_data = input_data.astype(_np.float32)
                    if not input_data.flags.c_contiguous:
                        input_data = input_data.copy()
                    label_data = label_data.astype(_np.float32)
                    if not label_data.flags.c_contiguous:
                        label_data = label_data.copy()

                    # Push this batch to the main thread.
                    numpy_batch_queue.put({'input'     : input_data,
                                           'label'     : label_data,
                                           'iteration' : batch.iteration})
            # Tell the main thread there's no more data.
            numpy_batch_queue.put(None)
        sframe_worker_thread = _Thread(target=sframe_worker)
        sframe_worker_thread.start()
        numpy_worker_thread = _Thread(target=numpy_worker)
        numpy_worker_thread.start()

        batch_queue = []
        def wait_for_batch():
            pending_loss = batch_queue.pop(0)
            batch_loss = pending_loss.asnumpy()  # Waits for the batch to finish
            return batch_loss.sum() / mps_loss_mult

        while True:
            batch = numpy_batch_queue.get()
            if batch is None:
                break

            # Adjust learning rate according to our schedule.
            if batch['iteration'] in steps:
                ii = steps.index(batch['iteration']) + 1
                new_lr = factors[ii] * base_lr
                mps_net.set_learning_rate(new_lr / mps_loss_mult)

            # Submit this match to MPS.
            batch_queue.append(mps_net.train(batch['input'], batch['label']))

            # If we have two batches in flight, wait for the first one.
            if len(batch_queue) > 1:
                cur_loss = wait_for_batch()

                # If we just submitted the first batch of an iteration, update
                # progress for the iteration completed by the last batch we just
                # waited for.
                if batch['iteration'] > iteration:
                    update_progress(cur_loss, iteration)
            iteration = batch['iteration']

        # Wait for any pending batches and finalize our progress updates.
        while len(batch_queue) > 0:
            cur_loss = wait_for_batch()
        update_progress(cur_loss, iteration)

        sframe_worker_thread.join()
        numpy_worker_thread.join()

        # Load back into mxnet
        mps_net_params = mps_net.export()
        keys = mps_net_params.keys()
        for k in keys:
            if k in net_params:
                net_params[k].set_data(mps_net_params[k])

    else:  # Use MxNet
        net.hybridize()

        options = {'learning_rate': base_lr, 'lr_scheduler': lr_scheduler,
                   'momentum': params['sgd_momentum'], 'wd': params['weight_decay'], 'rescale_grad': 1.0}
        clip_grad = params.get('clip_gradients')
        if clip_grad:
            options['clip_gradient'] = clip_grad
        trainer = _mx.gluon.Trainer(net.collect_params(), 'sgd', options)

        for batch in loader:
            data = _mx.gluon.utils.split_and_load(batch.data[0], ctx_list=ctx, batch_axis=0)
            label = _mx.gluon.utils.split_and_load(batch.label[0], ctx_list=ctx, batch_axis=0)

            Ls = []
            Zs = []

            with _mx.autograd.record():
                for x, y in zip(data, label):
                    z = net(x)
                    z0 = _mx.nd.transpose(z, [0, 2, 3, 1]).reshape(ymap_shape)
                    L = loss(z0, y)
                    Ls.append(L)
                for L in Ls:
                    L.backward()

            trainer.step(1)
            cur_loss = _np.mean([L.asnumpy()[0] for L in Ls])

            update_progress(cur_loss, batch.iteration)
            iteration = batch.iteration

    training_time = _time.time() - start_time
    if verbose:
        print(hr)   # progress table footer

    # Save the model
    training_iterations = iteration + 1
    state = {
        '_model': net,
        '_class_to_index': class_to_index,
        '_training_time_as_string': _seconds_as_string(training_time),
        '_grid_shape': grid_shape,
        'anchors': anchors,
        'model': model,
        'classes': classes,
        'batch_size': batch_size,
        'input_image_shape': input_image_shape,
        'feature': feature,
        'non_maximum_suppression_threshold': params['non_maximum_suppression_threshold'],
        'annotations': annotations,
        'num_classes': num_classes,
        'num_examples': num_images,
        'num_bounding_boxes': num_instances,
        'training_time': training_time,
        'training_epochs': training_iterations * batch_size // num_images,
        'training_iterations': training_iterations,
        'max_iterations': max_iterations,
        'training_loss': progress['smoothed_loss'],
    }
    return ObjectDetector(state)


class ObjectDetector(_CustomModel):
    """
    An trained model that is ready to use for classification, exported to
    Core ML, or for feature extraction.

    This model should not be constructed directly.
    """

    _PYTHON_OBJECT_DETECTOR_VERSION = 1

    def __init__(self, state):
        self.__proxy__ = _PythonProxy(state)

    @classmethod
    def _native_name(cls):
        return "object_detector"

    def _get_native_state(self):
        state = self.__proxy__.get_state()
        mxnet_params = state['_model'].collect_params()
        state['_model'] = _mxnet_utils.get_gluon_net_params_state(mxnet_params)
        return state

    def _get_version(self):
        return self._PYTHON_OBJECT_DETECTOR_VERSION

    @classmethod
    def _load_version(cls, state, version):
        _tkutl._model_version_check(version, cls._PYTHON_OBJECT_DETECTOR_VERSION)
        from ._model import tiny_darknet as _tiny_darknet

        num_anchors = len(state['anchors'])
        num_classes = state['num_classes']
        output_size = (num_classes + 5) * num_anchors

        net = _tiny_darknet(output_size=output_size)
        ctx = _mxnet_utils.get_mxnet_context(max_devices=state['batch_size'])

        net_params = net.collect_params()
        _mxnet_utils.load_net_params_from_state(net_params, state['_model'], ctx=ctx)
        state['_model'] = net
        state['input_image_shape'] = tuple([int(i) for i in state['input_image_shape']])
        state['_grid_shape'] = tuple([int(i) for i in state['_grid_shape']])
        return ObjectDetector(state)

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the ObjectDetector.
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
            ('Model', 'model'),
            ('Number of classes', 'num_classes'),
            ('Non-maximum suppression threshold', 'non_maximum_suppression_threshold'),
            ('Input image shape', 'input_image_shape'),
        ]
        training_fields = [
            ('Training time', '_training_time_as_string'),
            ('Training epochs', 'training_epochs'),
            ('Training iterations', 'training_iterations'),
            ('Number of examples (images)', 'num_examples'),
            ('Number of bounding boxes (instances)', 'num_bounding_boxes'),
            ('Final loss (specific to model)', 'training_loss'),
        ]

        section_titles = ['Schema', 'Training summary']
        return([model_fields, training_fields], section_titles)

    def _predict_with_options(self, dataset, with_ground_truth,
                              postprocess=True, confidence_threshold=0.001,
                              iou_threshold=None,
                              verbose=True):
        """
        Predict with options for what kind of SFrame should be returned.

        If postprocess is False, a single numpy array with raw unprocessed
        results will be returned.
        """
        if iou_threshold is None: iou_threshold = self.non_maximum_suppression_threshold
        _raise_error_if_not_detection_sframe(dataset, self.feature, self.annotations,
                                             require_annotations=with_ground_truth)
        from ._sframe_loader import SFrameDetectionIter as _SFrameDetectionIter
        from ._detection import (yolo_map_to_bounding_boxes as _yolo_map_to_bounding_boxes,
                                 non_maximum_suppression as _non_maximum_suppression,
                                 bbox_to_ybox as _bbox_to_ybox)
        import mxnet as _mx
        loader = _SFrameDetectionIter(dataset,
                                      batch_size=self.batch_size,
                                      input_shape=self.input_image_shape[1:],
                                      output_shape=self._grid_shape,
                                      anchors=self.anchors,
                                      class_to_index=self._class_to_index,
                                      loader_type='stretched',
                                      load_labels=with_ground_truth,
                                      shuffle=False,
                                      epochs=1,
                                      feature_column=self.feature,
                                      annotations_column=self.annotations)

        num_anchors = len(self.anchors)
        preds_per_box = 5 + len(self.classes)
        output_size = preds_per_box * num_anchors

        # If prediction is done with ground truth, two sframes of the same
        # structure are returned, the second one containing ground truth labels
        num_returns = 2 if with_ground_truth else 1

        sf_builders = [
            _tc.SFrameBuilder([int, str, float, float, float, float, float],
                              column_names=['row_id', 'label', 'confidence',
                                            'x', 'y', 'width', 'height'])
            for _ in range(num_returns)
        ]

        num_mxnet_gpus = _mxnet_utils.get_num_gpus_in_use(max_devices=self.batch_size)
        use_mps = _use_mps() and num_mxnet_gpus == 0
        if use_mps:
            if not hasattr(self, '_mps_inference_net') or self._mps_inference_net is None:
                mxnet_params = self._model.collect_params()
                mps_net_params = { k : mxnet_params[k].data().asnumpy()
                                   for k in mxnet_params }
                mps_config = {
                    'mode': _MpsGraphMode.Inference,
                    'od_include_network': True,
                    'od_include_loss': False,
                }
                mps_net = _get_mps_od_net(input_image_shape=self.input_image_shape,
                                          batch_size=self.batch_size,
                                          output_size=output_size,
                                          anchors=self.anchors,
                                          config=mps_config,
                                          weights=mps_net_params)
                self._mps_inference_net = mps_net

        dataset_size = len(dataset)
        ctx = _mxnet_utils.get_mxnet_context()
        done = False
        last_time = 0
        raw_results = []
        for batch in loader:
            if batch.pad is not None:
                size = self.batch_size - batch.pad
                b_data = _mx.nd.slice_axis(batch.data[0], axis=0, begin=0, end=size)
                b_indices = _mx.nd.slice_axis(batch.label[1], axis=0, begin=0, end=size)
                b_oshapes = _mx.nd.slice_axis(batch.label[2], axis=0, begin=0, end=size)
            else:
                b_data = batch.data[0]
                b_indices = batch.label[1]
                b_oshapes = batch.label[2]
                size = self.batch_size

            if b_data.shape[0] < len(ctx):
                ctx0 = ctx[:b_data.shape[0]]
            else:
                ctx0 = ctx

            split_data = _mx.gluon.utils.split_and_load(b_data, ctx_list=ctx0, even_split=False)
            split_indices = _mx.gluon.utils.split_data(b_indices, num_slice=len(ctx0), even_split=False)
            split_oshapes = _mx.gluon.utils.split_data(b_oshapes, num_slice=len(ctx0), even_split=False)

            for data, indices, oshapes in zip(split_data, split_indices, split_oshapes):
                if use_mps:
                    mps_data = _mxnet_to_mps(data.asnumpy())
                    n_samples = mps_data.shape[0]
                    if mps_data.shape[0] != self.batch_size:
                        mps_data_padded = _np.zeros((self.batch_size,) + mps_data.shape[1:],
                                                    dtype=mps_data.dtype)
                        mps_data_padded[:mps_data.shape[0]] = mps_data
                        mps_data = mps_data_padded
                    mps_float_array = self._mps_inference_net.predict(mps_data)
                    mps_z = mps_float_array.asnumpy()[:n_samples]
                    z = _mps_to_mxnet(mps_z)
                else:
                    z = self._model(data).asnumpy()
                if not postprocess:
                    raw_results.append(z)
                    continue

                ypred = z.transpose(0, 2, 3, 1)
                ypred = ypred.reshape(ypred.shape[:-1] + (num_anchors, -1))

                zipped = zip(indices.asnumpy(), ypred, oshapes.asnumpy())
                for index0, output0, oshape0 in zipped:
                    index0 = int(index0)
                    x_boxes, x_classes, x_scores = _yolo_map_to_bounding_boxes(
                            output0[_np.newaxis], anchors=self.anchors,
                            confidence_threshold=confidence_threshold,
                            nms_thresh=None)

                    x_boxes0 = _np.array(x_boxes).reshape(-1, 4)

                    # Normalize
                    x_boxes0[:, 0::2] /= self.input_image_shape[1]
                    x_boxes0[:, 1::2] /= self.input_image_shape[2]

                    # Re-shape to original input size
                    x_boxes0[:, 0::2] *= oshape0[0]
                    x_boxes0[:, 1::2] *= oshape0[1]

                    # Clip the boxes to the original sizes
                    x_boxes0[:, 0::2] = _np.clip(x_boxes0[:, 0::2], 0, oshape0[0])
                    x_boxes0[:, 1::2] = _np.clip(x_boxes0[:, 1::2], 0, oshape0[1])

                    # Non-maximum suppression (also limit to 100 detection per
                    # image, inspired by the evaluation in COCO)
                    x_boxes0, x_classes, x_scores = _non_maximum_suppression(
                            x_boxes0, x_classes, x_scores,
                            num_classes=self.num_classes, threshold=iou_threshold,
                            limit=100)

                    for bbox, cls, s in zip(x_boxes0, x_classes, x_scores):
                        cls = int(cls)
                        values = [index0, self.classes[cls], s] + list(_bbox_to_ybox(bbox))
                        sf_builders[0].append(values)

                    if index0 == len(dataset) - 1:
                        done = True

                    cur_time = _time.time()
                    # Do not print process if only a few samples are predicted
                    if verbose and (dataset_size >= 5 and cur_time > last_time + 10 or done):
                        print('Predicting {cur_n:{width}d}/{max_n:{width}d}'.format(
                            cur_n=index0 + 1, max_n=dataset_size, width=len(str(dataset_size))))
                        last_time = cur_time

                    if done:
                        break

            # Ground truth
            if with_ground_truth:
                zipped = _itertools.islice(zip(batch.label[1].asnumpy(), batch.raw_bboxes, batch.raw_classes), size)
                for index0, bbox0, cls0 in zipped:
                    index0 = int(index0)
                    for bbox, cls in zip(bbox0, cls0):
                        cls = int(cls)
                        if cls == -1:
                            break
                        values = [index0, self.classes[cls], 1.0] + list(bbox)
                        sf_builders[1].append(values)

                    if index0 == len(dataset) - 1:
                        break

        if postprocess:
            ret = tuple([sb.close() for sb in sf_builders])
            if len(ret) == 1:
                return ret[0]
            else:
                return ret
        else:
            return _np.concatenate(raw_results, axis=0)

    def _raw_predict(self, dataset):
        return self._predict_with_options(dataset, with_ground_truth=False,
                                          postprocess=False)

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

    def predict(self, dataset, confidence_threshold=0.25, iou_threshold=None, verbose=True):
        """
        Predict object instances in an sframe of images.

        Parameters
        ----------
        dataset : SFrame | SArray | turicreate.Image
            The images on which to perform object detection.
            If dataset is an SFrame, it must have a column with the same name
            as the feature column during training. Additional columns are
            ignored.

        confidence_threshold : float
            Only return predictions above this level of confidence. The
            threshold can range from 0 to 1.

        iou_threshold : float
            Threshold value for non-maximum suppression. Non-maximum suppression
            prevents multiple bounding boxes appearing over a single object. 
            This threshold, set between 0 and 1, controls how aggressive this 
            suppression is. A value of 1 means no maximum suppression will 
            occur, while a value of 0 will maximally suppress neighboring 
            boxes around a prediction.

        verbose : bool
            If True, prints prediction progress.

        Returns
        -------
        out : SArray
            An SArray with model predictions. Each element corresponds to
            an image and contains a list of dictionaries. Each dictionary
            describes an object instances that was found in the image. If
            `dataset` is a single image, the return value will be a single
            prediction.

        See Also
        --------
        evaluate

        Examples
        --------
        .. sourcecode:: python

            # Make predictions
            >>> pred = model.predict(data)

            # Stack predictions, for a better overview
            >>> turicreate.object_detector.util.stack_annotations(pred)
            Data:
            +--------+------------+-------+-------+-------+-------+--------+
            | row_id | confidence | label |   x   |   y   | width | height |
            +--------+------------+-------+-------+-------+-------+--------+
            |   0    |    0.98    |  dog  | 123.0 | 128.0 |  80.0 | 182.0  |
            |   0    |    0.67    |  cat  | 150.0 | 183.0 | 129.0 | 101.0  |
            |   1    |    0.8     |  dog  |  50.0 | 432.0 |  65.0 |  98.0  |
            +--------+------------+-------+-------+-------+-------+--------+
            [3 rows x 7 columns]

            # Visualize predictions by generating a new column of marked up images
            >>> data['image_pred'] = turicreate.object_detector.util.draw_bounding_boxes(data['image'], data['predictions'])
        """
        _numeric_param_check_range('confidence_threshold', confidence_threshold, 0.0, 1.0)
        dataset, unpack = self._canonize_input(dataset)
        stacked_pred = self._predict_with_options(dataset, with_ground_truth=False,
                                                  confidence_threshold=confidence_threshold,
                                                  iou_threshold=iou_threshold,
                                                  verbose=verbose)

        from . import util
        return unpack(util.unstack_annotations(stacked_pred, num_rows=len(dataset)))

    def evaluate(self, dataset, metric='auto',
            output_type='dict', iou_threshold=None, 
            confidence_threshold=None, verbose=True):
        """
        Evaluate the model by making predictions and comparing these to ground
        truth bounding box annotations.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the annotations and feature used for model training.
            Additional columns are ignored.

        metric : str or list, optional
            Name of the evaluation metric or list of several names. The primary
            metric is average precision, which is the area under the
            precision/recall curve and reported as a value between 0 and 1 (1
            being perfect). Possible values are:

            - 'auto'                      : Returns all primary metrics.
            - 'all'                       : Returns all available metrics.
            - 'average_precision_50'      : Average precision per class with
                                            intersection-over-union threshold at
                                            50% (PASCAL VOC metric).
            - 'average_precision'         : Average precision per class calculated over multiple
                                            intersection-over-union thresholds
                                            (at 50%, 55%, ..., 95%) and averaged.
            - 'mean_average_precision_50' : Mean over all classes (for ``'average_precision_50'``).
                                            This is the primary single-value metric.
            - 'mean_average_precision'    : Mean over all classes (for ``'average_precision'``)

        output_type : str
            Type of output:

            - 'dict'      : You are given a dictionary where each key is a metric name and the
                            value is another dictionary containing class-to-metric entries.
            - 'sframe'    : All metrics are returned as a single `SFrame`, where each row is a
                            class and each column is a metric. Metrics that are averaged over
                            class cannot be returned and are ignored under this format.
                            However, these are easily computed from the `SFrame` (e.g.
                            ``results['average_precision'].mean()``).

        iou_threshold : float
            Threshold value for non-maximum suppression. Non-maximum suppression
            prevents multiple bounding boxes appearing over a single object. 
            This threshold, set between 0 and 1, controls how aggressive this 
            suppression is. A value of 1 means no maximum suppression will 
            occur, while a value of 0 will maximally suppress neighboring 
            boxes around a prediction.

        confidence_threshold : float
            Only return predictions above this level of confidence. The
            threshold can range from 0 to 1. 

        verbose : bool
            If True, prints evaluation progress.

        Returns
        -------
        out : dict / SFrame
            Output type depends on the option `output_type`.

        See Also
        --------
        create, predict

        Examples
        --------
        >>> results = model.evaluate(data)
        >>> print('mAP: {:.1%}'.format(results['mean_average_precision']))
        mAP: 43.2%
        """
        if iou_threshold is None: iou_threshold = self.non_maximum_suppression_threshold
        if confidence_threshold is None: confidence_threshold = 0.001

        AP = 'average_precision'
        MAP = 'mean_average_precision'
        AP50 = 'average_precision_50'
        MAP50 = 'mean_average_precision_50'
        ALL_METRICS = {AP, MAP, AP50, MAP50}
        if isinstance(metric, (list, tuple, set)):
            metrics = metric
        elif metric == 'all':
            metrics = ALL_METRICS
        elif metric == 'auto':
            metrics = {AP50, MAP50}
        elif metric in ALL_METRICS:
            metrics = {metric}
        else:
            raise _ToolkitError("Metric '{}' not supported".format(metric))

        pred, gt = self._predict_with_options(dataset, with_ground_truth=True,
                                              confidence_threshold=confidence_threshold,
                                              iou_threshold=iou_threshold,
                                              verbose=verbose)

        pred_df = pred.to_dataframe()
        gt_df = gt.to_dataframe()

        thresholds = _np.arange(0.5, 1.0, 0.05)
        all_th_aps = _average_precision(pred_df, gt_df,
                                        class_to_index=self._class_to_index,
                                        iou_thresholds=thresholds)

        def class_dict(aps):
            return {classname: aps[index]
                    for classname, index in self._class_to_index.items()}

        if output_type == 'dict':
            ret = {}
            if AP50 in metrics:
                ret[AP50] = class_dict(all_th_aps[0])
            if AP in metrics:
                ret[AP] = class_dict(all_th_aps.mean(0))
            if MAP50 in metrics:
                ret[MAP50] = all_th_aps[0].mean()
            if MAP in metrics:
                ret[MAP] = all_th_aps.mean()
        elif output_type == 'sframe':
            ret = _tc.SFrame({'label': self.classes})
            if AP50 in metrics:
                ret[AP50] = all_th_aps[0]
            if AP in metrics:
                ret[AP] = all_th_aps.mean(0)
        else:
            raise _ToolkitError("Output type '{}' not supported".format(output_type))

        return ret

    def export_coreml(self, filename, 
            include_non_maximum_suppression = True,
            iou_threshold = None,
            confidence_threshold = None):
        """
        Save the model in Core ML format. The Core ML model takes an image of
        fixed size as input and produces two output arrays: `confidence` and
        `coordinates`.

        The first one, `confidence` is an `N`-by-`C` array, where `N` is the
        number of instances predicted and `C` is the number of classes. The
        number `N` is fixed and will include many low-confidence predictions.
        The instances are not sorted by confidence, so the first one will
        generally not have the highest confidence (unlike in `predict`). Also
        unlike the `predict` function, the instances have not undergone
        what is called `non-maximum suppression`, which means there could be
        several instances close in location and size that have all discovered
        the same object instance. Confidences do not need to sum to 1 over the
        classes; any remaining probability is implied as confidence there is no
        object instance present at all at the given coordinates. The classes
        appear in the array alphabetically sorted.

        The second array `coordinates` is of size `N`-by-4, where the first
        dimension `N` again represents instances and corresponds to the
        `confidence` array. The second dimension represents `x`, `y`, `width`,
        `height`, in that order.  The values are represented in relative
        coordinates, so (0.5, 0.5) represents the center of the image and (1,
        1) the bottom right corner. You will need to multiply the relative
        values with the original image size before you resized it to the fixed
        input size to get pixel-value coordinates similar to `predict`.

        See Also
        --------
        save

        Parameters
        ----------
        filename : string
            The path of the file where we want to save the Core ML model.
       
        include_non_maximum_suppression : bool
            Non-maximum suppression is only available in iOS 12+.
            A boolean parameter to indicate whether the Core ML model should be
            saved with built-in non-maximum suppression or not. 
            This parameter is set to True by default.

        iou_threshold : float
            Threshold value for non-maximum suppression. Non-maximum suppression
            prevents multiple bounding boxes appearing over a single object. 
            This threshold, set between 0 and 1, controls how aggressive this 
            suppression is. A value of 1 means no maximum suppression will 
            occur, while a value of 0 will maximally suppress neighboring 
            boxes around a prediction.

        confidence_threshold : float
            Only return predictions above this level of confidence. The
            threshold can range from 0 to 1. 

        Examples
        --------
        >>> model.export_coreml('detector.mlmodel')
        """
        import mxnet as _mx
        from .._mxnet_to_coreml import _mxnet_converter
        import coremltools
        from coremltools.models import datatypes, neural_network

        if iou_threshold is None: iou_threshold = self.non_maximum_suppression_threshold
        if confidence_threshold is None: confidence_threshold = 0.25

        preds_per_box = 5 + self.num_classes
        num_anchors = len(self.anchors)
        num_classes = self.num_classes
        batch_size = 1
        image_shape = (batch_size,) + tuple(self.input_image_shape)
        s_image_uint8 = _mx.sym.Variable(self.feature, shape=image_shape, dtype=_np.float32)
        s_image = s_image_uint8 / 255

        # Swap a maxpool+slice in mxnet to a coreml natively supported layer
        from copy import copy
        net = copy(self._model)
        net._children = copy(self._model._children)
        from ._model import _SpecialDarknetMaxpoolBlock
        op = _SpecialDarknetMaxpoolBlock(name='pool5')
        # Make sure we are removing the right layers
        assert (self._model[23].name == 'pool5' and
                self._model[24].name == 'specialcrop5')
        del net._children[24]
        net._children[23] = op

        s_ymap = net(s_image)
        mod = _mx.mod.Module(symbol=s_ymap, label_names=None, data_names=[self.feature])
        mod.bind(for_training=False, data_shapes=[(self.feature, image_shape)])

        # Copy over params from net
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

        input_names = [self.feature]
        input_dims = [list(self.input_image_shape)]
        input_types = [datatypes.Array(*dim) for dim in input_dims]
        input_features = list(zip(input_names, input_types))

        num_spatial = self._grid_shape[0] * self._grid_shape[1]
        num_bounding_boxes = num_anchors * num_spatial
        CONFIDENCE_STR = ("raw_confidence" if include_non_maximum_suppression 
            else "confidence")
        COORDINATES_STR = ("raw_coordinates" if include_non_maximum_suppression 
            else "coordinates")
        output_names = [
            CONFIDENCE_STR,
            COORDINATES_STR
        ]
        output_dims = [
            (num_bounding_boxes, num_classes),
            (num_bounding_boxes, 4),
        ]
        output_types = [datatypes.Array(*dim) for dim in output_dims]
        output_features = list(zip(output_names, output_types))
        mode = None
        builder = neural_network.NeuralNetworkBuilder(input_features, output_features, mode)
        _mxnet_converter.convert(mod, mode=None,
                                 input_shape=[(self.feature, image_shape)],
                                 builder=builder, verbose=False)

        prefix = '__tc__internal__'

        # (1, B, C+5, S*S)
        builder.add_reshape(name=prefix + 'ymap_sp_pre',
                            target_shape=[batch_size, num_anchors, preds_per_box, num_spatial],
                            mode=0,
                            input_name='conv8_fwd_output',
                            output_name=prefix + 'ymap_sp_pre')

        # (1, C+5, B, S*S)
        builder.add_permute(name=prefix + 'ymap_sp',
                            dim=[0, 2, 1, 3],
                            input_name=prefix + 'ymap_sp_pre',
                            output_name=prefix + 'ymap_sp')

        # POSITION: X/Y

        # (1, 2, B, S*S)
        builder.add_slice(name=prefix + 'raw_rel_xy_sp',
                          axis='channel',
                          start_index=0,
                          end_index=2,
                          stride=1,
                          input_name=prefix + 'ymap_sp',
                          output_name=prefix + 'raw_rel_xy_sp')
        # (1, 2, B, S*S)
        builder.add_activation(name=prefix + 'rel_xy_sp',
                               non_linearity='SIGMOID',
                               input_name=prefix + 'raw_rel_xy_sp',
                               output_name=prefix + 'rel_xy_sp')

        # (1, 2, B*H*W, 1)
        builder.add_reshape(name=prefix + 'rel_xy',
                            target_shape=[batch_size, 2, num_bounding_boxes, 1],
                            mode=0,
                            input_name=prefix + 'rel_xy_sp',
                            output_name=prefix + 'rel_xy')

        c_xy = _np.array(_np.meshgrid(_np.arange(self._grid_shape[1]),
                                      _np.arange(self._grid_shape[0])), dtype=_np.float32)

        c_xy_reshaped = (_np.tile(c_xy[:, _np.newaxis], (num_anchors, 1, 1))
                         .reshape(2, -1))[_np.newaxis, ..., _np.newaxis]

        # (1, 2, B*H*W, 1)
        builder.add_load_constant(prefix + 'constant_xy',
                                  constant_value=c_xy_reshaped,
                                  shape=c_xy_reshaped.shape[1:],
                                  output_name=prefix + 'constant_xy')

        # (1, 2, B*H*W, 1)
        builder.add_elementwise(name=prefix + 'xy',
                                mode='ADD',
                                input_names=[prefix + 'constant_xy', prefix + 'rel_xy'],
                                output_name=prefix + 'xy')

        # SHAPE: WIDTH/HEIGHT

        # (1, 2, B, S*S)
        builder.add_slice(name=prefix + 'raw_rel_wh_sp',
                          axis='channel',
                          start_index=2,
                          end_index=4,
                          stride=1,
                          input_name=prefix + 'ymap_sp',
                          output_name=prefix + 'raw_rel_wh_sp')

        # (1, 2, B, S*S)
        builder.add_unary(name=prefix + 'rel_wh_sp',
                          mode='exp',
                          input_name=prefix + 'raw_rel_wh_sp',
                          output_name=prefix + 'rel_wh_sp')

        # (1, 2*B, S, S)
        builder.add_reshape(name=prefix + 'rel_wh',
                            target_shape=[batch_size, 2 * num_anchors] + list(self._grid_shape),
                            mode=0,
                            input_name=prefix + 'rel_wh_sp',
                            output_name=prefix + 'rel_wh')

        np_anchors = _np.asarray(self.anchors, dtype=_np.float32).T
        anchors_0 = _np.tile(np_anchors.reshape([2 * num_anchors, 1, 1]), self._grid_shape)

        # (1, 2*B, S, S)
        builder.add_load_constant(name=prefix + 'c_anchors',
                                  constant_value=anchors_0,
                                  shape=anchors_0.shape,
                                  output_name=prefix + 'c_anchors')

        # (1, 2*B, S, S)
        builder.add_elementwise(name=prefix + 'wh_pre',
                                mode='MULTIPLY',
                                input_names=[prefix + 'c_anchors', prefix + 'rel_wh'],
                                output_name=prefix + 'wh_pre')

        # (1, 2, B*H*W, 1)
        builder.add_reshape(name=prefix + 'wh',
                            target_shape=[1, 2, num_bounding_boxes, 1],
                            mode=0,
                            input_name=prefix + 'wh_pre',
                            output_name=prefix + 'wh')

        # (1, 4, B*H*W, 1)
        builder.add_elementwise(name=prefix + 'boxes_out_transposed',
                                mode='CONCAT',
                                input_names=[prefix + 'xy', prefix + 'wh'],
                                output_name=prefix + 'boxes_out_transposed')

        # (1, B*H*W, 4, 1)
        builder.add_permute(name=prefix + 'boxes_out',
                            dim=[0, 2, 1, 3],
                            input_name=prefix + 'boxes_out_transposed',
                            output_name=prefix + 'boxes_out')

        scale = _np.zeros((num_bounding_boxes, 4, 1))
        scale[:, 0::2] = 1.0 / self._grid_shape[1]
        scale[:, 1::2] = 1.0 / self._grid_shape[0]

        # (1, B*H*W, 4, 1)
        builder.add_scale(name=COORDINATES_STR,
                          W=scale,
                          b=0,
                          has_bias=False,
                          shape_scale=(num_bounding_boxes, 4, 1),
                          input_name=prefix + 'boxes_out',
                          output_name=COORDINATES_STR)

        # CLASS PROBABILITIES AND OBJECT CONFIDENCE

        # (1, C, B, H*W)
        builder.add_slice(name=prefix + 'scores_sp',
                          axis='channel',
                          start_index=5,
                          end_index=preds_per_box,
                          stride=1,
                          input_name=prefix + 'ymap_sp',
                          output_name=prefix + 'scores_sp')

        # (1, C, B, H*W)
        builder.add_softmax(name=prefix + 'probs_sp',
                            input_name=prefix + 'scores_sp',
                            output_name=prefix + 'probs_sp')

        # (1, 1, B, H*W)
        builder.add_slice(name=prefix + 'logit_conf_sp',
                          axis='channel',
                          start_index=4,
                          end_index=5,
                          stride=1,
                          input_name=prefix + 'ymap_sp',
                          output_name=prefix + 'logit_conf_sp')

        # (1, 1, B, H*W)
        builder.add_activation(name=prefix + 'conf_sp',
                               non_linearity='SIGMOID',
                               input_name=prefix + 'logit_conf_sp',
                               output_name=prefix + 'conf_sp')

        # (1, C, B, H*W)
        if num_classes > 1:
            conf = prefix + 'conf_tiled_sp'
            builder.add_elementwise(name=prefix + 'conf_tiled_sp',
                                    mode='CONCAT',
                                    input_names=[prefix+'conf_sp']*num_classes,
                                    output_name=conf)
        else:
            conf = prefix + 'conf_sp'

        # (1, C, B, H*W)
        builder.add_elementwise(name=prefix + 'confprobs_sp',
                                mode='MULTIPLY',
                                input_names=[conf, prefix + 'probs_sp'],
                                output_name=prefix + 'confprobs_sp')

        # (1, C, B*H*W, 1)
        builder.add_reshape(name=prefix + 'confprobs_transposed',
                            target_shape=[1, num_classes, num_bounding_boxes, 1],
                            mode=0,
                            input_name=prefix + 'confprobs_sp',
                            output_name=prefix + 'confprobs_transposed')

        # (1, B*H*W, C, 1)
        builder.add_permute(name=CONFIDENCE_STR,
                            dim=[0, 2, 1, 3],
                            input_name=prefix + 'confprobs_transposed',
                            output_name=CONFIDENCE_STR)

        _mxnet_converter._set_input_output_layers(
            builder, input_names, output_names)
        builder.set_input(input_names, input_dims)
        builder.set_output(output_names, output_dims)
        builder.set_pre_processing_parameters(image_input_names=self.feature)
        model = builder.spec

        if include_non_maximum_suppression:
            # Non-Maximum Suppression is a post-processing algorithm
            # responsible for merging all detections that belong to the
            # same object.
            #  Core ML schematic   
            #                        +------------------------------------+
            #                        | Pipeline                           |
            #                        |                                    |
            #                        |  +------------+   +-------------+  |
            #                        |  | Neural     |   | Non-maximum |  |
            #                        |  | network    +---> suppression +----->  confidences
            #               Image  +---->            |   |             |  |
            #                        |  |            +--->             +----->  coordinates
            #                        |  |            |   |             |  |
            # Optional inputs:       |  +------------+   +-^---^-------+  |
            #                        |                     |   |          |
            #    IOU threshold     +-----------------------+   |          |
            #                        |                         |          |
            # Confidence threshold +---------------------------+          |
            #                        +------------------------------------+

            model_neural_network = model.neuralNetwork
            model.specificationVersion = 3
            model.pipeline.ParseFromString(b'')
            model.pipeline.models.add()
            model.pipeline.models[0].neuralNetwork.ParseFromString(b'')
            model.pipeline.models.add()
            model.pipeline.models[1].nonMaximumSuppression.ParseFromString(b'')
            # begin: Neural network  model
            nn_model = model.pipeline.models[0]

            nn_model.description.ParseFromString(b'')
            input_image = model.description.input[0]
            input_image.type.imageType.width = self.input_image_shape[1]
            input_image.type.imageType.height = self.input_image_shape[2]
            nn_model.description.input.add()
            nn_model.description.input[0].ParseFromString(
                input_image.SerializeToString())

            for i in range(2):
                del model.description.output[i].type.multiArrayType.shape[:]
            names = ["raw_confidence", "raw_coordinates"]
            bounds = [self.num_classes, 4]
            for i in range(2):
                output_i = model.description.output[i]
                output_i.name = names[i]
                for j in range(2):
                    ma_type = output_i.type.multiArrayType
                    ma_type.shapeRange.sizeRanges.add()
                    ma_type.shapeRange.sizeRanges[j].lowerBound = (
                        bounds[i] if j == 1 else 0)
                    ma_type.shapeRange.sizeRanges[j].upperBound = (
                        bounds[i] if j == 1 else -1)
                nn_model.description.output.add()
                nn_model.description.output[i].ParseFromString(
                    output_i.SerializeToString())

                ma_type = nn_model.description.output[i].type.multiArrayType
                ma_type.shape.append(num_bounding_boxes)
                ma_type.shape.append(bounds[i])
            
            # Think more about this line
            nn_model.neuralNetwork.ParseFromString(
                model_neural_network.SerializeToString())
            nn_model.specificationVersion = model.specificationVersion
            # end: Neural network  model

            # begin: Non maximum suppression model
            nms_model = model.pipeline.models[1]
            nms_model_nonMaxSup = nms_model.nonMaximumSuppression
            
            for i in range(2):
                output_i = model.description.output[i]
                nms_model.description.input.add()
                nms_model.description.input[i].ParseFromString(
                    output_i.SerializeToString())

                nms_model.description.output.add()
                nms_model.description.output[i].ParseFromString(
                    output_i.SerializeToString())
                nms_model.description.output[i].name = (
                    'confidence' if i==0 else 'coordinates')
            
            nms_model_nonMaxSup.iouThreshold = iou_threshold
            nms_model_nonMaxSup.confidenceThreshold = confidence_threshold
            nms_model_nonMaxSup.confidenceInputFeatureName = 'raw_confidence'
            nms_model_nonMaxSup.coordinatesInputFeatureName = 'raw_coordinates'
            nms_model_nonMaxSup.confidenceOutputFeatureName = 'confidence'
            nms_model_nonMaxSup.coordinatesOutputFeatureName = 'coordinates'
            nms_model.specificationVersion = model.specificationVersion
            nms_model_nonMaxSup.stringClassLabels.vector.extend(self.classes)

            for i in range(2):
                nms_model.description.input[i].ParseFromString(
                    nn_model.description.output[i].SerializeToString()
                )

            if include_non_maximum_suppression:
                # Iou Threshold
                IOU_THRESHOLD_STRING = 'iouThreshold'
                model.description.input.add()
                model.description.input[1].type.doubleType.ParseFromString(b'')
                model.description.input[1].name = IOU_THRESHOLD_STRING
                nms_model.description.input.add()
                nms_model.description.input[2].ParseFromString(
                    model.description.input[1].SerializeToString()
                )
                nms_model_nonMaxSup.iouThresholdInputFeatureName = IOU_THRESHOLD_STRING
                
                # Confidence Threshold
                CONFIDENCE_THRESHOLD_STRING = 'confidenceThreshold'
                model.description.input.add()
                model.description.input[2].type.doubleType.ParseFromString(b'')
                model.description.input[2].name = CONFIDENCE_THRESHOLD_STRING

                nms_model.description.input.add()
                nms_model.description.input[3].ParseFromString(
                    model.description.input[2].SerializeToString())

                nms_model_nonMaxSup.confidenceThresholdInputFeatureName = \
                    CONFIDENCE_THRESHOLD_STRING
                
            # end: Non maximum suppression model
            model.description.output[0].name = 'confidence'
            model.description.output[1].name = 'coordinates'

        iouThresholdString = '(optional) IOU Threshold override (default: {})'
        confidenceThresholdString = ('(optional)' + 
            ' Confidence Threshold override (default: {})')
        model_type = 'object detector (%s)' % self.model
        if include_non_maximum_suppression:
            model_type += ' with non-maximum suppression'
        model.description.metadata.shortDescription = \
            _coreml_utils._mlmodel_short_description(model_type)
        model.description.input[0].shortDescription = 'Input image'
        if include_non_maximum_suppression:
            iouThresholdString = '(optional) IOU Threshold override (default: {})'
            model.description.input[1].shortDescription = \
                iouThresholdString.format(iou_threshold)
            confidenceThresholdString = ('(optional)' + 
                ' Confidence Threshold override (default: {})')
            model.description.input[2].shortDescription = \
                confidenceThresholdString.format(confidence_threshold)
        model.description.output[0].shortDescription = \
            u'Boxes \xd7 Class confidence (see user-defined metadata "classes")'
        model.description.output[1].shortDescription = \
            u'Boxes \xd7 [x, y, width, height] (relative to image size)'
        version = ObjectDetector._PYTHON_OBJECT_DETECTOR_VERSION
        partial_user_defined_metadata = {
            'model': self.model,
            'max_iterations': str(self.max_iterations),
            'training_iterations': str(self.training_iterations),
            'include_non_maximum_suppression': str(
                include_non_maximum_suppression),
            'non_maximum_suppression_threshold': str(
                iou_threshold),
            'confidence_threshold': str(confidence_threshold),
            'iou_threshold': str(iou_threshold),
            'feature': self.feature,
            'annotations': self.annotations,
            'classes': ','.join(self.classes)
        }
        user_defined_metadata = _coreml_utils._get_model_metadata(
            self.__class__.__name__,
            partial_user_defined_metadata,
            version)
        model.description.metadata.userDefined.update(user_defined_metadata)
        from coremltools.models.utils import save_spec as _save_spec
        _save_spec(model, filename)
