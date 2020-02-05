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
from turicreate.toolkits._model import Model as _Model
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._model import PythonProxy as _PythonProxy
import turicreate as _tc
import numpy as _np
import math as _math
import six as _six
from .._mps_utils import (
    use_mps as _use_mps,
    mps_device_memory_limit as _mps_device_memory_limit,
    MpsGraphAPI as _MpsGraphAPI,
    MpsStyleGraphAPI as _MpsStyleGraphAPI,
    MpsGraphNetworkType as _MpsGraphNetworkType,
    MpsGraphMode as _MpsGraphMode,
)


def _get_mps_st_net(input_image_shape, batch_size, output_size, config, weights={}):
    """
    Initializes an MpsGraphAPI for style transfer.
    """
    c_in, h_in, w_in = input_image_shape

    c_out = output_size[0]
    h_out = h_in
    w_out = w_in

    c_view = c_in
    h_view = h_in
    w_view = w_in

    network = _MpsStyleGraphAPI(
        batch_size,
        c_in,
        h_in,
        w_in,
        c_out,
        h_out,
        w_out,
        weights=weights,
        config=config,
    )
    return network


def create(
    style_dataset,
    content_dataset,
    style_feature=None,
    content_feature=None,
    max_iterations=None,
    model="resnet-16",
    verbose=True,
    batch_size=1,
    **kwargs
):
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
    if not isinstance(style_dataset, _tc.SFrame):
        raise TypeError('"style_dataset" must be of type SFrame.')
    if not isinstance(content_dataset, _tc.SFrame):
        raise TypeError('"content_dataset" must be of type SFrame.')
    if len(style_dataset) == 0:
        raise _ToolkitError("style_dataset SFrame cannot be empty")
    if len(content_dataset) == 0:
        raise _ToolkitError("content_dataset SFrame cannot be empty")
    if batch_size < 1:
        raise _ToolkitError("'batch_size' must be greater than or equal to 1")
    if max_iterations is not None and (
        not isinstance(max_iterations, int) or max_iterations < 0
    ):
        raise _ToolkitError(
            "'max_iterations' must be an integer greater than or equal to 0"
        )

    if style_feature is None:
        style_feature = _tkutl._find_only_image_column(style_dataset)

    if content_feature is None:
        content_feature = _tkutl._find_only_image_column(content_dataset)
    if verbose:
        print(
            "Using '{}' in style_dataset as feature column and using "
            "'{}' in content_dataset as feature column".format(
                style_feature, content_feature
            )
        )

    _raise_error_if_not_training_sframe(style_dataset, style_feature)
    _raise_error_if_not_training_sframe(content_dataset, content_feature)
    _tkutl._handle_missing_values(style_dataset, style_feature, "style_dataset")
    _tkutl._handle_missing_values(content_dataset, content_feature, "content_dataset")

    params = {
        "batch_size": batch_size,
        "vgg16_content_loss_layer": 2,  # conv3_3 layer
        "lr": 0.001,
        "content_loss_mult": 1.0,
        "style_loss_mult": [1e-4, 1e-4, 1e-4, 1e-4],  # conv 1-4 layers
        "finetune_all_params": True,
        "pretrained_weights": False,
        "print_loss_breakdown": False,
        "input_shape": (256, 256),
        "training_content_loader_type": "stretch",
        "use_augmentation": False,
        "sequential_image_processing": False,
        # Only used if use_augmentaion is True
        "aug_resize": 0,
        "aug_min_object_covered": 0,
        "aug_rand_crop": 0.9,
        "aug_rand_pad": 0.9,
        "aug_rand_gray": 0.0,
        "aug_aspect_ratio": 1.25,
        "aug_hue": 0.05,
        "aug_brightness": 0.05,
        "aug_saturation": 0.05,
        "aug_contrast": 0.05,
        "aug_horizontal_flip": True,
        "aug_area_range": (0.05, 1.5),
        "aug_pca_noise": 0.0,
        "aug_max_attempts": 20,
        "aug_inter_method": 2,
        "checkpoint": False,
        "checkpoint_prefix": "style_transfer",
        "checkpoint_increment": 1000,
    }

    if "_advanced_parameters" in kwargs:
        # Make sure no additional parameters are provided
        new_keys = set(kwargs["_advanced_parameters"].keys())
        set_keys = set(params.keys())
        unsupported = new_keys - set_keys
        if unsupported:
            raise _ToolkitError("Unknown advanced parameters: {}".format(unsupported))

        params.update(kwargs["_advanced_parameters"])

    name = "style_transfer"

    import turicreate as _turicreate

    # Imports tensorflow
    import turicreate.toolkits.libtctensorflow

    model = _turicreate.extensions.style_transfer()
    pretrained_resnet_model = _pre_trained_models.STYLE_TRANSFER_BASE_MODELS[
        "resnet-16"
    ]()
    pretrained_vgg16_model = _pre_trained_models.STYLE_TRANSFER_BASE_MODELS["Vgg16"]()
    options = {}
    options["image_height"] = params["input_shape"][0]
    options["image_width"] = params["input_shape"][1]
    options["content_feature"] = content_feature
    options["style_feature"] = style_feature
    if verbose is not None:
        options["verbose"] = verbose
    else:
        options["verbose"] = False
    if batch_size is not None:
        options["batch_size"] = batch_size
    if max_iterations is not None:
        options["max_iterations"] = max_iterations
    options["num_styles"] = len(style_dataset)
    options["resnet_mlmodel_path"] = pretrained_resnet_model.get_model_path("coreml")
    options["vgg_mlmodel_path"] = pretrained_vgg16_model.get_model_path("coreml")
    options["pretrained_weights"] = params["pretrained_weights"]

    model.train(style_dataset[style_feature], content_dataset[content_feature], options)
    return StyleTransfer(model_proxy=model, name=name)


def _raise_error_if_not_training_sframe(dataset, context_column):
    _raise_error_if_not_sframe(dataset, "datset")
    if context_column not in dataset.column_names():
        raise _ToolkitError("Context Image column '%s' does not exist" % context_column)
    if dataset[context_column].dtype != _tc.Image:
        raise _ToolkitError("Context Image column must contain images")


class StyleTransfer(_Model):
    """
    A trained model using C++ implementation that is ready to use for classification or export to
    CoreML.

    This model should not be constructed directly.
    """

    _CPP_STYLE_TRANSFER_VERSION = 1

    def __init__(self, model_proxy=None, name=None):
        self.__proxy__ = model_proxy
        self.__name__ = name

    @classmethod
    def _native_name(cls):
        return "style_transfer"

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
        out = _tkutl._toolkit_repr_print(self, sections, section_titles, width=width)
        return out

    def _get_version(self):
        return self._CPP_STYLE_TRANSFER_VERSION

    def export_coreml(
        self, filename, image_shape=(256, 256), include_flexible_shape=True
    ):
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
            Allows the size of the input image to be flexible. Any input image were the
            height and width are at least 64 will be accepted by the Core ML Model.

        See Also
        --------
        save

        Examples
        --------
        >>> model.export_coreml('StyleTransfer.mlmodel')
        """
        options = {}
        options["image_width"] = image_shape[1]
        options["image_height"] = image_shape[0]
        options["include_flexible_shape"] = include_flexible_shape
        additional_user_defined_metadata = _coreml_utils._get_tc_version_info()
        short_description = _coreml_utils._mlmodel_short_description("Style Transfer")

        self.__proxy__.export_to_coreml(
            filename, short_description, additional_user_defined_metadata, options
        )

    def stylize(self, images, style=None, verbose=True, max_size=800, batch_size=4):
        """
        Stylize an SFrame of Images given a style index or a list of
        styles.

        Parameters
        ----------
        images : SFrame | SArray | turicreate.Image
            A dataset that has the same content image column that was used
            during training.

        style : None | int | list
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
        if not isinstance(images, (_tc.SFrame, _tc.SArray, _tc.Image)):
            raise TypeError(
                '"image" parameter must be of type SFrame, SArray or turicreate.Image.'
            )
        if isinstance(images, (_tc.SFrame, _tc.SArray)) and len(images) == 0:
            raise _ToolkitError('"image" parameter cannot be empty')
        if style is not None and not isinstance(style, (int, list)):
            raise TypeError('"style" must parameter must be a None, int or a list')
        if not isinstance(max_size, int):
            raise TypeError('"max_size" must parameter must be an int')
        if max_size < 1:
            raise _ToolkitError("'max_size' must be greater than or equal to 1")
        if not isinstance(batch_size, int):
            raise TypeError('"batch_size" must parameter must be an int')
        if batch_size < 1:
            raise _ToolkitError("'batch_size' must be greater than or equal to 1")

        options = {}
        options["style_idx"] = style
        options["verbose"] = verbose
        options["max_size"] = max_size
        options["batch_size"] = batch_size

        if isinstance(style, list) or style is None:
            if isinstance(images, _tc.SFrame):
                image_feature = _tkutl._find_only_image_column(images)
                stylized_images = self.__proxy__.predict(images[image_feature], options)
                stylized_images = stylized_images.rename(
                    {"stylized_image": "stylized_" + str(image_feature)}
                )
                return stylized_images
            return self.__proxy__.predict(images, options)
        else:
            if isinstance(images, _tc.SFrame):
                if len(images) == 0:
                    raise _ToolkitError("SFrame cannot be empty")
                image_feature = _tkutl._find_only_image_column(images)
                stylized_images = self.__proxy__.predict(images[image_feature], options)
                stylized_images = stylized_images.rename(
                    {"stylized_image": "stylized_" + str(image_feature)}
                )
                return stylized_images
            elif isinstance(images, (_tc.Image)):
                stylized_images = self.__proxy__.predict(images, options)
                return stylized_images["stylized_image"][0]
            elif isinstance(images, (_tc.SArray)):
                stylized_images = self.__proxy__.predict(images, options)
                return stylized_images["stylized_image"]

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
        return self.__proxy__.get_styles(style)

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
            ("Model", "model"),
            ("Number of unique styles", "num_styles"),
        ]

        training_fields = [
            ("Training time", "_training_time_as_string"),
            ("Training epochs", "training_epochs"),
            ("Training iterations", "training_iterations"),
            ("Number of style images", "num_styles"),
            ("Number of content images", "num_content_images"),
            ("Final loss", "training_loss"),
        ]

        section_titles = ["Schema", "Training summary"]
        return ([model_fields, training_fields], section_titles)
