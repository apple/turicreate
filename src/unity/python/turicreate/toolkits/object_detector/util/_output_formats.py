# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as _tc
from turicreate.toolkits._internal_utils import (_raise_error_if_not_sframe,
                                                 _raise_error_if_not_sarray)


def stack_annotations(annotations_sarray):
    """
    Converts object detection annotations (ground truth or predictions) to
    stacked format (an `SFrame` where each row is one object instance).

    Parameters
    ----------
    annotations_sarray: SArray
        An `SArray` with unstacked predictions, exactly formatted as the
        annotations column when training an object detector or when making
        predictions.

    Returns
    -------
    annotations_sframe: An `SFrame` with stacked annotations.

    See also
    --------
    unstack_annotations

    Examples
    --------
    Predictions are returned by the object detector in unstacked format:

    >>> predictions = detector.predict(images)

    By converting it to stacked format, it is easier to get an overview of
    object instances:

    >>> turicreate.object_detector.util.stack_annotations(predictions)
    Data:
    +--------+------------+-------+-------+-------+-------+--------+
    | row_id | confidence | label |   x   |   y   | width | height |
    +--------+------------+-------+-------+-------+-------+--------+
    |   0    |    0.98    |  dog  | 123.0 | 128.0 |  80.0 | 182.0  |
    |   0    |    0.67    |  cat  | 150.0 | 183.0 | 129.0 | 101.0  |
    |   1    |    0.8     |  dog  |  50.0 | 432.0 |  65.0 |  98.0  |
    +--------+------------+-------+-------+-------+-------+--------+
    [3 rows x 7 columns]
    """
    _raise_error_if_not_sarray(annotations_sarray, variable_name='annotations_sarray')
    sf = _tc.SFrame({'annotations': annotations_sarray}).add_row_number('row_id')
    sf = sf.stack('annotations', new_column_name='annotations', drop_na=True)
    if len(sf) == 0:
        cols = ['row_id', 'confidence', 'label', 'height', 'width', 'x', 'y']
        return _tc.SFrame({k: [] for k in cols})
    sf = sf.unpack('annotations', column_name_prefix='')
    sf = sf.unpack('coordinates', column_name_prefix='')
    del sf['type']
    return sf


def unstack_annotations(annotations_sframe, num_rows=None):
    """
    Converts object detection annotations (ground truth or predictions) to
    unstacked format (an `SArray` where each element is a list of object
    instances).

    Parameters
    ----------
    annotations_sframe: SFrame
        An `SFrame` with stacked predictions, produced by the
        `stack_annotations` function.

    num_rows: int
        Optionally specify the number of rows in your original dataset, so that
        all get represented in the unstacked format, regardless of whether or
        not they had instances or not.

    Returns
    -------
    annotations_sarray: An `SArray` with unstacked annotations.

    See also
    --------
    stack_annotations

    Examples
    --------
    If you have annotations in stacked format:

    >>> stacked_predictions
    Data:
    +--------+------------+-------+-------+-------+-------+--------+
    | row_id | confidence | label |   x   |   y   | width | height |
    +--------+------------+-------+-------+-------+-------+--------+
    |   0    |    0.98    |  dog  | 123.0 | 128.0 |  80.0 | 182.0  |
    |   0    |    0.67    |  cat  | 150.0 | 183.0 | 129.0 | 101.0  |
    |   1    |    0.8     |  dog  |  50.0 | 432.0 |  65.0 |  98.0  |
    +--------+------------+-------+-------+-------+-------+--------+
    [3 rows x 7 columns]

    They can be converted to unstacked format using this function:

    >>> turicreate.object_detector.util.unstack_annotations(stacked_predictions)[0]
    [{'confidence': 0.98,
      'coordinates': {'height': 182.0, 'width': 80.0, 'x': 123.0, 'y': 128.0},
      'label': 'dog',
      'type': 'rectangle'},
     {'confidence': 0.67,
      'coordinates': {'height': 101.0, 'width': 129.0, 'x': 150.0, 'y': 183.0},
      'label': 'cat',
      'type': 'rectangle'}]
    """
    _raise_error_if_not_sframe(annotations_sframe, variable_name="annotations_sframe")

    cols = ['label', 'type', 'coordinates']
    has_confidence = 'confidence' in annotations_sframe.column_names()
    if has_confidence:
        cols.append('confidence')

    if num_rows is None:
        if len(annotations_sframe) == 0:
            num_rows = 0
        else:
            num_rows = annotations_sframe['row_id'].max() + 1

    sf = annotations_sframe
    sf['type'] = 'rectangle'
    sf = sf.pack_columns(['x', 'y', 'width', 'height'], dtype=dict,
                         new_column_name='coordinates')
    sf = sf.pack_columns(cols, dtype=dict, new_column_name='ann')
    sf = sf.unstack('ann', new_column_name='annotations')
    sf_all_ids = _tc.SFrame({'row_id': range(num_rows)})
    sf = sf.join(sf_all_ids, on='row_id', how='right')
    sf = sf.fillna('annotations', [])
    sf = sf.sort('row_id')

    annotations_sarray = sf['annotations']
    # Sort the confidences again, since the unstack does not preserve the order
    if has_confidence:
        annotations_sarray = annotations_sarray.apply(
                lambda x: sorted(x, key=lambda ann: ann['confidence'], reverse=True),
                dtype=list)
    return annotations_sarray
