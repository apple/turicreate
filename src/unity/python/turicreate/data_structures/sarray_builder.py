# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
An interface for creating an SArray over time.
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from ..cython.cy_sarray_builder import UnitySArrayBuilderProxy
from .sarray import SArray

class SArrayBuilder(object):
    """
    An interface to incrementally build an SArray element by element.

    Once closed, the SArray cannot be "reopened" using this interface.

    Parameters
    ----------
    num_segments : int, optional
        Number of segments that can be written in parallel.

    history_size : int, optional
        The number of elements to be cached as history. Caches the last
        `history_size` elements added with `append` or `append_multiple`.

    dtype : type, optional
        The type the resulting SArray will be. If None, the resulting SArray
        will take on the type of the first non-None value it receives.
        
    Returns
    -------
    out : SArrayBuilder

    Examples
    --------
    >>> from turicreate import SArrayBuilder

    >>> sb = SArrayBuilder()

    >>> sb.append(1)

    >>> sb.append([2,3])

    >>> sb.close()
    dtype: int
    Rows: 3
    [1, 2, 3]

    """
    def __init__(self, dtype, num_segments=1, history_size=10):
        self._builder = UnitySArrayBuilderProxy()
        if dtype is None:
            dtype = type(None)
        self._builder.init(num_segments, history_size, dtype)
        self._block_size = 1024

    def append(self, data, segment=0):
        """
        Append a single element to an SArray.

        Throws a RuntimeError if the type of `data` is incompatible with
        the type of the SArray. 

        Parameters
        ----------
        data  : any SArray-supported type
            A data element to add to the SArray.

        segment : int
            The segment to write this element. Each segment is numbered
            sequentially, starting with 0. Any value in segment 1 will be after
            any value in segment 0, and the order of elements in each segment is
            preserved as they are added.
        """
        self._builder.append(data, segment)
        
    def append_multiple(self, data, segment=0):
        """
        Append multiple elements to an SArray.

        Throws a RuntimeError if the type of `data` is incompatible with
        the type of the SArray. 

        Parameters
        ----------
        data  : any SArray-supported type
            A data element to add to the SArray.

        segment : int
            The segment to write this element. Each segment is numbered
            sequentially, starting with 0. Any value in segment 1 will be after
            any value in segment 0, and the order of elements in each segment is
            preserved as they are added.
        """
        if not hasattr(data, '__iter__'):
            raise TypeError("append_multiple must be passed an iterable object")
        tmp_list = []
        block_pos = 0

        for i in data:
            tmp_list.append(i)
            if len(tmp_list) >= self._block_size:
                self._builder.append_multiple(tmp_list, segment)
                tmp_list = []
        if len(tmp_list) > 0:
            self._builder.append_multiple(tmp_list, segment)

    def get_type(self):
        """
        The type the result SArray will be if `close` is called.
        """
        return self._builder.get_type()

    def read_history(self, num=10, segment=0):
        """
        Outputs the last `num` elements that were appended either by `append` or
        `append_multiple`.

        Returns
        -------
        out : list

        """
        if num < 0:
            num = 0
        if segment < 0:
            raise TypeError("segment must be >= 0")
        return self._builder.read_history(num, segment)

    def close(self):
        """
        Creates an SArray from all values that were appended to the
        SArrayBuilder. No function that appends data may be called after this
        is called.

        Returns
        -------
        out : SArray

        """
        return SArray(_proxy=self._builder.close())
