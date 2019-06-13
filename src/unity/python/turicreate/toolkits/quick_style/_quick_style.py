# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import time as _time
from datetime import datetime as _datetime

import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits import _coreml_utils
from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe, _mac_ver
from ._utils import _seconds_as_string
from .. import _pre_trained_models
from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._model import PythonProxy as _PythonProxy
import turicreate as _tc
import numpy as _np
import math as _math
import six as _six

def create(style_dataset, style_feature=None, batch_size = 1, **kwargs):
    """
    Create a :class:`QuickStyle` model.

    Parameters
    ----------
    style_dataset: SFrame
        Input style images. The columns named by the ``style_feature`` parameters will
        be extracted for training the model.

    style_feature: string
        Name of the column containing the input images in style SFrame.
        'None' (the default) indicates the only image column in the style SFrame
        should be used as the feature.

    batch_size : int, optional
        If you are getting memory errors, try decreasing this value. If you
        have a powerful computer, increasing this value may improve training
        throughput.

    verbose : bool, optional
        If True, print progress updates and model details.


    Returns
    -------
    out : QuickStyle
        A trained :class:`QuickStyle` model.

    See Also
    --------
    QuickStyle

    Examples
    --------
    .. sourcecode:: python

        # Create datasets
        >>> style_dataset = turicreate.image_analysis.load_images('style_images/')

        # Train a quick style model
        >>> model = turicreate.quick_style.create(style_dataset)

        # Stylize an image on all styles
        >>> stylized_images = model.stylize(data)

        # Visualize the stylized images
        >>> stylized_images.explore()

    """
    if len(style_dataset) == 0:
        raise _ToolkitError("style_dataset SFrame cannot be empty")
    if(batch_size < 1):
        raise _ToolkitError("'batch_size' must be greater than or equal to 1")

    from ._sframe_loader import SFrameSTIter as _SFrameSTIter
    import mxnet as _mx
    from .._mxnet import _mxnet_utils

    if style_feature is None:
        style_feature = _tkutl._find_only_image_column(style_dataset)
    if verbose:
        print("Using '{}' in style_dataset as feature column and using ".format(style_feature))

    _raise_error_if_not_training_sframe(style_dataset, style_feature)

    # delete additional parameters
    params = {
        'batch_size': batch_size,
        'input_shape': (256, 256)
    }

    num_gpus = _mxnet_utils.get_num_gpus_in_use(max_devices=params['batch_size'])
    batch_size_each = params['batch_size'] // max(num_gpus, 1)
    batch_size = max(num_gpus, 1) * batch_size_each
    input_shape = params['input_shape']

    ctx = _mxnet_utils.get_mxnet_context(max_devices=params['batch_size'])
    num_styles = len(style_dataset)

    style_images_loader = _SFrameSTIter(style_dataset, batch_size, shuffle=False, num_epochs=1,
                                        feature_column=style_feature, input_shape=input_shape,
                                        loader_type='stretch',
                                        sequential=True)


    import _model

    featurizer = _model.inception_v3(pretrained=True, ctx=ctx)
    bottle_neck = _model.bottle_neck(pretrained=True, ctx=ctx)
    parameters_bg = _model.style_params(pretrained=True, ctx=ctx)
    transformer = _model.transformer(pretrained=True, num_styles=num_styles, ctx=ctx)

    start_time = _time.time()

    transformer_parameters = {
        "transformer_residualblock0_instancenorm0_gamma": [],
        "transformer_residualblock0_instancenorm0_beta": [],
        "transformer_residualblock0_instancenorm1_gamma": [],
        "transformer_residualblock0_instancenorm1_beta": [],
        "transformer_residualblock1_instancenorm0_gamma": [],
        "transformer_residualblock1_instancenorm0_beta": [],
        "transformer_residualblock1_instancenorm1_gamma": [],
        "transformer_residualblock1_instancenorm1_beta": [],
        "transformer_residualblock2_instancenorm0_gamma": [],
        "transformer_residualblock2_instancenorm0_beta": [],
        "transformer_residualblock2_instancenorm1_gamma": [],
        "transformer_residualblock2_instancenorm1_beta": [],
        "transformer_residualblock3_instancenorm0_gamma": [],
        "transformer_residualblock3_instancenorm0_beta": [],
        "transformer_residualblock3_instancenorm1_gamma": [],
        "transformer_residualblock3_instancenorm1_beta": [],
        "transformer_residualblock4_instancenorm0_gamma": [],
        "transformer_residualblock4_instancenorm0_beta": [],
        "transformer_residualblock4_instancenorm1_gamma": [],
        "transformer_residualblock4_instancenorm1_beta": [],
        "transformer_instancenorm0_gamma": [],
        "transformer_instancenorm0_beta": [],
        "transformer_instancenorm1_gamma": [],
        "transformer_instancenorm1_beta": [],
        "transformer_instancenorm2_gamma": [],
        "transformer_instancenorm2_beta": []
    }

    for s_batch in style_images_loader:
        s_data = _gluon.utils.split_and_load(s_batch.data[0], ctx_list=ctx, batch_axis=0)
        for s in s_data:
            f = featurizer(s)
            b = bottle_neck(f)
            p = parameters_bg(b)

            transformer_parameters["transformer_residualblock0_instancenorm0_gamma"].append(p["residual1_conv_1_gamma"][0])
            transformer_parameters["transformer_residualblock0_instancenorm0_beta"].append(p["residual1_conv_1_beta"][0])
            transformer_parameters["transformer_residualblock0_instancenorm1_gamma"].append(p["residual1_conv_2_gamma"][0])
            transformer_parameters["transformer_residualblock0_instancenorm1_beta"].append(p["residual1_conv_2_beta"][0])

            transformer_parameters["transformer_residualblock1_instancenorm0_gamma"].append(p["residual2_conv_1_gamma"][0])
            transformer_parameters["transformer_residualblock1_instancenorm0_beta"].append(p["residual2_conv_1_beta"][0])
            transformer_parameters["transformer_residualblock1_instancenorm1_gamma"].append(p["residual2_conv_2_gamma"][0])
            transformer_parameters["transformer_residualblock1_instancenorm1_beta"].append(p["residual2_conv_2_beta"][0])

            transformer_parameters["transformer_residualblock2_instancenorm0_gamma"].append(p["residual3_conv_1_gamma"][0])
            transformer_parameters["transformer_residualblock2_instancenorm0_beta"].append(p["residual3_conv_1_beta"][0])
            transformer_parameters["transformer_residualblock2_instancenorm1_gamma"].append(p["residual3_conv_2_gamma"][0])
            transformer_parameters["transformer_residualblock2_instancenorm1_beta"].append(p["residual3_conv_2_beta"][0])

            transformer_parameters["transformer_residualblock3_instancenorm0_gamma"].append(p["residual4_conv_1_gamma"][0])
            transformer_parameters["transformer_residualblock3_instancenorm0_beta"].append(p["residual4_conv_1_beta"][0])
            transformer_parameters["transformer_residualblock3_instancenorm1_gamma"].append(p["residual4_conv_2_gamma"][0])
            transformer_parameters["transformer_residualblock3_instancenorm1_beta"].append(p["residual4_conv_2_beta"][0])

            transformer_parameters["transformer_residualblock4_instancenorm0_gamma"].append(p["residual5_conv_1_gamma"][0])
            transformer_parameters["transformer_residualblock4_instancenorm0_beta"].append(p["residual5_conv_1_beta"][0])
            transformer_parameters["transformer_residualblock4_instancenorm1_gamma"].append(p["residual5_conv_2_gamma"][0])
            transformer_parameters["transformer_residualblock4_instancenorm1_beta"].append(p["residual5_conv_2_beta"][0])

            transformer_parameters["transformer_instancenorm0_gamma"].append(p["expand1_conv_1_gamma"][0])
            transformer_parameters["transformer_instancenorm0_beta"].append(p["expand1_conv_1_beta"][0])

            transformer_parameters["transformer_instancenorm1_gamma"].append(p["expand2_conv_1_gamma"][0])
            transformer_parameters["transformer_instancenorm1_beta"].append(p["expand2_conv_1_beta"][0])

            transformer_parameters["transformer_instancenorm2_gamma"].append(p["expand3_conv_1_gamma"][0])
            transformer_parameters["transformer_instancenorm2_beta"].append(p["expand3_conv_1_beta"][0])

    del style_images_loader

    transformer.collect_params()["transformer_residualblock0_instancenorm0_gamma"]._data[0] = transformer_parameters["transformer_residualblock0_instancenorm0_gamma"]
    transformer.collect_params()["transformer_residualblock0_instancenorm0_beta"]._data[0] = transformer_parameters["transformer_residualblock0_instancenorm0_beta"]
    transformer.collect_params()["transformer_residualblock0_instancenorm1_gamma"]._data[0] = transformer_parameters["transformer_residualblock0_instancenorm1_gamma"]
    transformer.collect_params()["transformer_residualblock0_instancenorm1_beta"]._data[0] = transformer_parameters["transformer_residualblock0_instancenorm1_beta"]

    transformer.collect_params()["transformer_residualblock1_instancenorm0_gamma"]._data[0] = transformer_parameters["transformer_residualblock1_instancenorm0_gamma"]
    transformer.collect_params()["transformer_residualblock1_instancenorm0_beta"]._data[0] = transformer_parameters["transformer_residualblock1_instancenorm0_beta"]
    transformer.collect_params()["transformer_residualblock1_instancenorm1_gamma"]._data[0] = transformer_parameters["transformer_residualblock1_instancenorm1_gamma"]
    transformer.collect_params()["transformer_residualblock1_instancenorm1_beta"]._data[0] = transformer_parameters["transformer_residualblock1_instancenorm1_beta"]

    transformer.collect_params()["transformer_residualblock2_instancenorm0_gamma"]._data[0] = transformer_parameters["transformer_residualblock2_instancenorm0_gamma"]
    transformer.collect_params()["transformer_residualblock2_instancenorm0_beta"]._data[0] = transformer_parameters["transformer_residualblock2_instancenorm0_beta"]
    transformer.collect_params()["transformer_residualblock2_instancenorm1_gamma"]._data[0] = transformer_parameters["transformer_residualblock2_instancenorm1_gamma"]
    transformer.collect_params()["transformer_residualblock2_instancenorm1_beta"]._data[0] = transformer_parameters["transformer_residualblock2_instancenorm1_beta"]

    transformer.collect_params()["transformer_residualblock3_instancenorm0_gamma"]._data[0] = transformer_parameters["transformer_residualblock3_instancenorm0_gamma"]
    transformer.collect_params()["transformer_residualblock3_instancenorm0_beta"]._data[0] = transformer_parameters["transformer_residualblock3_instancenorm0_beta"]
    transformer.collect_params()["transformer_residualblock3_instancenorm1_gamma"]._data[0] = transformer_parameters["transformer_residualblock3_instancenorm1_gamma"]
    transformer.collect_params()["transformer_residualblock3_instancenorm1_beta"]._data[0] = transformer_parameters["transformer_residualblock3_instancenorm1_beta"]

    transformer.collect_params()["transformer_residualblock4_instancenorm0_gamma"]._data[0] = transformer_parameters["transformer_residualblock4_instancenorm0_gamma"]
    transformer.collect_params()["transformer_residualblock4_instancenorm0_beta"]._data[0] = transformer_parameters["transformer_residualblock4_instancenorm0_beta"]
    transformer.collect_params()["transformer_residualblock4_instancenorm1_gamma"]._data[0] = transformer_parameters["transformer_residualblock4_instancenorm1_gamma"]
    transformer.collect_params()["transformer_residualblock4_instancenorm1_beta"]._data[0] = transformer_parameters["transformer_residualblock4_instancenorm1_beta"]

    transformer.collect_params()["transformer_instancenorm0_gamma"]._data[0] = transformer_parameters["transformer_instancenorm0_gamma"]
    transformer.collect_params()["transformer_instancenorm0_beta"]._data[0] = transformer_parameters["transformer_instancenorm0_beta"]

    transformer.collect_params()["transformer_instancenorm1_gamma"]._data[0] = transformer_parameters["transformer_instancenorm1_gamma"]
    transformer.collect_params()["transformer_instancenorm1_beta"]._data[0] = transformer_parameters["transformer_instancenorm1_beta"]

    transformer.collect_params()["transformer_instancenorm2_gamma"]._data[0] = transformer_parameters["transformer_instancenorm2_gamma"]
    transformer.collect_params()["transformer_instancenorm2_beta"]._data[0] = transformer_parameters["transformer_instancenorm2_beta"]

    style_sa = style_dataset[style_feature]
    idx_column = _tc.SArray(range(0, style_sa.shape[0]))
    style_sframe = _tc.SFrame({"style": idx_column, style_feature: style_sa})

    training_time = _time.time() - start_time

    state = {
        '_model': transformer,
        '_training_time_as_string': _seconds_as_string(training_time),
        'batch_size': batch_size,
        'num_styles': num_styles,
        'input_image_shape': input_shape,
        'styles': style_sframe,
        'training_time': training_time,
        'style_feature': style_feature,
        "_index_column": "style"
    }

    return QuickStyle(state)


def _raise_error_if_not_training_sframe(dataset, context_column):
    _raise_error_if_not_sframe(dataset, 'datset')
    if context_column not in dataset.column_names():
        raise _ToolkitError("Context Image column '%s' does not exist"
            % context_column)
    if dataset[context_column].dtype != _tc.Image:
        raise _ToolkitError("Context Image column must contain images")


class QuickStyle(_CustomModel):
    """
    An trained model that is ready to use for quick style transfer, exported to
    Core ML.

    This model should not be constructed directly.
    """

    _PYTHON_QUICK_STYLE_VERSION = 1

    def __init__(self, state=None):
        if(state != None):
            self.__proxy__ = _PythonProxy(state)

    @classmethod
    def _native_name(cls):
        return "quick_style"

    def _get_native_state(self):
        from .._mxnet import _mxnet_utils
        state = self.__proxy__.get_state()
        mxnet_params = state['_model'].collect_params()
        state['_model'] = _mxnet_utils.get_gluon_net_params_state(mxnet_params)
        return state

    def _get_version(self):
        return self._PYTHON_QUICK_STYLE_VERSION
    
    @classmethod
    def _load_version(cls, state, version):
        from _model import Transformer as _Transformer
        from .._mxnet import _mxnet_utils

        _tkutl._model_version_check(version, cls._PYTHON_QUICK_STYLE_VERSION)

        net = _Transformer(state['num_styles'], state['batch_size'])
        ctx = _mxnet_utils.get_mxnet_context(max_devices=state['batch_size'])

        net_params = net.collect_params()
        _mxnet_utils.load_net_params_from_state(net_params, state['_model'], ctx=ctx)
        state['_model'] = net
        state['input_image_shape'] = tuple([int(i) for i in state['input_image_shape']])
        return QuickStyle(state)

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the QuickStyle.
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
            ('Number of unique styles', 'num_styles'),
            ('Input image shape', 'input_image_shape'),
        ]

        training_fields = [
            ('Training time', '_training_time_as_string'),
            ('Number of style images', 'num_styles'),
        ]

        section_titles = ['Schema', 'Training summary']
        return([model_fields, training_fields], section_titles)

    def _style_indices(self):
        set_of_all_idx = set(self.styles[self._index_column])
        return set_of_all_idx

    def _style_input_check(self, style):
        set_of_all_idx = self._style_indices()
        scalar = False
        if isinstance(style, (list, tuple)):
            if len(style) == 0:
                raise _ToolkitError("the `style` list cannot be empty")
            elif set(style).issubset(set_of_all_idx):
                pass
            else:
                raise _ToolkitError("the `style` variable cannot be parsed")
        elif isinstance(style, _six.integer_types):
            scalar = True
            if style in set_of_all_idx:
                style = [style]
            else:
                raise _ToolkitError("the `style` variable cannot be parsed")
        elif style is None:
            style = list(set_of_all_idx)
        else:
            raise _ToolkitError("the `style` variable cannot be parsed")

        return style, scalar

    def _canonize_content_input(self, dataset, single_style):
        """
        Takes input and returns tuple of the input in canonical form (SFrame)
        along with an unpack callback function that can be applied to
        prediction results to "undo" the canonization.
        """
        unpack = lambda x: x
        if isinstance(dataset, _tc.SArray):
            dataset = _tc.SFrame({self.content_feature: dataset})
            if single_style:
                unpack = lambda sf: sf['stylized_' + self.content_feature]
        elif isinstance(dataset, _tc.Image):
            dataset = _tc.SFrame({self.content_feature: [dataset]})
            if single_style:
                unpack = lambda sf: sf['stylized_' + self.content_feature][0]
        return dataset, unpack

    def stylize(self, images, style=None, verbose=True, max_size=800, batch_size = 4):
        """
        Stylize an SFrame of Images given a style index or a list of
        styles.

        Parameters
        ----------
        images : SFrame | Image
            A dataset that has the same content image column that was used
            during training.

        style : int or list, optional
            The selected style or list of styles to use on the ``images``. If
            `None`, all styles will be applied to each image in ``images``.

        verbose : bool, optional
            If True, print progress updates.

        max_size : int or tuple
            Max input image size that will not get resized during stylization.

            Images with a side larger than this value, will be scaled down, due
            to time and memory constraints. If tuple, interpreted as (max
            width, max height). Without resizing, larger input images take more
            time to stylize.  Resizing can effect the quality of the final
            stylized image.

        batch_size : int, optional
            If you are getting memory errors, try decreasing this value. If you
            have a powerful computer, increasing this value may improve
            performance.

        Returns
        -------
        out : SFrame or SArray or turicreate.Image
            If ``style`` is a list, an SFrame is always returned. If ``style``
            is a single integer, the output type will match the input type
            (Image, SArray, or SFrame).

        See Also
        --------
        create

        Examples
        --------
        >>> image = tc.Image("/path/to/image.jpg")
        >>> stylized_images = model.stylize(image, style=[0, 1])
        Data:
        +--------+-------+------------------------+
        | row_id | style |     stylized_image     |
        +--------+-------+------------------------+
        |   0    |   0   | Height: 256 Width: 256 |
        |   0    |   1   | Height: 256 Width: 256 |
        +--------+-------+------------------------+
        [2 rows x 3 columns]

        >>> images = tc.image_analysis.load_images('/path/to/images')
        >>> stylized_images = model.stylize(images)
        Data:
        +--------+-------+------------------------+
        | row_id | style |     stylized_image     |
        +--------+-------+------------------------+
        |   0    |   0   | Height: 256 Width: 256 |
        |   0    |   1   | Height: 256 Width: 256 |
        |   0    |   2   | Height: 256 Width: 256 |
        |   0    |   3   | Height: 256 Width: 256 |
        |   1    |   0   | Height: 640 Width: 648 |
        |   1    |   1   | Height: 640 Width: 648 |
        |   1    |   2   | Height: 640 Width: 648 |
        |   1    |   3   | Height: 640 Width: 648 |
        +--------+-------+------------------------+
        [8 rows x 3 columns]
        """
        if(batch_size < 1):
            raise _ToolkitError("'batch_size' must be greater than or equal to 1")

        from ._sframe_loader import SFrameSTIter as _SFrameSTIter
        import mxnet as _mx
        from mxnet import gluon as _gluon
        from .._mxnet import _mxnet_utils

        set_of_all_idx = self._style_indices()
        style, single_style = self._style_input_check(style)

        if isinstance(max_size, _six.integer_types):
            input_shape = (max_size, max_size)
        else:
            # Outward-facing, we use (width, height), but internally we use
            # (height, width)
            input_shape = max_size[::-1]

        images, unpack = self._canonize_content_input(images, single_style=single_style)

        dataset_size = len(images)
        output_size = dataset_size * len(style)
        batch_size_each = min(batch_size, output_size)
        num_mxnet_gpus = _mxnet_utils.get_num_gpus_in_use(max_devices=batch_size_each)

        if num_mxnet_gpus == 0:
            # CPU processing prefers native size to prevent stylizing
            # unnecessary regions
            batch_size_each = 1
            loader_type = 'favor-native-size'
        else:
            # GPU processing prefers batches of same size, using padding
            # for smaller images
            loader_type = 'pad'

        self._model.batch_size = batch_size_each
        self._model.hybridize()

        ctx = _mxnet_utils.get_mxnet_context(max_devices=batch_size_each)
        batch_size = max(num_mxnet_gpus, 1) * batch_size_each
        last_time = 0
        if dataset_size == 0:
            raise _ToolkitError("SFrame cannot be empty")
        content_feature = _tkutl._find_only_image_column(images)
        _raise_error_if_not_training_sframe(images, content_feature)

        max_h = 0
        max_w = 0
        oversized_count = 0
        for img in images[content_feature]:
            if img.height > input_shape[0] or img.width > input_shape[1]:
                oversized_count += 1
            max_h = max(img.height, max_h)
            max_w = max(img.width, max_w)

        if input_shape[0] > max_h:
            input_shape = (max_h, input_shape[1])
        if input_shape[1] > max_w:
            input_shape = (input_shape[0], max_w)

        # If we find large images, let's switch to sequential iterator
        # pre-processing, to prevent memory issues.
        sequential = max(max_h, max_w) > 2000

        if verbose and output_size != 1:
            print('Stylizing {} image(s) using {} style(s)'.format(dataset_size, len(style)))
            if oversized_count > 0:
                print('Scaling down {} image(s) exceeding {}x{}'.format(oversized_count, input_shape[1], input_shape[0]))

        content_images_loader = _SFrameSTIter(images, batch_size,
                                              shuffle=False,
                                              feature_column=content_feature,
                                              input_shape=input_shape,
                                              num_epochs=1,
                                              loader_type=loader_type,
                                              repeat_each_image=len(style),
                                              sequential=sequential)

        sb = _tc.SFrameBuilder([int, int, _tc.Image],
                               column_names=['row_id', 'style', 'stylized_{}'.format(self.content_feature)])

        count = 0
        for i, batch in enumerate(content_images_loader):
            if loader_type == 'favor-native-size':
                c_data = [batch.data[0][0].expand_dims(0)]
            else:
                c_data = _gluon.utils.split_and_load(batch.data[0], ctx_list=ctx, batch_axis=0)
            indices_data = _gluon.utils.split_and_load(_mx.nd.array(batch.repeat_indices, dtype=_np.int64),
                                                       ctx_list=ctx, batch_axis=0)
            outputs = []
            for b_img, b_indices in zip(c_data, indices_data):
                mx_style = _mx.nd.array(style, dtype=_np.int64, ctx=b_indices.context)
                b_batch_styles = mx_style[b_indices]
                output = self._model(b_img, b_batch_styles)
                outputs.append(output)

            image_data = _np.concatenate([
                (output.asnumpy().transpose(0, 2, 3, 1) * 255).astype(_np.uint8)
                for output in outputs], axis=0)

            batch_styles = [style[idx] for idx in batch.repeat_indices]

            for b in range(batch_size - (batch.pad or 0)):
                image = image_data[b]
                # Crop to remove added padding
                crop = batch.crop[b]
                cropped_image = image[crop[0]:crop[1], crop[2]:crop[3]]
                tc_img = _tc.Image(_image_data=cropped_image.tobytes(),
                                   _width=cropped_image.shape[1],
                                   _height=cropped_image.shape[0],
                                   _channels=cropped_image.shape[2],
                                   _format_enum=2,
                                   _image_data_size=cropped_image.size)
                sb.append([batch.indices[b], batch_styles[b], tc_img])
                count += 1

            cur_time = _time.time()
            if verbose and output_size != 1 and (cur_time > last_time + 10 or count == output_size):
                print('Stylizing {curr_image:{width}d}/{max_n:{width}d}'.
                      format(curr_image=count, max_n=output_size, width=len(str(output_size))))
                last_time = cur_time

        return unpack(sb.close())

    def _export_coreml_image(self, image, array_shape):
        from coremltools.proto import FeatureTypes_pb2 as ft

        channels, height, width = array_shape
        if channels == 1:
            image.type.imageType.colorSpace = ft.ImageFeatureType.ColorSpace.Value('GRAYSCALE')
        elif channels == 3:
            image.type.imageType.colorSpace = ft.ImageFeatureType.ColorSpace.Value('RGB')
        else:
            raise ValueError("Channel Value %d not supported for image inputs" % channels)

        image.type.imageType.width = width
        image.type.imageType.height = height

    def export_coreml(self, path, image_shape=(256, 256), 
        include_flexible_shape=True):
        """
        Save the model in Core ML format. The Core ML model takes an image of
        fixed size, and a style index inputs and produces an output
        of an image of fixed size

        Parameters
        ----------
        path : string
            A string to the path for saving the Core ML model.

        image_shape: tuple
            A tuple (defaults to (256, 256)) will bind the coreml model to a fixed shape.

        include_flexible_shape: bool
            A boolean value indicating whether flexible_shape should be included or not.

        See Also
        --------
        save

        Examples
        --------
        >>> model.export_coreml('QuickStyle.mlmodel')
        """
        import mxnet as _mx
        from .._mxnet._mxnet_to_coreml import _mxnet_converter
        import coremltools

        transformer = self._model
        index = _mx.sym.Variable("index", shape=(1,), dtype=_np.int32)

        # append batch size and channels
        image_shape = (1, 3) + image_shape
        c_image = _mx.sym.Variable(self.content_feature, shape=image_shape,
                                         dtype=_np.float32)

        # signal that we want the transformer to prepare for coreml export
        # using a zero batch size
        transformer.batch_size = 0
        transformer.scale255 = True
        sym_out = transformer(c_image, index)

        mod = _mx.mod.Module(symbol=sym_out, data_names=[self.content_feature, "index"],
                                    label_names=None)
        mod.bind(data_shapes=zip([self.content_feature, "index"], [image_shape, (1,)]), for_training=False,
                 inputs_need_grad=False)
        gluon_weights = transformer.collect_params()
        gluon_layers = []
        for layer in transformer.collect_params()._params:
            gluon_layers.append(layer)

        sym_layers = mod._param_names
        sym_weight_dict = {}
        for gluon_layer, sym_layer in zip(gluon_layers, sym_layers):
            sym_weight_dict[sym_layer] = gluon_weights[gluon_layer]._data[0]

        mod.set_params(sym_weight_dict, sym_weight_dict)
        index_dim = (1, self.num_styles)
        coreml_model = _mxnet_converter.convert(mod, input_shape=[(self.content_feature, image_shape), ('index', index_dim)],
                        mode=None, preprocessor_args=None, builder=None, verbose=False)

        transformer.scale255 = False
        spec = coreml_model.get_spec()
        image_input = spec.description.input[0]
        image_output = spec.description.output[0]

        input_array_shape = tuple(image_input.type.multiArrayType.shape)
        output_array_shape = tuple(image_output.type.multiArrayType.shape)

        self._export_coreml_image(image_input, input_array_shape)
        self._export_coreml_image(image_output, output_array_shape)

        stylized_image = 'stylized%s' % self.content_feature.capitalize()
        coremltools.utils.rename_feature(spec,
                'transformer__mulscalar0_output', stylized_image, True, True)

        if include_flexible_shape:
            # Support flexible shape
            flexible_shape_utils = _mxnet_converter._coremltools.models.neural_network.flexible_shape_utils
            img_size_ranges = flexible_shape_utils.NeuralNetworkImageSizeRange()
            img_size_ranges.add_height_range((64, -1))
            img_size_ranges.add_width_range((64, -1))
            flexible_shape_utils.update_image_size_range(spec, feature_name=self.content_feature, size_range=img_size_ranges)
            flexible_shape_utils.update_image_size_range(spec, feature_name=stylized_image, size_range=img_size_ranges)

        model_type = 'quick style (%s)' % self.model
        spec.description.metadata.shortDescription = _coreml_utils._mlmodel_short_description(
            model_type)
        spec.description.input[0].shortDescription = 'Input image'
        spec.description.input[1].shortDescription = u'Style index array (set index I to 1.0 to enable Ith style)'
        spec.description.output[0].shortDescription = 'Stylized image'
        user_defined_metadata = _coreml_utils._get_model_metadata(
            self.__class__.__name__, {
                'model': self.model,
                'num_styles': str(self.num_styles),
                'content_feature': self.content_feature,
                'style_feature': self.style_feature,
                'max_iterations': str(self.max_iterations),
                'training_iterations': str(self.training_iterations),
            }, version=QuickStyle._PYTHON_QUICK_STYLE_VERSION)
        spec.description.metadata.userDefined.update(user_defined_metadata)
        from coremltools.models.utils import save_spec as _save_spec
        _save_spec(spec, path)

    def get_styles(self, style=None):
        """
        Returns SFrame of style images used for training the model

        Parameters
        ----------
        style: int or list, optional
            The selected style or list of styles to return. If `None`, all
            styles will be returned

        See Also
        --------
        stylize

        Examples
        --------
        >>>  model.get_styles()
        Columns:
            style   int
            image   Image

        Rows: 4

        Data:
        +-------+--------------------------+
        | style |          image           |
        +-------+--------------------------+
        |  0    |  Height: 642 Width: 642  |
        |  1    |  Height: 642 Width: 642  |
        |  2    |  Height: 642 Width: 642  |
        |  3    |  Height: 642 Width: 642  |
        +-------+--------------------------+
        """

        style, _ = self._style_input_check(style)
        return self.styles.filter_by(style, self._index_column)

