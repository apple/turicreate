# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from turicreate import extensions as _extensions

def preview_augmented_images(data,
                             target,
                             n=1,
                             augmentation_mode="auto",
                             flip_horizontal=True,
                             flip_vertical=True,
                             backgrounds=None,
                             seed=None,
                             verbose=True,
                             **kwargs):
    """
    A utility function that would allow the user to visualize 
    `n` augmented images per row of the data if data is an SFrame,
    and just one augmented image if the data is an Image.

    Parameters
    ----------

    data : SFrame | tc.Image
    target : string
    n : int
        The number of augmented images per row of data to generate.

    Returns
    -------
    out : SFrame
        An SFrame of n augmented images along with their annotations.
    """
    dataset_to_augment, image_column_name, target_column_name = check_one_shot_input(data, target)
    one_shot_model = _extensions.one_shot_object_detector()
    options_for_augmentation = {
        "seed": seed,
        "flip_horizontal": flip_horizontal,
        "flip_vertical": flip_vertical,
        "kwargs": kwargs,
        "n": n
    }
    augmented_data = one_shot_model.augment(dataset_to_augment,
                                            image_column_name,
                                            target_column_name,
                                            backgrounds,
                                            options_for_augmentation)
    return augmented_data
