# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

import random as _random
import turicreate as _tc
from turicreate import extensions as _extensions
from turicreate.toolkits.one_shot_object_detector.util._error_handling import check_one_shot_input
from turicreate.toolkits import _data_zoo

def preview_augmented_images(data,
                             target,
                             backgrounds=None,
                             verbose=True,
                             **kwargs):
    """
    A utility function that would allow the user to visualize 
    augmented images where data is the SFrame of starter images.

    Parameters
    ----------

    data : SFrame | tc.Image
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
    
    backgrounds : optional SArray
        A list of backgrounds that the user wants to provide for data
        augmentation.
        If this is provided, only the backgrounds provided as this field will be
        used for augmentation.

    Returns
    -------
    out : SFrame
        An SFrame of n augmented images along with their annotations.
    """
    dataset_to_augment, image_column_name, target_column_name = check_one_shot_input(data, target)
    one_shot_model = _extensions.one_shot_object_detector()
    seed = kwargs["seed"] if "seed" in kwargs else _random.randint(0, 2**32 - 1)
    if backgrounds is None:
        backgrounds_downloader = _data_zoo.OneShotObjectDetectorBackgroundData()
        backgrounds_tar_path = backgrounds_downloader.get_backgrounds_path()
        backgrounds_tar = _tarfile.open(backgrounds_tar_path)
        backgrounds_tar.extractall()
        backgrounds = _tc.SArray("one_shot_backgrounds.sarray")
    # Option arguments to pass in to C++ Object Detector, if we use it:
    # {'mlmodel_path':'darknet.mlmodel', 'max_iterations' : 25}
    options_for_augmentation = {
        "seed": seed
    }
    augmented_data = one_shot_model.augment(dataset_to_augment,
                                            image_column_name,
                                            target_column_name,
                                            backgrounds,
                                            options_for_augmentation)
    return augmented_data
