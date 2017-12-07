# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
An interface for accessing an SFrame that is grouped by the values it contains
in one or more columns.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
class GroupedSFrame(object):
    """
    Left undocumented intentionally.
    """
    def __init__(self, sframe, key_columns):
        from .. import extensions
        self._sf_group = extensions.grouped_sframe()
        if isinstance(key_columns, str):
            key_columns = [key_columns]

        if not isinstance(key_columns, list):
            raise TypeError("Must give key columns as str or list.")
        self._sf_group.group(sframe, key_columns, False)

    def get_group(self, name):
        if not isinstance(name, list):
            name = [name]

        name.append(None)
        return self._sf_group.get_group(name)

    def groups(self):
        return self._sf_group.groups()

    def num_groups(self):
        return self._sf_group.num_groups()

    def __iter__(self):
        def generator():
            elems_at_a_time = 16
            self._sf_group.begin_iterator()
            ret = self._sf_group.iterator_get_next(elems_at_a_time)
            while(True):
                for j in ret:
                    yield tuple(j)

                if len(ret) == elems_at_a_time:
                    ret = self._sf_group.iterator_get_next(elems_at_a_time)
                else:
                    break

        return generator()
