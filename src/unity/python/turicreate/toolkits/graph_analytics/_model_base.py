# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Model base for graph analytics models
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from turicreate.toolkits._model import CustomModel
from prettytable import PrettyTable as _PrettyTable
from turicreate.cython.cy_graph import UnityGraphProxy
from turicreate.cython.cy_sframe import UnitySFrameProxy
import turicreate.toolkits._main as _main
from turicreate.data_structures.sframe import SFrame
from turicreate.data_structures.sgraph import SGraph
from turicreate.toolkits._internal_utils import _precomputed_field, _toolkit_repr_print

import six

class GraphAnalyticsModel(CustomModel):

    @classmethod
    def _native_name(cls):
        return None

    def _get(self, field):
        """
        Return the value for the queried field.

        Get the value of a given field. The list of all queryable fields is
        documented in the beginning of the model class.

        >>> out = m._get('graph')

        Parameters
        ----------
        field : string
            Name of the field to be retrieved.

        Returns
        -------
        out : value
            The current value of the requested field.
        """
        if field in self._list_fields():
            obj = self.__proxy__.get(field)
            if type(obj) == UnityGraphProxy:
                return SGraph(_proxy=obj)
            elif type(obj) == UnitySFrameProxy:
                return SFrame(_proxy=obj)
            else:
                return obj
        else:
            raise KeyError('Key \"%s\" not in model. Available fields are %s.' % (field, ', '.join(self._list_fields())))

    @classmethod
    def _describe_fields(cls):
        """
        Return a dictionary for the class fields description.
        Fields should NOT be wrapped by _precomputed_field, if necessary
        """
        dispatch_table = {
            'ShortestPathModel': 'sssp_model_fields',
            'GraphColoringModel': 'graph_coloring_model_fields',
            'PagerankModel': 'pagerank_model_fields',
            'ConnectedComponentsModel': 'connected_components_model_fields',
            'TriangleCountingModel': 'triangle_counting_model_fields',
            'KcoreModel': 'kcore_model_fields',
            'DegreeCountingModel': 'degree_count_model_fields',
            'LabelPropagationModel': 'label_propagation_model_fields'
        }
        try:
            fields_description = _main.run(dispatch_table[cls.__name__], {})
            return fields_description
        except:
            raise RuntimeError('Model %s does not have fields description' % cls.__name__)

    def _format(self, title, key_values):
        if len(key_values) == 0:
            return ""
        tbl = _PrettyTable(header=False)
        for k, v in six.iteritems(key_values):
                tbl.add_row([k, v])
        tbl.align['Field 1'] = 'l'
        tbl.align['Field 2'] = 'l'
        s = title + ":\n"
        s += tbl.__str__() + '\n'
        return s

    def _get_summary_struct(self):
        """
        Returns a structured description of the model, including (where relevant)
        the schema of the training data, description of the training data,
        training statistics, and model hyperparameters.

        Returns
        -------
        sections : list (of list of tuples)
            A list of summary sections.
              Each section is a list.
                Each item in a section list is a tuple of the form:
                  ('<label>','<field>')
        section_titles: list
            A list of section titles.
              The order matches that of the 'sections' object.
        """
        g = self.graph

        section_titles = ['Graph']

        graph_summary = [(k, _precomputed_field(v)) for k, v in six.iteritems(g.summary())]

        sections = [graph_summary]

        # collect other sections
        results = [(k, _precomputed_field(v)) for k, v in six.iteritems(self._result_fields())]
        methods = [(k, _precomputed_field(v)) for k, v in six.iteritems(self._method_fields())]
        settings = [(k, v) for k, v in six.iteritems(self._setting_fields())]
        metrics = [(k, v) for k, v in six.iteritems(self._metric_fields())]

        optional_sections = [('Results', results), ('Settings', settings), \
                                ('Metrics', metrics), ('Methods', methods)]

        # if section is not empty, append to summary structure
        for (title, section) in optional_sections:
            if len(section) > 0:
                section_titles.append(title)
                sections.append(section)

        return (sections, section_titles)

    def __repr__(self):

        descriptions = [(k, _precomputed_field(v)) for k, v in six.iteritems(self._describe_fields())]

        (sections, section_titles) = self._get_summary_struct()
        non_empty_sections = [s for s in sections if len(s) > 0]
        non_empty_section_titles = [section_titles[i] for i in range(len(sections)) if len(sections[i]) > 0]

        non_empty_section_titles.append('Queryable Fields')
        non_empty_sections.append(descriptions)

        return _toolkit_repr_print(self, non_empty_sections, non_empty_section_titles, width=40)

    def __str__(self):
        return self.__repr__()

    def _setting_fields(self):
        """
        Return model fields related to input setting
        Fields SHOULD be wrapped by _precomputed_field, if necessary
        """
        return dict()

    def _method_fields(self):
        """
        Return model fields related to model methods
        Fields should NOT be wrapped by _precomputed_field
        """
        return dict()

    def _result_fields(self):
        """
        Return results information
        Fields should NOT be wrapped by _precomputed_field
        """
        return {'graph': "SGraph. See m['graph']"}

    def _metric_fields(self):
        """
        Return model fields related to training metric
        Fields SHOULD be wrapped by _precomputed_field, if necessary
        """
        return {'training time (secs)': 'training_time'}
