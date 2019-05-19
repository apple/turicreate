# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
#
import random as _random
import turicreate as _tc
import turicreate.toolkits._internal_utils as _tkutl
from turicreate import extensions as _extensions
from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from turicreate.toolkits.object_detector.object_detector import ObjectDetector as _ObjectDetector

def create(dataset,
           target,
           augmentation_mode="auto",
           flip_horizontal=True,
           flip_vertical=True,
           backgrounds=None,
           batch_size=0,
           max_iterations=0,
           seed=None,
           verbose=True,
           **kwargs):
    """
    Create a :class:`ObjectDetector` model.

    Parameters
    ----------

    dataset : SFrame | tc.Image
        An SFrame that contains all the starter images along with their
        corresponding labels.
        If the dataset is a single tc.Image, this parameter is just that image.
        Every starter image should entirely be just the starter image without
        any padding.
        RGB and RGBA images allowed.

    target : string
        The target column name in the SFrame that contains all the labels. 
        If the dataset is a single tc.Image, this parameter is just a label for
        that image.

    augmentation_mode : string optional
        An augmentation mode is a shortcut to setting the 'flip_horizontal',
        'flip_vertical', and other advanced parameters in **kwargs.
        For example, an augmentation_mode of 'auto' automatically sets the
        following parameter values:
            flip_horizontal = True
            flip_vertical = True
            yaw = [(0, 360)]
            pitch = [(0, 360)]
            roll = [(0, 360)]
            blur = True
            noise = True
            lighting_perturbation = True
        Here is the set of augmentation modes available:
        - 'auto'
        - 'logo'
        - 'road_sign'
        - 'playing_card'
        - 'chart'
        TBD/TODO: Specify the different pre-set parameter values each of these
        modes corresponds to, probably as a table.
        If 'dataset' is an SFrame, the augmentation_mode specified as this
        parameter will be applied to all the images provided in the SFrame.
        If you'd like different images in the 'dataset' SFrame to have different
        augmentation_mode's, those modes can be specified as a column in the
        SFrame, and that column name should be passed in to the
        'augmentation_mode' parameter.

    flip_horizontal : bool optional
        Boolean flag that indicates whether the object can be flipped
        horizontally,
        i.e. whether a water image of the object should appear can be in the
        augmented data.
        This will probably be false for logos.
        If the user wants to set this field and has multiple starter images in
        the dataset, they can also provide values for this field as a different
        column in the dataset.
        Defaults to True.
        If 'dataset' is an SFrame, the flip_horizontal value specified as this
        parameter will be applied to all the images provided in the SFrame.
        If you'd like different images in the 'dataset' SFrame to have different
        flip_horizontal's, those values can be specified as a column in the
        SFrame, and that column name should be passed in to the
        'flip_horizontal' parameter.
        This parameter takes precedence over any value specified for the
        augmentation_mode.

    flip_vertical : bool optional
        Boolean flag that indicates whether the object can be flipped
        vertically,
        i.e. whether a mirror image of the object should appear can be in the
        augmented data.
        This will probably be false for logos.
        If the user wants to set this field and has multiple starter images in
        the dataset, they can also provide values for this field as a different
        column in the dataset.
        Defaults to True.
        If 'dataset' is an SFrame, the flip_vertical value specified as this
        parameter will be applied to all the images provided in the SFrame.
        If you'd like different images in the 'dataset' SFrame to have different
        flip_vertical's, those values can be specified as a column in the
        SFrame, and that column name should be passed in to the
        'flip_vertical' parameter
        This parameter takes precedence over any value specified for the
        augmentation_mode.

    backgrounds : optional SArray
        A list of backgrounds that the user wants to provide for data
        augmentation.
        If this is provided, only the backgrounds provided as this field will be
        used for augmentation.

    batch_size : int
        The number of images per training iteration. If 0, then it will be
        automatically determined based on resource availability.

    max_iterations : int
        The number of training iterations. If 0, then it will be automatically
        be determined based on the amount of data you provide.

    verbose : bool optional
        If True, print progress updates and model details.

    **kwargs : dict optional
        A dictionary of advanced parameters.
        Here are the parameters currently supported:
        - yaw  : rotation of the logo (or any 2-D starter image) along Y axis
        - pitch: rotation of the logo (or any 2-D starter image) along X axis
        - roll : rotation of the logo (or any 2-D starter image) along Z axis
            A list of the ranges that the user wants the respective angle 
            parameter to be in. e.g. yaw = [(0, 90), (180, 270)]. 
            This would assume a normal distribution by default.
            Here is some ASCII art explaining yaw, pitch, roll

            ##############################      y (yaw)
            ##############################      ^
            ########## ________ ##########      |
            ##########|  logo  |##########      |
            ##########|________|##########      |-------> x (pitch)
            ##############################     /
            ##############################    /
            ##############################   /
            ##############################  z (roll)

        - blur: A boolean flag indicating whether augmented images should be
                blurred or not. True by default.
        - noise: A boolean flag indicating whether augmented images should have
                artificial noise or not. True by default.
        - lighting_perturbation: A boolean flag indicating whether the 
                augmented images should have their lightings perturbed or not.
                True by default.
    """
    if not isinstance(target, str):
        raise TypeError("'target' must be of type string")
    if isinstance(dataset, _tc.SFrame):
        image_column_name = _tkutl._find_only_image_column(dataset)
        target_column_name = target
        dataset_to_augment = dataset
    elif isinstance(dataset, _tc.Image):
        image_column_name = "image"
        target_column_name = "target"
        dataset_to_augment = _tc.SFrame({image_column_name: dataset,
                                         target_column_name: target})
    else:
        raise TypeError("'dataset' must be of type SFrame or Image")

    one_shot_model = _extensions.one_shot_object_detector()
    if seed is None: seed = _random.randint(0, 2**32 - 1)
    if backgrounds is None:
        # replace this with loading backgrounds from developer.apple.com
        backgrounds = _tc.SArray()
    # Option arguments to pass in to C++ Object Detector, if we use it:
    # {'mlmodel_path':'darknet.mlmodel', 'max_iterations' : 25}
    options_for_augmentation = {
        "seed": seed,
        "flip_horizontal": flip_horizontal,
        "flip_vertical": flip_vertical,
        "kwargs": kwargs
    }
    augmented_data = one_shot_model.augment(dataset_to_augment,
                                            image_column_name,
                                            target_column_name,
                                            backgrounds,
                                            options_for_augmentation)
    model = _tc.object_detector.create( augmented_data,
                                        batch_size=batch_size,
                                        max_iterations=max_iterations,
                                        verbose=verbose)
    return OneShotObjectDetector(model.__proxy__)


class OneShotObjectDetector(_ObjectDetector):
    def __init__(self, state):
        super(OneShotObjectDetector, self).__init__(state)

    def get_augmented_images(self):
        pass
