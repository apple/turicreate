# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

def preview_augmented_images(data, target, n=1):
    """
    A utility function that would allow the user to visualize 
    `n` augmented images per row of the data if data is an SFrame,
    and just one augmented image if the data is an Image.

    Parameters
    ----------

    dataset : SFrame | tc.Image
    target : string
    n : int
        The number of augmented images per row of data to generate.
    """
    pass