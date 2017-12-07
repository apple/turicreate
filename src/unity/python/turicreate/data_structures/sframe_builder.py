# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
An interface for creating an SFrame over time.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ..cython.cy_sframe_builder import UnitySFrameBuilderProxy
from .sframe import SFrame 
from ..util import _make_internal_url
import logging as _logging

__LOGGER__ = _logging.getLogger(__name__)

class SFrameBuilder(object):
    """
    An interface to incrementally build an SFrame (row by row). It has some
    basic features such as appending multiple rows at once, polling the last N
    rows appended, and a primitive parallel insert interface.

    Once closed, the SFrame cannot be "reopened" using this interface.

    Parameters
    ----------
    column_types : list[type]
        The column types of the result SFrame. Types must be of the python
        types that are supported by SFrame (int, float, str, array.array, list,
        dict, datetime.datetime, image). Column types are strictly enforced
        while appending.

    column_names : list[str], optional
        Column names of the result SFrame. If given, length of the list must
        equal the length of `column_types`. If not given, names are generated.

    num_segments : int, optional
        Number of segments that can be written in parallel.

    history_size : int, optional
        The number of rows to be cached as history. Caches the last
        `history_size` rows added with `append` or `append_multiple`.

    Returns
    -------
    out : SFrameBuilder

    Examples
    --------
    >>> from turicreate import SFrameBuilder

    >>> sb = SFrameBuilder([int,float,str])

    >>> sb.append([1,1.0,"1"])

    >>> sb.append_multiple([[2,2.0,"2"],[3,3.0,"3"]])

    >>> sb.close()
    Columns:
            X1      int
            X2      float
            X3      str
    Rows: 3
    Data:
    +----+-----+----+
    | X1 |  X2 | X3 |
    +----+-----+----+
    | 1  | 1.0 | 1  |
    | 2  | 2.0 | 2  |
    | 3  | 3.0 | 3  |
    +----+-----+----+
    [3 rows x 3 columns]

    """
    def __init__(self, column_types, column_names=None, num_segments=1,
            history_size=10, save_location=None):
        self._column_names = column_names
        self._column_types = column_types
        self._num_segments = num_segments
        self._history_size = history_size
        if save_location is None:
            self._save_location = ""
        else:
            self._save_location = _make_internal_url(save_location)

        if column_names is not None and column_types is not None:
            if len(column_names) != len(column_types):
                raise AssertionError("There must be same amount of column names as column types.")
        elif column_names is None and column_types is not None:
            self._column_names = self._generate_column_names(len(column_types))
        else:
            raise AssertionError("Column types must be defined!")

        self._builder = UnitySFrameBuilderProxy()
        self._builder.init(self._column_types,
                           self._column_names,
                           self._num_segments,
                           self._history_size,
                           self._save_location)
        self._block_size = 1024

    def _generate_column_names(self, num_columns):
        return ["X"+str(i) for i in range(1,num_columns+1)]

    def append(self, data, segment=0):
        """
        Append a single row to an SFrame.

        Throws a RuntimeError if one or more column's type is incompatible with
        a type appended.

        Parameters
        ----------
        data  : iterable
            An iterable representation of a single row.

        segment : int
            The segment to write this row. Each segment is numbered
            sequentially, starting with 0. Any value in segment 1 will be after
            any value in segment 0, and the order of rows in each segment is
            preserved as they are added.
        """
        # Assume this case refers to an SFrame with a single column
        if not hasattr(data, '__iter__'):
            data = [data]
        self._builder.append(data, segment)

    def append_multiple(self, data, segment=0):
        """
        Append multiple rows to an SFrame.

        Throws a RuntimeError if one or more column's type is incompatible with
        a type appended.

        Parameters
        ----------
        data  : iterable[iterable]
            A collection of multiple iterables, each representing a single row.

        segment : int
            The segment to write the given rows. Each segment is numbered
            sequentially, starting with 0. Any value in segment 1 will be after
            any value in segment 0, and the order of rows in each segment is
            preserved as they are added.
        """
        if not hasattr(data, '__iter__'):
            raise TypeError("append_multiple must be passed an iterable object")
        tmp_list = []
        block_pos = 0

        # Avoid copy in cases that we are passed materialized data that is
        # smaller than our block size
        if hasattr(data, '__len__'):
            if len(data) <= self._block_size:
                self._builder.append_multiple(data, segment)
                return

        for i in data:
            tmp_list.append(i)
            if len(tmp_list) >= self._block_size:
                self._builder.append_multiple(tmp_list, segment)
                tmp_list = []
        if len(tmp_list) > 0:
            self._builder.append_multiple(tmp_list, segment)

    def column_names(self):
        return self._builder.column_names()

    def column_types(self):
        return self._builder.column_types()

    def read_history(self, num=10, segment=0):
        """
        Outputs the last `num` rows that were appended either by `append` or
        `append_multiple`.

        Returns
        -------
        out : list[list]
        """
        if num < 0:
          num = 0
        return self._builder.read_history(num, segment)

    def close(self):
        """
        Creates an SFrame from all values that were appended to the
        SFrameBuilder. No function that appends data may be called after this
        is called.

        Returns
        -------
        out : SFrame
        """
        return SFrame(_proxy=self._builder.close())

