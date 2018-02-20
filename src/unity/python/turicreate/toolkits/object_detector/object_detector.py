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

_MXNET_MODEL_FILENAME = "mxnet_model.params"


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
           classes=None, max_iterations=0, verbose=True, **kwargs):
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
        'batch_size': 32,
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
        'learning_rate': 1.0e-3,
        'shuffle': True,
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

    num_gpus = _mxnet_utils.get_num_gpus_in_use(max_devices=params['batch_size'])
    batch_size_each = params['batch_size'] // max(num_gpus, 1)
    # Note, this may slightly alter the batch size to fit evenly on the GPUs
    batch_size = max(num_gpus, 1) * batch_size_each

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
                                  annotations_column=annotations)

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
    if max_iterations == 0:
        # Set number of iterations through a heuristic
        num_iterations_raw = 5000 * _np.sqrt(num_instances) / batch_size
        num_iterations = 1000 * max(1, int(round(num_iterations_raw / 1000)))
    else:
        num_iterations = max_iterations

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

    options = {'learning_rate': base_lr, 'lr_scheduler': lr_scheduler,
               'momentum': 0.9, 'wd': 0.00005, 'rescale_grad': 1.0}
    clip_grad = params.get('clip_gradients')
    if clip_grad:
        options['clip_gradient'] = clip_grad

    trainer = _mx.gluon.Trainer(net.collect_params(), 'sgd', options)

    iteration = 0
    smoothed_loss = None
    last_time = 0
    while iteration < num_iterations:
        loader.reset()
        for batch in loader:
            data = _mx.gluon.utils.split_and_load(batch.data[0], ctx_list=ctx, batch_axis=0)
            label = _mx.gluon.utils.split_and_load(batch.label[0], ctx_list=ctx, batch_axis=0)

            Ls = []
            with _mx.autograd.record():
                for x, y in zip(data, label):
                    z = net(x)
                    z0 = _mx.nd.transpose(z, [0, 2, 3, 1]).reshape(ymap_shape)
                    L = loss(z0, y)
                    Ls.append(L)
                for L in Ls:
                    L.backward()

            cur_loss = _np.mean([L.asnumpy()[0] for L in Ls])
            if smoothed_loss is None:
                smoothed_loss = cur_loss
            else:
                smoothed_loss = 0.9 * smoothed_loss + 0.1 * cur_loss
            trainer.step(1)
            iteration += 1
            cur_time = _time.time()
            if verbose and cur_time > last_time + 10:
                print('{now:%Y-%m-%d %H:%M:%S}  Training {cur_iter:{width}d}/{num_iterations:{width}d}  Loss {loss:6.3f}'.format(
                    now=_datetime.now(), cur_iter=iteration, num_iterations=num_iterations,
                    loss=smoothed_loss, width=len(str(num_iterations))))
                last_time = cur_time
            if iteration == num_iterations:
                break

    training_time = _time.time() - start_time

    # Save the model
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
        'training_epochs': loader.cur_epoch,
        'training_iterations': iteration,
        'max_iterations': max_iterations,
        'training_loss': smoothed_loss,
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
                              verbose=True):
        """
        Predict with options for what kind of SFrame should be returned.

        If postprocess is False, a single numpy array with raw unprocessed
        results will be returned.
        """
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

        # If prediction is done with ground truth, two sframes of the same
        # structure are returned, the second one containing ground truth labels
        num_returns = 2 if with_ground_truth else 1

        sf_builders = [
            _tc.SFrameBuilder([int, str, float, float, float, float, float],
                              column_names=['row_id', 'label', 'confidence',
                                            'x', 'y', 'width', 'height'])
            for _ in range(num_returns)
        ]

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
                            num_classes=self.num_classes, threshold=self.non_maximum_suppression_threshold,
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

    def predict(self, dataset, confidence_threshold=0.25, verbose=True):
        """
        Predict object instances in an sframe of images.

        Parameters
        ----------
        dataset : SFrame
            A dataset that has the same columns that were used during training.
            If the annotations column exists in ``dataset`` it will be ignored
            while making predictions.

        confidence_threshold : float
            Only return predictions above this level of confidence. The
            threshold can range from 0 to 1.

        verbose : bool
            If True, prints prediction progress.

        Returns
        -------
        out : SArray
            An SArray with model predictions. Each element corresponds to
            an image and contains a list of dictionaries. Each dictionary
            describes an object instances that was found in the image.

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
        stacked_pred = self._predict_with_options(dataset, with_ground_truth=False,
                                                  confidence_threshold=confidence_threshold,
                                                  verbose=verbose)

        from . import util
        return util.unstack_annotations(stacked_pred, num_rows=len(dataset))

    def evaluate(self, dataset, metric='auto', output_type='dict', verbose=True):
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
            - 'average_precision'         : Average precision per class calculated over multiple
                                            intersection-over-union thresholds
                                            (at 50%, 55%, ..., 95%) and averaged.
            - 'average_precision_50'      : Average precision per class with
                                            intersection-over-union threshold at
                                            50% (PASCAL VOC metric).
            - 'mean_average_precision'    : Mean over all classes (for ``'average_precision'``)
                                            This is the primary single-value metric.
            - 'mean_average_precision_50' : Mean over all classes (for ``'average_precision_50'``).

        output_type : str
            Type of output:

            - 'dict'      : You are given a dictionary where each key is a metric name and the
                            value is another dictionary containing class-to-metric entries.
            - 'sframe'    : All metrics are returned as a single `SFrame`, where each row is a
                            class and each column is a metric. Metrics that are averaged over
                            class cannot be returned and are ignored under this format.
                            However, these are easily computed from the `SFrame` (e.g.
                            ``results['average_precision'].mean()``).

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
            metrics = {AP, MAP}
        elif metric in ALL_METRICS:
            metrics = {metric}
        else:
            raise _ToolkitError("Metric '{}' not supported".format(metric))

        pred, gt = self._predict_with_options(dataset, with_ground_truth=True,
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

    def export_coreml(self, filename):
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

        Examples
        --------
        >>> model.export_coreml('detector.mlmodel')
        """
        import mxnet as _mx
        from .._mxnet_to_coreml import _mxnet_converter
        import coremltools
        from coremltools.models import datatypes, neural_network

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
        output_names = [
            'confidence',
            'coordinates',
        ]
        output_dims = [
            (num_anchors * num_spatial, num_classes),
            (num_anchors * num_spatial, 4),
        ]
        output_types = [datatypes.Array(*dim) for dim in output_dims]
        output_features = list(zip(output_names, output_types))
        mode = None
        builder = neural_network.NeuralNetworkBuilder(input_features, output_features, mode)
        _mxnet_converter.convert(mod, mode=None,
                                 input_shape={self.feature: image_shape},
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
                            target_shape=[batch_size, 2, num_anchors * num_spatial, 1],
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
                            target_shape=[1, 2, num_anchors * num_spatial, 1],
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

        scale = _np.zeros((num_anchors * num_spatial, 4, 1))
        scale[:, 0::2] = 1.0 / self._grid_shape[1]
        scale[:, 1::2] = 1.0 / self._grid_shape[0]

        # (1, B*H*W, 4, 1)
        builder.add_scale(name='coordinates',
                          W=scale,
                          b=0,
                          has_bias=False,
                          shape_scale=(num_anchors * num_spatial, 4, 1),
                          input_name=prefix + 'boxes_out',
                          output_name='coordinates')

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
                                    input_names=[prefix + 'conf_sp'] * num_classes,
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
                            target_shape=[1, num_classes, num_anchors * num_spatial, 1],
                            mode=0,
                            input_name=prefix + 'confprobs_sp',
                            output_name=prefix + 'confprobs_transposed')

        # (1, B*H*W, C, 1)
        builder.add_permute(name='confidence',
                            dim=[0, 2, 1, 3],
                            input_name=prefix + 'confprobs_transposed',
                            output_name='confidence')

        _mxnet_converter._set_input_output_layers(builder, input_names, output_names)
        builder.set_input(input_names, input_dims)
        builder.set_output(output_names, output_dims)
        builder.set_pre_processing_parameters(image_input_names=self.feature)
        mlmodel = coremltools.models.MLModel(builder.spec)
        model_type = 'object detector (%s)' % self.model
        mlmodel.short_description = _coreml_utils._mlmodel_short_description(model_type)
        mlmodel.input_description[self.feature] = 'Input image'
        mlmodel.output_description['confidence'] = \
                u'Boxes \xd7 Class confidence (see user-defined metadata "classes")'
        mlmodel.output_description['coordinates'] = \
                u'Boxes \xd7 [x, y, width, height] (relative to image size)'
        _coreml_utils._set_model_metadata(mlmodel, self.__class__.__name__, {
                'model': self.model,
                'max_iterations': str(self.max_iterations),
                'training_iterations': str(self.training_iterations),
                'non_maximum_suppression_threshold': str(self.non_maximum_suppression_threshold),
                'feature': self.feature,
                'annotations': self.annotations,
                'classes': ','.join(self.classes),
            }, version=ObjectDetector._PYTHON_OBJECT_DETECTOR_VERSION)
        mlmodel.save(filename)
