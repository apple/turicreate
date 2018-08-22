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
from .. import _mxnet_utils
from ._utils import _seconds_as_string
from .. import _pre_trained_models
from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._model import PythonProxy as _PythonProxy
import turicreate as _tc
import numpy as _np
import math as _math
import six as _six


def _vgg16_data_prep(batch):
    """
    Takes images scaled to [0, 1] and returns them appropriately scaled and
    mean-subtracted for VGG-16
    """
    from mxnet import nd
    mean = nd.array([123.68, 116.779, 103.939], ctx=batch.context)
    return nd.broadcast_sub(255 * batch, mean.reshape((-1, 1, 1)))

def create(style_dataset, content_dataset, style_feature=None,
        content_feature=None, max_iterations=None, model='resnet-16',
        verbose=True, batch_size = 6, **kwargs):
    """
    Create a :class:`StyleTransfer` model.

    Parameters
    ----------
    style_dataset: SFrame
        Input style images. The columns named by the ``style_feature`` parameters will
        be extracted for training the model.

    content_dataset : SFrame
        Input content images. The columns named by the ``content_feature`` parameters will
        be extracted for training the model.

    style_feature: string
        Name of the column containing the input images in style SFrame.
        'None' (the default) indicates the only image column in the style SFrame
        should be used as the feature.

    content_feature: string
        Name of the column containing the input images in content SFrame.
        'None' (the default) indicates the only image column in the content
        SFrame should be used as the feature.

    max_iterations : int
        The number of training iterations. If 'None' (the default), then it will
        be automatically determined based on the amount of data you provide.

    model : string optional
        Style transfer model to use:

            - "resnet-16" : Fast and small-sized residual network that uses
                            VGG-16 as reference network during training.

    batch_size : int, optional
        If you are getting memory errors, try decreasing this value. If you
        have a powerful computer, increasing this value may improve training
        throughput.

    verbose : bool, optional
        If True, print progress updates and model details.


    Returns
    -------
    out : StyleTransfer
        A trained :class:`StyleTransfer` model.

    See Also
    --------
    StyleTransfer

    Examples
    --------
    .. sourcecode:: python

        # Create datasets
        >>> content_dataset = turicreate.image_analysis.load_images('content_images/')
        >>> style_dataset = turicreate.image_analysis.load_images('style_images/')

        # Train a style transfer model
        >>> model = turicreate.style_transfer.create(content_dataset, style_dataset)

        # Stylize an image on all styles
        >>> stylized_images = model.stylize(data)

        # Visualize the stylized images
        >>> stylized_images.explore()

    """
    if len(style_dataset) == 0:
        raise _ToolkitError("style_dataset SFrame cannot be empty")
    if len(content_dataset) == 0:
        raise _ToolkitError("content_dataset SFrame cannot be empty")
    if(batch_size < 1):
        raise _ToolkitError("'batch_size' must be greater than or equal to 1")

    from ._sframe_loader import SFrameSTIter as _SFrameSTIter
    import mxnet as _mx

    if style_feature is None:
        style_feature = _tkutl._find_only_image_column(style_dataset)
    if content_feature is None:
        content_feature = _tkutl._find_only_image_column(content_dataset)
    if verbose:
        print("Using '{}' in style_dataset as feature column and using "
              "'{}' in content_dataset as feature column".format(style_feature, content_feature))

    _raise_error_if_not_training_sframe(style_dataset, style_feature)
    _raise_error_if_not_training_sframe(content_dataset, content_feature)

    params = {
        'batch_size': batch_size,
        'vgg16_content_loss_layer': 2,  # conv3_3 layer
        'lr': 0.001,
        'content_loss_mult': 1.0,
        'style_loss_mult': [1e-4, 1e-4, 1e-4, 1e-4],  # conv 1-4 layers
        'finetune_all_params': False,
        'print_loss_breakdown': False,
        'input_shape': (256, 256),
        'training_content_loader_type': 'stretch',
        'use_augmentation': False,
        'sequential_image_processing': False,
        # Only used if use_augmentaion is True
        'aug_resize': 0,
        'aug_min_object_covered': 0,
        'aug_rand_crop': 0.9,
        'aug_rand_pad': 0.9,
        'aug_rand_gray': 0.0,
        'aug_aspect_ratio': 1.25,
        'aug_hue': 0.05,
        'aug_brightness': 0.05,
        'aug_saturation': 0.05,
        'aug_contrast': 0.05,
        'aug_horizontal_flip': True,
        'aug_area_range': (.05, 1.5),
        'aug_pca_noise': 0.0,
        'aug_max_attempts': 20,
        'aug_inter_method': 2,
    }

    if '_advanced_parameters' in kwargs:
        # Make sure no additional parameters are provided
        new_keys = set(kwargs['_advanced_parameters'].keys())
        set_keys = set(params.keys())
        unsupported = new_keys - set_keys
        if unsupported:
            raise _ToolkitError('Unknown advanced parameters: {}'.format(unsupported))

        params.update(kwargs['_advanced_parameters'])

    _content_loss_mult = params['content_loss_mult']
    _style_loss_mult = params['style_loss_mult']

    num_gpus = _mxnet_utils.get_num_gpus_in_use(max_devices=params['batch_size'])
    batch_size_each = params['batch_size'] // max(num_gpus, 1)
    batch_size = max(num_gpus, 1) * batch_size_each
    input_shape = params['input_shape']

    iterations = 0
    if max_iterations is None:
        max_iterations = len(style_dataset) * 500 + 2000
        if verbose:
            print('Setting max_iterations to be {}'.format(max_iterations))

    # data loader
    if params['use_augmentation']:
        content_loader_type = '%s-with-augmentation' % params['training_content_loader_type']
    else:
        content_loader_type = params['training_content_loader_type']

    content_images_loader = _SFrameSTIter(content_dataset, batch_size, shuffle=True,
                                  feature_column=content_feature, input_shape=input_shape,
                                  loader_type=content_loader_type, aug_params=params,
                                  sequential=params['sequential_image_processing'])
    ctx = _mxnet_utils.get_mxnet_context(max_devices=params['batch_size'])

    num_styles = len(style_dataset)

    # TRANSFORMER MODEL
    from ._model import Transformer as _Transformer
    transformer_model_path = _pre_trained_models.STYLE_TRANSFER_BASE_MODELS[model]().get_model_path()
    transformer = _Transformer(num_styles, batch_size_each)
    transformer.collect_params().initialize(ctx=ctx)
    transformer.load_params(transformer_model_path, ctx, allow_missing=True)
    # For some reason, the transformer fails to hybridize for training, so we
    # avoid this until resolved
    # transformer.hybridize()

    # VGG MODEL
    from ._model import Vgg16 as _Vgg16
    vgg_model_path = _pre_trained_models.STYLE_TRANSFER_BASE_MODELS['Vgg16']().get_model_path()
    vgg_model = _Vgg16()
    vgg_model.collect_params().initialize(ctx=ctx)
    vgg_model.load_params(vgg_model_path, ctx=ctx, ignore_extra=True)
    vgg_model.hybridize()

    # TRAINER
    from mxnet import gluon as _gluon
    from ._model import gram_matrix as _gram_matrix

    if params['finetune_all_params']:
        trainable_params = transformer.collect_params()
    else:
        trainable_params = transformer.collect_params('.*gamma|.*beta')

    trainer = _gluon.Trainer(trainable_params, 'adam', {'learning_rate': params['lr']})
    mse_loss = _gluon.loss.L2Loss()
    start_time = _time.time()
    smoothed_loss = None
    last_time = 0

    cuda_gpus = _mxnet_utils.get_gpus_in_use(max_devices=params['batch_size'])
    num_mxnet_gpus = len(cuda_gpus)

    if verbose:
        # Estimate memory usage (based on experiments)
        cuda_mem_req = 260 + batch_size_each * 880 + num_styles * 1.4

        _tkutl._print_neural_compute_device(cuda_gpus=cuda_gpus, use_mps=False,
                                            cuda_mem_req=cuda_mem_req, has_mps_impl=False)
    #
    # Pre-compute gram matrices for style images
    #
    if verbose:
        print('Analyzing visual features of the style images')

    style_images_loader = _SFrameSTIter(style_dataset, batch_size, shuffle=False, num_epochs=1,
                                        feature_column=style_feature, input_shape=input_shape,
                                        loader_type='stretch',
                                        sequential=params['sequential_image_processing'])
    num_layers = len(params['style_loss_mult'])
    gram_chunks = [[] for _ in range(num_layers)]
    for s_batch in style_images_loader:
        s_data = _gluon.utils.split_and_load(s_batch.data[0], ctx_list=ctx, batch_axis=0)
        for s in s_data:
            vgg16_s = _vgg16_data_prep(s)
            ret = vgg_model(vgg16_s)
            grams = [_gram_matrix(x) for x in ret]
            for i, gram in enumerate(grams):
                if gram.context != _mx.cpu(0):
                    gram = gram.as_in_context(_mx.cpu(0))
                gram_chunks[i].append(gram)
    del style_images_loader

    grams = [
        # The concatenated styles may be padded, so we slice overflow
        _mx.nd.concat(*chunks, dim=0)[:num_styles]
        for chunks in gram_chunks
    ]

    # A context->grams look-up table, where all the gram matrices have been
    # distributed
    ctx_grams = {}
    if ctx[0] == _mx.cpu(0):
        ctx_grams[_mx.cpu(0)] = grams
    else:
        for ctx0 in ctx:
            ctx_grams[ctx0] = [gram.as_in_context(ctx0) for gram in grams]

    #
    # Training loop
    #

    vgg_content_loss_layer = params['vgg16_content_loss_layer']
    rs = _np.random.RandomState(1234)
    while iterations < max_iterations:
        content_images_loader.reset()
        for c_batch in content_images_loader:
            c_data = _gluon.utils.split_and_load(c_batch.data[0], ctx_list=ctx, batch_axis=0)

            Ls = []
            curr_content_loss = []
            curr_style_loss = []
            with _mx.autograd.record():
                for c in c_data:
                    # Randomize styles to train
                    indices = _mx.nd.array(rs.randint(num_styles, size=batch_size_each),
                                           dtype=_np.int64, ctx=c.context)

                    # Generate pastiche
                    p = transformer(c, indices)

                    # mean subtraction
                    vgg16_p = _vgg16_data_prep(p)
                    vgg16_c = _vgg16_data_prep(c)

                    # vgg forward
                    p_vgg_outputs = vgg_model(vgg16_p)

                    c_vgg_outputs = vgg_model(vgg16_c)
                    c_content_layer = c_vgg_outputs[vgg_content_loss_layer]
                    p_content_layer = p_vgg_outputs[vgg_content_loss_layer]

                    # Calculate Loss
                    # Style Loss between style image and stylized image
                    # Ls = sum of L2 norm of gram matrix of vgg16's conv layers
                    style_losses = []
                    for gram, p_vgg_output, style_loss_mult in zip(ctx_grams[c.context], p_vgg_outputs, _style_loss_mult):
                        gram_s_vgg = gram[indices]
                        gram_p_vgg = _gram_matrix(p_vgg_output)

                        style_losses.append(style_loss_mult * mse_loss(gram_s_vgg, gram_p_vgg))

                    style_loss = _mx.nd.add_n(*style_losses)

                    # Content Loss between content image and stylized image
                    # Lc = L2 norm at a single layer in vgg16
                    content_loss = _content_loss_mult * mse_loss(c_content_layer,
                                                                 p_content_layer)

                    curr_content_loss.append(content_loss)
                    curr_style_loss.append(style_loss)
                    # Divide loss by large number to get into a more legible
                    # range
                    total_loss = (content_loss + style_loss) / 10000.0
                    Ls.append(total_loss)
                for L in Ls:
                    L.backward()

            cur_loss = _np.mean([L.asnumpy()[0] for L in Ls])

            if smoothed_loss is None:
                smoothed_loss = cur_loss
            else:
                smoothed_loss = 0.9 * smoothed_loss + 0.1 * cur_loss
            iterations += 1
            trainer.step(batch_size)

            if verbose and iterations == 1:
                # Print progress table header
                column_names = ['Iteration', 'Loss', 'Elapsed Time']
                num_columns = len(column_names)
                column_width = max(map(lambda x: len(x), column_names)) + 2
                hr = '+' + '+'.join(['-' * column_width] * num_columns) + '+'
                print(hr)
                print(('| {:<{width}}' * num_columns + '|').format(*column_names, width=column_width-1))
                print(hr)

            cur_time = _time.time()
            if verbose and (cur_time > last_time + 10 or iterations == max_iterations):
                # Print progress table row
                elapsed_time = cur_time - start_time
                print("| {cur_iter:<{width}}| {loss:<{width}.3f}| {time:<{width}.1f}|".format(
                    cur_iter = iterations, loss = smoothed_loss,
                    time = elapsed_time , width = column_width-1))
                if params['print_loss_breakdown']:
                    print_content_loss = _np.mean([L.asnumpy()[0] for L in curr_content_loss])
                    print_style_loss = _np.mean([L.asnumpy()[0] for L in curr_style_loss])
                    print('Total Loss: {:6.3f} | Content Loss: {:6.3f} | Style Loss: {:6.3f}'.format(cur_loss, print_content_loss, print_style_loss))
                last_time = cur_time
            if iterations == max_iterations:
                print(hr)
                break

    training_time = _time.time() - start_time
    style_sa = style_dataset[style_feature]
    idx_column = _tc.SArray(range(0, style_sa.shape[0]))
    style_sframe = _tc.SFrame({"style": idx_column, style_feature: style_sa})

    # Save the model state
    state = {
        '_model': transformer,
        '_training_time_as_string': _seconds_as_string(training_time),
        'batch_size': batch_size,
        'num_styles': num_styles,
        'model': model,
        'input_image_shape': input_shape,
        'styles': style_sframe,
        'num_content_images': len(content_dataset),
        'training_time': training_time,
        'max_iterations': max_iterations,
        'training_iterations': iterations,
        'training_epochs': content_images_loader.cur_epoch,
        'style_feature': style_feature,
        'content_feature': content_feature,
        "_index_column": "style",
        'training_loss': smoothed_loss,
    }

    return StyleTransfer(state)


def _raise_error_if_not_training_sframe(dataset, context_column):
    _raise_error_if_not_sframe(dataset, 'datset')
    if context_column not in dataset.column_names():
        raise _ToolkitError("Context Image column '%s' does not exist"
            % context_column)
    if dataset[context_column].dtype != _tc.Image:
        raise _ToolkitError("Context Image column must contain images")


class StyleTransfer(_CustomModel):
    """
    An trained model that is ready to use for style transfer, exported to
    Core ML.

    This model should not be constructed directly.
    """

    _PYTHON_STYLE_TRANSFER_VERSION = 1

    def __init__(self, state=None):
        if(state != None):
            self.__proxy__ = _PythonProxy(state)

    @classmethod
    def _native_name(cls):
        return "style_transfer"

    def _get_native_state(self):
        state = self.__proxy__.get_state()
        mxnet_params = state['_model'].collect_params()
        state['_model'] = _mxnet_utils.get_gluon_net_params_state(mxnet_params)
        return state

    def _get_version(self):
        return self._PYTHON_STYLE_TRANSFER_VERSION 

    @classmethod
    def _load_version(cls, state, version):
        from ._model import Transformer as _Transformer
        _tkutl._model_version_check(version, cls._PYTHON_STYLE_TRANSFER_VERSION)

        net = _Transformer(state['num_styles'], state['batch_size'])
        ctx = _mxnet_utils.get_mxnet_context(max_devices=state['batch_size'])

        net_params = net.collect_params()
        _mxnet_utils.load_net_params_from_state(net_params, state['_model'], ctx=ctx)
        state['_model'] = net
        state['input_image_shape'] = tuple([int(i) for i in state['input_image_shape']])
        return StyleTransfer(state)

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the StyleTransfer.
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
            ('Training epochs', 'training_epochs'),
            ('Training iterations', 'training_iterations'),
            ('Number of style images', 'num_styles'),
            ('Number of content images', 'num_content_images'),
            ('Final loss', 'training_loss'),
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
        >>> model.export_coreml('StyleTransfer.mlmodel')
        """
        import mxnet as _mx
        from .._mxnet_to_coreml import _mxnet_converter
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

        model_type = 'style transfer (%s)' % self.model
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
            }, version=StyleTransfer._PYTHON_STYLE_TRANSFER_VERSION)
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
