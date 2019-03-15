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

import turicreate as __tc

def _warning_annotations():
    print(
        """
        *** WARNING: If you don't assign the output of the annotate function
                     to a variable all annotations are going to be lost.
                     To recover the last annotated SFrame call:

        `tc.image_classifier.recover_annotation()`

        """
    )

def annotate(data, image_column=None, annotation_column='annotations'):
    """
        Annotate your images loaded in either an SFrame or SArray Format

        The annotate util is a GUI assisted application used to create labels in
        SArray Image data. Specifying a column, with dtype Image, in an SFrame
        works as well since SFrames are composed of multiple SArrays.

        When the GUI is terminated an SFrame is returned with the representative,
        images and annotations.

        The returned SFrame includes the newly created annotations.

        Parameters
        --------------
        data : SArray | SFrame
            The data containing the images. If the data type is 'SArray'
            the 'image_column', and 'annotation_column' variables are used to construct
            a new 'SFrame' containing the 'SArray' data for annotation.
            If the data type is 'SFrame' the 'image_column', and 'annotation_column'
            variables are used to annotate the images.

        image_column: string, optional
            If the data type is SFrame and the 'image_column' parameter is specified
            then the column name is used as the image column used in the annotation. If
            the data type is 'SFrame' and the 'image_column' variable is left empty. A
            default column value of 'image' is used in the annotation. If the data type is
            'SArray', the 'image_column' is used to construct the 'SFrame' data for
            the annotation

        annotation_column : string, optional
            If the data type is SFrame and the 'annotation_column' parameter is specified
            then the column name is used as the annotation column used in the annotation. If
            the data type is 'SFrame' and the 'annotation_column' variable is left empty. A
            default column value of 'annotation' is used in the annotation. If the data type is
            'SArray', the 'annotation_column' is used to construct the 'SFrame' data for
            the annotation


        Returns
        -------

        out : SFrame
            A new SFrame that contains the newly annotated data.

        Examples
        --------

        >> import turicreate as tc
        >> images = tc.image_analysis.load_images("path/to/images")
        >> print(images)

            Columns:

                path    str
                image   Image

            Rows: 4

            Data:
            +------------------------+--------------------------+
            |          path          |          image           |
            +------------------------+--------------------------+
            | /Users/username/Doc... | Height: 1712 Width: 1952 |
            | /Users/username/Doc... | Height: 1386 Width: 1000 |
            | /Users/username/Doc... |  Height: 536 Width: 858  |
            | /Users/username/Doc... | Height: 1512 Width: 2680 |
            +------------------------+--------------------------+
            [4 rows x 2 columns]

        >> images = tc.image_classifier.annotate(images)
        >> print(images)

            Columns:
                path    str
                image   Image
                annotation  str

            Rows: 4

            Data:
            +------------------------+--------------------------+-------------------+
            |          path          |          image           |    annotation     |
            +------------------------+--------------------------+-------------------+
            | /Users/username/Doc... | Height: 1712 Width: 1952 |        dog        |
            | /Users/username/Doc... | Height: 1386 Width: 1000 |        dog        |
            | /Users/username/Doc... |  Height: 536 Width: 858  |        cat        |
            | /Users/username/Doc... | Height: 1512 Width: 2680 |       mouse       |
            +------------------------+--------------------------+-------------------+
            [4 rows x 3 columns]


    """

    

    # Check Value of Column Variables
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
        data = __tc.SFrame({image_column:__tc.SArray([data])})

    elif type(data) == __tc.data_structures.sframe.SFrame:
        if(data.shape[0] == 0):
            return data
        
        if not (data[image_column].dtype == __tc.data_structures.image.Image):
            raise TypeError("'data[image_column]' must be an SFrame or SArray")

    elif type(data) == __tc.data_structures.sarray.SArray:
        if(data.shape[0] == 0):
            return data

        data = __tc.SFrame({image_column:data})
    else:
        raise TypeError("'data' must be an SFrame or SArray")

    _warning_annotations()

    annotation_window = __tc.extensions.create_image_classification_annotation(
                            data,
                            [image_column],
                            annotation_column
                        )
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
        
        >> annotations = tc.image_classifier.recover_annotation()
        >> print(annotations)

        Columns:
            images  Image
            labels  int
            annotations str

        Rows: 400

        Data:
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
