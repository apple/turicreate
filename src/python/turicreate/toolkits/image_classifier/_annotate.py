# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Class definition and utilities for the annotation utility of the image classification toolkit
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ...visualization import _get_client_app_path
import turicreate.toolkits._internal_utils as _tkutl
from turicreate._cython.cy_server import QuietProgress as _QuietProgress

import turicreate as __tc


def annotate(data, image_column=None, annotation_column="annotations"):
    """
    Annotate images using a GUI assisted application. When the GUI is
    terminated an SFrame with the representative images and annotations is
    returned.

    Parameters
    ----------
    data : SArray | SFrame
        The data containing the input images.

    image_column: string, optional
        The name of the input column in the SFrame that contains the image that
        needs to be annotated. In case `data` is of type SArray, then the
        output SFrame contains a column (with this name) containing the input
        images.

    annotation_column : string, optional
        The column containing the annotations in the output SFrame.

    Returns
    -------
    out : SFrame
        A new SFrame that contains the newly annotated data.

    Examples
    --------
    >>> import turicreate as tc
    >>> images = tc.image_analysis.load_images("path/to/images")
    >>> print(images)
        +------------------------+--------------------------+
        |          path          |          image           |
        +------------------------+--------------------------+
        | /Users/username/Doc... | Height: 1712 Width: 1952 |
        | /Users/username/Doc... | Height: 1386 Width: 1000 |
        | /Users/username/Doc... |  Height: 536 Width: 858  |
        | /Users/username/Doc... | Height: 1512 Width: 2680 |
        +------------------------+--------------------------+
        [4 rows x 2 columns]

    >>> images = tc.image_classifier.annotate(images)
    >>> print(images)
        +------------------------+--------------------------+-------------------+
        |          path          |          image           |    annotations    |
        +------------------------+--------------------------+-------------------+
        | /Users/username/Doc... | Height: 1712 Width: 1952 |        dog        |
        | /Users/username/Doc... | Height: 1386 Width: 1000 |        dog        |
        | /Users/username/Doc... |  Height: 536 Width: 858  |        cat        |
        | /Users/username/Doc... | Height: 1512 Width: 2680 |       mouse       |
        +------------------------+--------------------------+-------------------+
        [4 rows x 3 columns]

    """
    # Check Value of Column Variables
    if not isinstance(data, __tc.SFrame):
        raise TypeError('"data" must be of type SFrame.')

    # Check if Value is Empty
    if data.num_rows() == 0:
        raise Exception("input data cannot be empty")

    if image_column == None:
        image_column = _tkutl._find_only_image_column(data)

    if image_column == None:
        raise ValueError("'image_column' cannot be 'None'")

    if type(image_column) != str:
        raise TypeError("'image_column' has to be of type 'str'")

    if annotation_column == None:
        annotation_column = ""

    if type(annotation_column) != str:
        raise TypeError("'annotation_column' has to be of type 'str'")

    # Check Data Structure
    if type(data) == __tc.data_structures.image.Image:
        data = __tc.SFrame({image_column: __tc.SArray([data])})

    elif type(data) == __tc.data_structures.sframe.SFrame:
        if data.shape[0] == 0:
            return data
        if not (data[image_column].dtype == __tc.data_structures.image.Image):
            raise TypeError("'data[image_column]' must be an SFrame or SArray")

    elif type(data) == __tc.data_structures.sarray.SArray:
        if data.shape[0] == 0:
            return data

        data = __tc.SFrame({image_column: data})
    else:
        raise TypeError("'data' must be an SFrame or SArray")

    annotation_window = __tc.extensions.create_image_classification_annotation(
        data, [image_column], annotation_column
    )

    with _QuietProgress(False):
        annotation_window.annotate(_get_client_app_path())
        return annotation_window.returnAnnotations()


def recover_annotation():
    """
    Recover the last annotated SFrame.

    If you annotate an SFrame and forget to assign it to a variable, this
    function allows you to recover the last annotated SFrame.

    Returns
    -------
    out : SFrame
        A new SFrame that contains the recovered annotation data.

    Examples
    --------
    >>> annotations = tc.image_classifier.recover_annotation()
    >>> print(annotations)
    +----------------------+-------------+
    |        images        | annotations |
    +----------------------+-------------+
    | Height: 28 Width: 28 |     Cat     |
    | Height: 28 Width: 28 |     Dog     |
    | Height: 28 Width: 28 |    Mouse    |
    | Height: 28 Width: 28 |   Feather   |
    | Height: 28 Width: 28 |     Bird    |
    | Height: 28 Width: 28 |     Cat     |
    | Height: 28 Width: 28 |     Cat     |
    | Height: 28 Width: 28 |     Dog     |
    | Height: 28 Width: 28 |     Cat     |
    | Height: 28 Width: 28 |     Bird    |
    +----------------------+-------------+
    [400 rows x 3 columns]

    """
    empty_instance = __tc.extensions.ImageClassification()
    annotation_wrapper = empty_instance.get_annotation_registry()
    return annotation_wrapper.annotation_sframe
