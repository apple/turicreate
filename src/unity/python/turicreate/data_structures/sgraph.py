# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
This package defines the Turi Create SGraph, Vertex, and Edge objects. The SGraph
is a directed graph, consisting of a set of Vertex objects and Edges that
connect pairs of Vertices. The methods in this module are available from the top
level import of the turicreate package.
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from .. import connect as _mt
from ..connect import main as glconnect
from .sframe import SFrame
from .sarray import SArray
from .gframe import GFrame, VERTEX_GFRAME, EDGE_GFRAME
from ..cython.cy_graph import UnityGraphProxy
from ..cython.context import debug_trace as cython_context
from ..util import _is_non_string_iterable, _make_internal_url
from ..deps import pandas as pd
from ..deps import HAS_PANDAS

import inspect
import copy

import sys
if sys.version_info.major > 2:
    from functools import reduce

## \internal Default column name for vertex id.
_VID_COLUMN = '__id'

## \internal Default column name for source vid.
_SRC_VID_COLUMN = '__src_id'

## \internal Default column name for target vid.
_DST_VID_COLUMN = '__dst_id'


#/**************************************************************************/
#/*                                                                        */
#/*                         SGraph Related Classes                         */
#/*                                                                        */
#/**************************************************************************/
class Vertex(object):
    """
    A vertex object, consisting of a vertex ID and a dictionary of vertex
    attributes. The vertex ID can be an integer, string, or float.

    Parameters
    ----------
    vid : int or string or float
        Vertex ID.

    attr : dict, optional
        Vertex attributes. A Dictionary of string keys and values with one of
        the following types: int, float, string, array of floats.

    See Also
    --------
    Edge, SGraph

    Examples
    --------
    >>> from turicreate import SGraph, Vertex, Edge
    >>> g = SGraph()

    >>> verts = [Vertex(0, attr={'breed': 'labrador'}),
                 Vertex(1, attr={'breed': 'labrador'}),
                 Vertex(2, attr={'breed': 'vizsla'})]
    >>> g = g.add_vertices(verts)
    """

    __slots__ = ['vid', 'attr']

    def __init__(self, vid, attr={}, _series=None):
        """__init__(self, vid, attr={})
        Construct a new vertex.
        """
        if not _series is None:
            self.vid = _series[_VID_COLUMN]
            self.attr = _series.to_dict()
            self.attr.pop(_VID_COLUMN)
        else:
            self.vid = vid
            self.attr = attr

    def __repr__(self):
        return "V(" + str(self.vid) + ", " + str(self.attr) + ")"

    def __str__(self):
        return "V(" + str(self.vid) + ", " + str(self.attr) + ")"


class Edge(object):
    """
    A directed edge between two Vertex objects. An Edge object consists of a
    source vertex ID, a destination vertex ID, and a dictionary of edge
    attributes.

    Parameters
    ----------
    src_vid : int or string or float
        Source vertex ID.

    dst_vid : int or string or float
        Target vertex ID.

    attr : dict
        Edge attributes. A Dictionary of string keys and values with one of the
        following types: integer, float, string, array of floats.

    See Also
    --------
    Vertex, SGraph

    Examples
    --------
    >>> from turicreate import SGraph, Vertex, Edge

    >>> verts = [Vertex(0, attr={'breed': 'labrador'}),
                 Vertex(1, attr={'breed': 'vizsla'})]
    >>> edges = [Edge(0, 1, attr={'size': 'larger_than'})]

    >>> g = SGraph()
    >>> g = g.add_vertices(verts).add_edges(edges)
    """

    __slots__ = ['src_vid', 'dst_vid', 'attr']

    def __init__(self, src_vid, dst_vid, attr={}, _series=None):
        """__init__(self, vid, attr={})
        Construct a new edge.
        """
        if not _series is None:
            self.src_vid = _series[_SRC_VID_COLUMN]
            self.dst_vid = _series[_DST_VID_COLUMN]
            self.attr = _series.to_dict()
            self.attr.pop(_SRC_VID_COLUMN)
            self.attr.pop(_DST_VID_COLUMN)
        else:
            self.src_vid = src_vid
            self.dst_vid = dst_vid
            self.attr = attr

    def __repr__(self):
        return ("E(" + str(self.src_vid) + " -> " + str(self.dst_vid) + ", " +
                str(self.attr) + ")")

    def __str__(self):
        return ("E(" + str(self.src_vid) + " -> " + str(self.dst_vid) + ", " +
                str(self.attr) + ")")


class SGraph(object):
    """
    A scalable graph data structure. The SGraph data structure allows arbitrary
    dictionary attributes on vertices and edges, provides flexible vertex and
    edge query functions, and seamless transformation to and from
    :class:`~turicreate.SFrame`.

    There are several ways to create an SGraph. The simplest way is to make an
    empty SGraph then add vertices and edges with the :py:func:`add_vertices`
    and :py:func:`add_edges` methods. SGraphs can also be created from vertex
    and edge lists stored in :class:`~turicreate.SFrames`. Columns of these
    SFrames not used as vertex IDs are assumed to be vertex or edge attributes.

    Please see the `User Guide
    <https://apple.github.io/turicreate/docs/userguide/sgraph/sgraph.html>`_
    for a more detailed introduction to creating and working with SGraphs.

    Parameters
    ----------
    vertices : SFrame, optional
        Vertex data. Must include an ID column with the name specified by
        the `vid_field` parameter. Additional columns are treated as vertex
        attributes.

    edges : SFrame, optional
        Edge data. Must include source and destination ID columns as specified
        by `src_field` and `dst_field` parameters. Additional columns are treated
        as edge attributes.

    vid_field : str, optional
        The name of vertex ID column in the `vertices` SFrame.

    src_field : str, optional
        The name of source ID column in the `edges` SFrame.

    dst_field : str, optional
        The name of destination ID column in the `edges` SFrame.

    See Also
    --------
    SFrame

    Notes
    -----
    - SGraphs are *structurally immutable*. In the example below, the
      :func:`~add_vertices` and :func:`~add_edges` commands both return a new
      graph; the old graph gets garbage collected.

    Examples
    --------
    >>> from turicreate import SGraph, Vertex, Edge
    >>> g = SGraph()
    >>> verts = [Vertex(0, attr={'breed': 'labrador'}),
                 Vertex(1, attr={'breed': 'labrador'}),
                 Vertex(2, attr={'breed': 'vizsla'})]
    >>> g = g.add_vertices(verts)
    >>> g = g.add_edges(Edge(1, 2))
    """

    __slots__ = ['__proxy__', '_vertices', '_edges']

    def __init__(self, vertices=None, edges=None, vid_field='__id',
                 src_field='__src_id', dst_field='__dst_id', _proxy=None):
        """
        __init__(vertices=None, edges=None, vid_field='__id', src_field='__src_id', dst_field='__dst_id')

        By default, construct an empty graph when vertices and edges are None.
        Otherwise construct an SGraph with given vertices and edges.

        Parameters
        ----------
        vertices : SFrame, optional
            An SFrame containing vertex id columns and optional vertex data
            columns.

        edges : SFrame, optional
            An SFrame containing source and target id columns and optional edge
            data columns.

        vid_field : str, optional
            The name of vertex id column in the `vertices` SFrame.

        src_field : str, optional
            The name of source id column in the `edges` SFrame.

        dst_field : str, optional
            The name of target id column in the `edges` SFrame.
        """
        if (_proxy is None):
            self.__proxy__ = UnityGraphProxy()
            if vertices is not None:
                self.__proxy__ = self.add_vertices(vertices, vid_field).__proxy__
            if edges is not None:
                self.__proxy__ = self.add_edges(edges, src_field, dst_field).__proxy__
        else:
            self.__proxy__ = _proxy
        self._vertices = GFrame(self, VERTEX_GFRAME)
        self._edges = GFrame(self, EDGE_GFRAME)

    def __str__(self):
        """Returns a readable string representation summarizing the graph."""
        return "SGraph(%s)" % str(self.summary())

    def __repr__(self):
        """Returns a readable string representation summarizing the graph."""
        return "SGraph(%s)\nVertex Fields:%s\nEdge Fields:%s" % \
               (str(self.summary()), str(self.get_vertex_fields()), str(self.get_edge_fields()))

    def __copy__(self):
        return SGraph(_proxy=self.__proxy__)

    def __deepcopy__(self, memo):
        return self.__copy__()

    def copy(self):
        """
        Returns a shallow copy of the SGraph.
        """
        return self.__copy__()

    @property
    def vertices(self):
        """
        Special vertex SFrame of the SGraph. Modifying the contents of this
        SFrame changes the vertex data of the SGraph. To preserve the graph
        structure, the ``__id`` column of this SFrame is read-only.

        See Also
        --------
        edges

        Examples
        --------
        >>> from turicreate import SGraph, Vertex
        >>> g = SGraph().add_vertices([Vertex('cat', {'fluffy': 1}),
                                       Vertex('dog', {'fluffy': 1, 'woof': 1}),
                                       Vertex('hippo', {})])

        Copy the 'woof' vertex attribute into a new 'bark' vertex attribute:

        >>> g.vertices['bark'] = g.vertices['woof']

        Remove the 'woof' attribute:

        >>> del g.vertices['woof']

        Create a new field 'likes_fish':

        >>> g.vertices['likes_fish'] = g.vertices['__id'] == 'cat'
        +-------+--------+------+------------+
        |  __id | fluffy | bark | likes_fish |
        +-------+--------+------+------------+
        |  dog  |  1.0   | 1.0  |     0      |
        |  cat  |  1.0   | nan  |     1      |
        | hippo |  nan   | nan  |     0      |
        +-------+--------+------+------------+

        Replace missing values with zeros:

        >>> for col in g.vertices.column_names():
        ...     if col != '__id':
        ...         g.vertices.fillna(col, 0)
        +-------+--------+------+------------+
        |  __id | fluffy | bark | likes_fish |
        +-------+--------+------+------------+
        |  dog  |  1.0   | 1.0  |     0      |
        |  cat  |  1.0   | 0.0  |     1      |
        | hippo |  0.0   | 0.0  |     0      |
        +-------+--------+------+------------+
        """

        return self._vertices

    @property
    def edges(self):
        """
        Special edge SFrame of the SGraph. Modifying the contents of this SFrame
        changes the edge data of the SGraph. To preserve the graph structure,
        the ``__src_id``, and ``__dst_id`` columns of this SFrame are read-only.

        See Also
        --------
        vertices

        Examples
        --------
        >>> from turicreate import SGraph, Vertex, Edge
        >>> g = SGraph()
        >>> g = g.add_vertices([Vertex(x) for x in ['cat', 'dog', 'fossa']])
        >>> g = g.add_edges([Edge('cat', 'dog', attr={'relationship': 'dislikes'}),
                             Edge('dog', 'cat', attr={'relationship': 'likes'}),
                             Edge('dog', 'fossa', attr={'relationship': 'likes'})])
        >>> g.edges['size'] = ['smaller than', 'larger than', 'equal to']
        +----------+----------+--------------+--------------+
        | __src_id | __dst_id | relationship |     size     |
        +----------+----------+--------------+--------------+
        |   cat    |   dog    |   dislikes   | smaller than |
        |   dog    |   cat    |    likes     | larger than  |
        |   dog    |  fossa   |    likes     |   equal to   |
        +----------+----------+--------------+--------------+
        """

        return self._edges

    def summary(self):
        """
        Return the number of vertices and edges as a dictionary.

        Returns
        -------
        out : dict
            A dictionary containing the number of vertices and edges.

        See Also
        --------
        vertices, edges

        Examples
        --------
        >>> from turicreate import SGraph, Vertex
        >>> g = SGraph().add_vertices([Vertex(i) for i in range(10)])
        >>> n_vertex = g.summary()['num_vertices']
        10
        >>> n_edge = g.summary()['num_edges']
        0
        """
        ret = self.__proxy__.summary()
        return dict(ret.items())

    def get_vertices(self, ids=[], fields={}, format='sframe'):
        """
        get_vertices(self, ids=list(), fields={}, format='sframe')
        Return a collection of vertices and their attributes.

        Parameters
        ----------

        ids : list [int | float | str] or SArray
            List of vertex IDs to retrieve. Only vertices in this list will be
            returned. Also accepts a single vertex id.

        fields : dict | pandas.DataFrame
            Dictionary specifying equality constraint on field values. For
            example ``{'gender': 'M'}``, returns only vertices whose 'gender'
            field is 'M'. ``None`` can be used to designate a wild card. For
            example, {'relationship': None} will find all vertices with the
            field 'relationship' regardless of the value.

        format : {'sframe', 'list'}
            Output format. The SFrame output (default) contains a column
            ``__src_id`` with vertex IDs and a column for each vertex attribute.
            List output returns a list of Vertex objects.

        Returns
        -------
        out : SFrame or list [Vertex]
            An SFrame or list of Vertex objects.

        See Also
        --------
        vertices, get_edges

        Examples
        --------
        Return all vertices in the graph.

        >>> from turicreate import SGraph, Vertex
        >>> g = SGraph().add_vertices([Vertex(0, attr={'gender': 'M'}),
                                       Vertex(1, attr={'gender': 'F'}),
                                       Vertex(2, attr={'gender': 'F'})])
        >>> g.get_vertices()
        +------+--------+
        | __id | gender |
        +------+--------+
        |  0   |   M    |
        |  2   |   F    |
        |  1   |   F    |
        +------+--------+

        Return vertices 0 and 2.

        >>> g.get_vertices(ids=[0, 2])
        +------+--------+
        | __id | gender |
        +------+--------+
        |  0   |   M    |
        |  2   |   F    |
        +------+--------+

        Return vertices with the vertex attribute "gender" equal to "M".

        >>> g.get_vertices(fields={'gender': 'M'})
        +------+--------+
        | __id | gender |
        +------+--------+
        |  0   |   M    |
        +------+--------+
        """

        if not _is_non_string_iterable(ids):
            ids = [ids]

        if type(ids) not in (list, SArray):
            raise TypeError('ids must be list or SArray type')

        with cython_context():
            sf = SFrame(_proxy=self.__proxy__.get_vertices(ids, fields))

        if (format == 'sframe'):
            return sf
        elif (format == 'dataframe'):
            assert HAS_PANDAS, 'Cannot use dataframe because Pandas is not available or version is too low.'
            if sf.num_rows() == 0:
                return pd.DataFrame()
            else:
                df = sf.head(sf.num_rows()).to_dataframe()
                return df.set_index('__id')
        elif (format == 'list'):
            return _dataframe_to_vertex_list(sf.to_dataframe())
        else:
            raise ValueError("Invalid format specifier")

    def get_edges(self, src_ids=[], dst_ids=[], fields={}, format='sframe'):
        """
        get_edges(self, src_ids=list(), dst_ids=list(), fields={}, format='sframe')
        Return a collection of edges and their attributes. This function is used
        to find edges by vertex IDs, filter on edge attributes, or list in-out
        neighbors of vertex sets.

        Parameters
        ----------
        src_ids, dst_ids : list or SArray, optional
            Parallel arrays of vertex IDs, with each pair corresponding to an
            edge to fetch. Only edges in this list are returned. ``None`` can be
            used to designate a wild card. For instance, ``src_ids=[1, 2,
            None]``, ``dst_ids=[3, None, 5]`` will fetch the edge 1->3, all
            outgoing edges of 2 and all incoming edges of 5. src_id and dst_id
            may be left empty, which implies an array of all wild cards.

        fields : dict, optional
            Dictionary specifying equality constraints on field values. For
            example, ``{'relationship': 'following'}``, returns only edges whose
            'relationship' field equals 'following'. ``None`` can be used as a
            value to designate a wild card. e.g. ``{'relationship': None}`` will
            find all edges with the field 'relationship' regardless of the
            value.

        format : {'sframe', 'list'}, optional
            Output format. The 'sframe' output (default) contains columns
            __src_id and __dst_id with edge vertex IDs and a column for each
            edge attribute. List output returns a list of Edge objects.

        Returns
        -------
        out : SFrame | list [Edge]
            An SFrame or list of edges.

        See Also
        --------
        edges, get_vertices

        Examples
        --------
        Return all edges in the graph.

        >>> from turicreate import SGraph, Edge
        >>> g = SGraph().add_edges([Edge(0, 1, attr={'rating': 5}),
                                    Edge(0, 2, attr={'rating': 2}),
                                    Edge(1, 2)])
        >>> g.get_edges(src_ids=[None], dst_ids=[None])
        +----------+----------+--------+
        | __src_id | __dst_id | rating |
        +----------+----------+--------+
        |    0     |    2     |   2    |
        |    0     |    1     |   5    |
        |    1     |    2     |  None  |
        +----------+----------+--------+

        Return edges with the attribute "rating" of 5.

        >>> g.get_edges(fields={'rating': 5})
        +----------+----------+--------+
        | __src_id | __dst_id | rating |
        +----------+----------+--------+
        |    0     |    1     |   5    |
        +----------+----------+--------+

        Return edges 0 --> 1 and 1 --> 2 (if present in the graph).

        >>> g.get_edges(src_ids=[0, 1], dst_ids=[1, 2])
        +----------+----------+--------+
        | __src_id | __dst_id | rating |
        +----------+----------+--------+
        |    0     |    1     |   5    |
        |    1     |    2     |  None  |
        +----------+----------+--------+
        """

        if not _is_non_string_iterable(src_ids):
            src_ids = [src_ids]
        if not _is_non_string_iterable(dst_ids):
            dst_ids = [dst_ids]

        if type(src_ids) not in (list, SArray):
            raise TypeError('src_ids must be list or SArray type')
        if type(dst_ids) not in (list, SArray):
            raise TypeError('dst_ids must be list or SArray type')

        # implicit Nones
        if len(src_ids) == 0 and len(dst_ids) > 0:
            src_ids = [None] * len(dst_ids)
        # implicit Nones
        if len(dst_ids) == 0 and len(src_ids) > 0:
            dst_ids = [None] * len(src_ids)

        with cython_context():
            sf = SFrame(_proxy=self.__proxy__.get_edges(src_ids, dst_ids, fields))

        if (format == 'sframe'):
            return sf
        if (format == 'dataframe'):
            assert HAS_PANDAS, 'Cannot use dataframe because Pandas is not available or version is too low.'
            if sf.num_rows() == 0:
                return pd.DataFrame()
            else:
                return sf.head(sf.num_rows()).to_dataframe()
        elif (format == 'list'):
            return _dataframe_to_edge_list(sf.to_dataframe())
        else:
            raise ValueError("Invalid format specifier")

    def add_vertices(self, vertices, vid_field=None):
        """
        Add vertices to the SGraph. Vertices should be input as a list of
        :class:`~turicreate.Vertex` objects, an :class:`~turicreate.SFrame`, or a
        pandas DataFrame. If vertices are specified by SFrame or DataFrame,
        ``vid_field`` specifies which column contains the vertex ID. Remaining
        columns are assumed to hold additional vertex attributes. If these
        attributes are not already present in the graph's vertex data, they are
        added, with existing vertices acquiring the value ``None``.

        Parameters
        ----------
        vertices : Vertex | list [Vertex] | pandas.DataFrame | SFrame
            Vertex data. If the vertices are in an SFrame or DataFrame, then
            ``vid_field`` specifies the column containing the vertex IDs.
            Additional columns are treated as vertex attributes.

        vid_field : string, optional
            Column in the DataFrame or SFrame to use as vertex ID. Required if
            vertices is an SFrame. If ``vertices`` is a DataFrame and
            ``vid_field`` is not specified, the row index is used as vertex ID.

        Returns
        -------
        out : SGraph
            A new SGraph with vertices added.

        See Also
        --------
        vertices, SFrame, add_edges

        Notes
        -----
        - If vertices are added with indices that already exist in the graph,
          they are overwritten completely. All attributes for these vertices
          will conform to the specification in this method.

        Examples
        --------
        >>> from turicreate import SGraph, Vertex, SFrame
        >>> g = SGraph()

        Add a single vertex.

        >>> g = g.add_vertices(Vertex(0, attr={'breed': 'labrador'}))

        Add a list of vertices.

        >>> verts = [Vertex(0, attr={'breed': 'labrador'}),
                     Vertex(1, attr={'breed': 'labrador'}),
                     Vertex(2, attr={'breed': 'vizsla'})]
        >>> g = g.add_vertices(verts)

        Add vertices from an SFrame.

        >>> sf_vert = SFrame({'id': [0, 1, 2], 'breed':['lab', 'lab', 'vizsla']})
        >>> g = g.add_vertices(sf_vert, vid_field='id')
        """

        sf = _vertex_data_to_sframe(vertices, vid_field)

        with cython_context():
            proxy = self.__proxy__.add_vertices(sf.__proxy__, _VID_COLUMN)
            return SGraph(_proxy=proxy)

    def add_edges(self, edges, src_field=None, dst_field=None):
        """
        Add edges to the SGraph. Edges should be input as a list of
        :class:`~turicreate.Edge` objects, an :class:`~turicreate.SFrame`, or a
        Pandas DataFrame. If the new edges are in an SFrame or DataFrame, then
        ``src_field`` and ``dst_field`` are required to specify the columns that
        contain the source and destination vertex IDs; additional columns are
        treated as edge attributes. If these attributes are not already present
        in the graph's edge data, they are added, with existing edges acquiring
        the value ``None``.

        Parameters
        ----------
        edges : Edge | list [Edge] | pandas.DataFrame | SFrame
            Edge data. If the edges are in an SFrame or DataFrame, then
            ``src_field`` and ``dst_field`` are required to specify the columns
            that contain the source and destination vertex IDs. Additional
            columns are treated as edge attributes.

        src_field : string, optional
            Column in the SFrame or DataFrame to use as source vertex IDs. Not
            required if ``edges`` is a list.

        dst_field : string, optional
            Column in the SFrame or Pandas DataFrame to use as destination
            vertex IDs. Not required if ``edges`` is a list.

        Returns
        -------
        out : SGraph
            A new SGraph with `edges` added.

        See Also
        --------
        edges, SFrame, add_vertices

        Notes
        -----
        - If an edge is added whose source and destination IDs match edges that
          already exist in the graph, a new edge is added to the graph. This
          contrasts with :py:func:`add_vertices`, which overwrites existing
          vertices.

        Examples
        --------
        >>> from turicreate import SGraph, Vertex, Edge, SFrame
        >>> g = SGraph()
        >>> verts = [Vertex(0, attr={'breed': 'labrador'}),
                     Vertex(1, attr={'breed': 'labrador'}),
                     Vertex(2, attr={'breed': 'vizsla'})]
        >>> g = g.add_vertices(verts)

        Add a single edge.

        >>> g = g.add_edges(Edge(1, 2))

        Add a list of edges.

        >>> g = g.add_edges([Edge(0, 2), Edge(1, 2)])

        Add edges from an SFrame.

        >>> sf_edge = SFrame({'source': [0, 1], 'dest': [2, 2]})
        >>> g = g.add_edges(sf_edge, src_field='source', dst_field='dest')
        """

        sf = _edge_data_to_sframe(edges, src_field, dst_field)

        with cython_context():
            proxy = self.__proxy__.add_edges(sf.__proxy__, _SRC_VID_COLUMN, _DST_VID_COLUMN)
            return SGraph(_proxy=proxy)

    def get_fields(self):
        """
        Return a list of vertex and edge attribute fields in the SGraph. If a
        field is common to both vertex and edge attributes, it will show up
        twice in the returned list.

        Returns
        -------
        out : list
            Names of fields contained in the vertex or edge data.

        See Also
        --------
        get_vertex_fields, get_edge_fields

        Examples
        --------
        >>> from turicreate import SGraph, Vertex, Edge
        >>> g = SGraph()
        >>> verts = [Vertex(0, attr={'name': 'alex'}),
                     Vertex(1, attr={'name': 'barbara'})]
        >>> g = g.add_vertices(verts)
        >>> g = g.add_edges(Edge(0, 1, attr={'frequency': 6}))
        >>> fields = g.get_fields()
        ['__id', 'name', '__src_id', '__dst_id', 'frequency']
        """

        return self.get_vertex_fields() + self.get_edge_fields()

    def get_vertex_fields(self):
        """
        Return a list of vertex attribute fields in the SGraph.

        Returns
        -------
        out : list
            Names of fields contained in the vertex data.

        See Also
        --------
        get_fields, get_edge_fields

        Examples
        --------
        >>> from turicreate import SGraph, Vertex, Edge
        >>> g = SGraph()
        >>> verts = [Vertex(0, attr={'name': 'alex'}),
                     Vertex(1, attr={'name': 'barbara'})]
        >>> g = g.add_vertices(verts)
        >>> g = g.add_edges(Edge(0, 1, attr={'frequency': 6}))
        >>> fields = g.get_vertex_fields()
        ['__id', 'name']
        """

        with cython_context():
            return self.__proxy__.get_vertex_fields()

    def get_edge_fields(self):
        """
        Return a list of edge attribute fields in the graph.

        Returns
        -------
        out : list
            Names of fields contained in the vertex data.

        See Also
        --------
        get_fields, get_vertex_fields

        Examples
        --------
        >>> from turicreate import SGraph, Vertex, Edge
        >>> g = SGraph()
        >>> verts = [Vertex(0, attr={'name': 'alex'}),
                     Vertex(1, attr={'name': 'barbara'})]
        >>> g = g.add_vertices(verts)
        >>> g = g.add_edges(Edge(0, 1, attr={'frequency': 6}))
        >>> fields = g.get_vertex_fields()
        ['__src_id', '__dst_id', 'frequency']
        """

        with cython_context():
            return self.__proxy__.get_edge_fields()

    def select_fields(self, fields):
        """
        Return a new SGraph with only the selected fields. Other fields are
        discarded, while fields that do not exist in the SGraph are ignored.

        Parameters
        ----------
        fields : string | list [string]
            A single field name or a list of field names to select.

        Returns
        -------
        out : SGraph
            A new graph whose vertex and edge data are projected to the selected
            fields.

        See Also
        --------
        get_fields, get_vertex_fields, get_edge_fields

        Examples
        --------
        >>> from turicreate import SGraph, Vertex
        >>> verts = [Vertex(0, attr={'breed': 'labrador', 'age': 5}),
                     Vertex(1, attr={'breed': 'labrador', 'age': 3}),
                     Vertex(2, attr={'breed': 'vizsla', 'age': 8})]
        >>> g = SGraph()
        >>> g = g.add_vertices(verts)
        >>> g2 = g.select_fields(fields=['breed'])
        """

        if (type(fields) is str):
            fields = [fields]
        if not isinstance(fields, list) or not all(type(x) is str for x in fields):
            raise TypeError('\"fields\" must be a str or list[str]')

        vfields = self.__proxy__.get_vertex_fields()
        efields = self.__proxy__.get_edge_fields()
        selected_vfields = []
        selected_efields = []
        for f in fields:
            found = False
            if f in vfields:
                selected_vfields.append(f)
                found = True
            if f in efields:
                selected_efields.append(f)
                found = True
            if not found:
                raise ValueError('Field \'%s\' not in graph' % f)

        with cython_context():
            proxy = self.__proxy__
            proxy = proxy.select_vertex_fields(selected_vfields)
            proxy = proxy.select_edge_fields(selected_efields)
            return SGraph(_proxy=proxy)

    def triple_apply(self, triple_apply_fn, mutated_fields, input_fields=None):
        '''
        Apply a transform function to each edge and its associated source and
        target vertices in parallel. Each edge is visited once and in parallel.
        Modification to vertex data is protected by lock. The effect on the
        returned SGraph is equivalent to the following pseudocode:

        >>> PARALLEL FOR (source, edge, target) AS triple in G:
        ...     LOCK (triple.source, triple.target)
        ...     (source, edge, target) = triple_apply_fn(triple)
        ...     UNLOCK (triple.source, triple.target)
        ... END PARALLEL FOR

        Parameters
        ----------
        triple_apply_fn : function : (dict, dict, dict) -> (dict, dict, dict)
            The function to apply to each triple of (source_vertex, edge,
            target_vertex). This function must take as input a tuple of
            (source_data, edge_data, target_data) and return a tuple of
            (new_source_data, new_edge_data, new_target_data). All variables in
            the both tuples must be of dict type.
            This can also be a toolkit extension function which is compiled
            as a native shared library using SDK.

        mutated_fields : list[str] | str
            Fields that ``triple_apply_fn`` will mutate. Note: columns that are
            actually mutated by the triple apply function but not specified in
            ``mutated_fields`` will have undetermined effects.

        input_fields : list[str] | str, optional
            Fields that ``triple_apply_fn`` will have access to.
            The default is ``None``, which grants access to all fields.
            ``mutated_fields`` will always be included in ``input_fields``.

        Returns
        -------
        out : SGraph
            A new SGraph with updated vertex and edge data. Only fields
            specified in the ``mutated_fields`` parameter are updated.

        Notes
        -----
        - ``triple_apply`` does not currently support creating new fields in the
          lambda function.

        Examples
        --------
        Import turicreate and set up the graph.

        >>> edges = turicreate.SFrame({'source': range(9), 'dest': range(1, 10)})
        >>> g = turicreate.SGraph()
        >>> g = g.add_edges(edges, src_field='source', dst_field='dest')
        >>> g.vertices['degree'] = 0

        Define the function to apply to each (source_node, edge, target_node)
        triple.

        >>> def degree_count_fn (src, edge, dst):
                src['degree'] += 1
                dst['degree'] += 1
                return (src, edge, dst)

        Apply the function to the SGraph.

        >>> g = g.triple_apply(degree_count_fn, mutated_fields=['degree'])


        Using native toolkit extension function:

        .. code-block:: c++

            #include <turicreate/sdk/toolkit_function_macros.hpp>
            #include <vector>

            using namespace turi;
            std::vector<variant_type> connected_components_parameterized(
              std::map<std::string, flexible_type>& src,
              std::map<std::string, flexible_type>& edge,
              std::map<std::string, flexible_type>& dst,
              std::string column) {
                if (src[column] < dst[column]) dst[column] = src[column];
                else src[column] = dst[column];
                return {to_variant(src), to_variant(edge), to_variant(dst)};
            }

            BEGIN_FUNCTION_REGISTRATION
            REGISTER_FUNCTION(connected_components_parameterized, "src", "edge", "dst", "column");
            END_FUNCTION_REGISTRATION

        compiled into example.so

        >>> from example import connected_components_parameterized as cc
        >>> e = tc.SFrame({'__src_id':[1,2,3,4,5], '__dst_id':[3,1,2,5,4]})
        >>> g = tc.SGraph().add_edges(e)
        >>> g.vertices['cid'] = g.vertices['__id']
        >>> for i in range(2):
        ...     g = g.triple_apply(lambda src, edge, dst: cc(src, edge, dst, 'cid'), ['cid'], ['cid'])
        >>> g.vertices['cid']
        dtype: int
        Rows: 5
        [4, 1, 1, 1, 4]
        '''

        assert inspect.isfunction(triple_apply_fn), "Input must be a function"
        if not (type(mutated_fields) is list or type(mutated_fields) is str):
            raise TypeError('mutated_fields must be str or list of str')
        if not (input_fields is None or type(input_fields) is list or type(input_fields) is str):
            raise TypeError('input_fields must be str or list of str')
        if type(mutated_fields) == str:
            mutated_fields = [mutated_fields]
        if len(mutated_fields) is 0:
            raise ValueError('mutated_fields cannot be empty')
        for f in ['__id', '__src_id', '__dst_id']:
            if f in mutated_fields:
                raise ValueError('mutated_fields cannot contain %s' % f)

        all_fields = self.get_fields()
        if not set(mutated_fields).issubset(set(all_fields)):
            extra_fields = list(set(mutated_fields).difference(set(all_fields)))
            raise ValueError('graph does not contain fields: %s' % str(extra_fields))

        # select input fields
        if input_fields is None:
            input_fields = self.get_fields()
        elif type(input_fields) is str:
            input_fields = [input_fields]

        # make input fields a superset of mutated_fields
        input_fields_set = set(input_fields + mutated_fields)
        input_fields = [x for x in self.get_fields() if x in input_fields_set]
        g = self.select_fields(input_fields)

        nativefn = None
        try:
            from .. import extensions
            nativefn = extensions._build_native_function_call(triple_apply_fn)
        except:
            # failure are fine. we just fall out into the next few phases
            pass
        if nativefn is not None:
            with cython_context():
                return SGraph(_proxy=g.__proxy__.lambda_triple_apply_native(nativefn, mutated_fields))
        else:
            with cython_context():
                return SGraph(_proxy=g.__proxy__.lambda_triple_apply(triple_apply_fn, mutated_fields))

    def save(self, filename, format='auto'):
        """
        Save the SGraph to disk. If the graph is saved in binary format, the
        graph can be re-loaded using the :py:func:`load_sgraph` method.
        Alternatively, the SGraph can be saved in JSON format for a
        human-readable and portable representation.

        Parameters
        ----------
        filename : string
            Filename to use when saving the file. It can be either a local or
            remote url.

        format : {'auto', 'binary', 'json'}, optional
            File format. If not specified, the format is detected automatically
            based on the filename. Note that JSON format graphs cannot be
            re-loaded with :py:func:`load_sgraph`.

        See Also
        --------
        load_sgraph

        Examples
        --------
        >>> g = turicreate.SGraph()
        >>> g = g.add_vertices([turicreate.Vertex(i) for i in range(5)])

        Save and load in binary format.

        >>> g.save('mygraph')
        >>> g2 = turicreate.load_sgraph('mygraph')

        Save in JSON format.

        >>> g.save('mygraph.json', format='json')
        """

        if format is 'auto':
            if filename.endswith(('.json', '.json.gz')):
                format = 'json'
            else:
                format = 'binary'

        if format not in ['binary', 'json', 'csv']:
            raise ValueError('Invalid format: %s. Supported formats are: %s'
                             % (format, ['binary', 'json', 'csv']))
        with cython_context():
            self.__proxy__.save_graph(_make_internal_url(filename), format)

    def get_neighborhood(self, ids, radius=1, full_subgraph=True):
        """
        Retrieve the graph neighborhood around a set of vertices, ignoring edge
        directions. Note that setting radius greater than two often results in a
        time-consuming query for a very large subgraph.

        Parameters
        ----------
        ids : list [int | float | str]
            List of target vertex IDs.

        radius : int, optional
            Radius of the neighborhood. Every vertex in the returned subgraph is
            reachable from at least one of the target vertices on a path of
            length no longer than ``radius``. Setting radius larger than 2 may
            result in a very large subgraph.

        full_subgraph : bool, optional
            If True, return all edges between vertices in the returned
            neighborhood. The result is also known as the subgraph induced by
            the target nodes' neighbors, or the egocentric network for the
            target nodes. If False, return only edges on paths of length <=
            ``radius`` from the target node, also known as the reachability
            graph.

        Returns
        -------
        out : Graph
            The subgraph with the neighborhoods around the target vertices.

        See Also
        --------
        get_edges, get_vertices

        References
        ----------
        - Marsden, P. (2002) `Egocentric and sociocentric measures of network
          centrality <http://www.sciencedirect.com/science/article/pii/S03788733
          02000163>`_.
        - `Wikipedia - Reachability <http://en.wikipedia.org/wiki/Reachability>`_

        Examples
        --------
        >>> sf_edge = turicreate.SFrame({'source': range(9), 'dest': range(1, 10)})
        >>> g = turicreate.SGraph()
        >>> g = g.add_edges(sf_edge, src_field='source', dst_field='dest')
        >>> subgraph = g.get_neighborhood(ids=[1, 7], radius=2,
                                          full_subgraph=True)
        """


        verts = ids

        ## find the vertices within radius (and the path edges)
        for i in range(radius):
            edges_out = self.get_edges(src_ids=verts)
            edges_in = self.get_edges(dst_ids=verts)

            verts = list(edges_in['__src_id']) + list(edges_in['__dst_id']) + \
                list(edges_out['__src_id']) + list(edges_out['__dst_id'])
            verts = list(set(verts))

        ## make a new graph to return and add the vertices
        g = SGraph()
        g = g.add_vertices(self.get_vertices(verts), vid_field='__id')

        ## add the requested edge set
        if full_subgraph is True:
            induced_edge_out = self.get_edges(src_ids=verts)
            induced_edge_in = self.get_edges(dst_ids=verts)
            df_induced = induced_edge_out.append(induced_edge_in)
            df_induced = df_induced.groupby(df_induced.column_names(), {})

            verts_sa = SArray(list(verts))
            edges = df_induced.filter_by(verts_sa, "__src_id")
            edges = edges.filter_by(verts_sa, "__dst_id")

        else:
            path_edges = edges_out.append(edges_in)
            edges = path_edges.groupby(path_edges.column_names(), {})

        g = g.add_edges(edges, src_field='__src_id', dst_field='__dst_id')
        return g


#/**************************************************************************/
#/*                                                                        */
#/*                            Module Function                             */
#/*                                                                        */
#/**************************************************************************/
def load_sgraph(filename, format='binary', delimiter='auto'):
    """
    Load SGraph from text file or previously saved SGraph binary.

    Parameters
    ----------
    filename : string
        Location of the file. Can be a local path or a remote URL.

    format : {'binary', 'snap', 'csv', 'tsv'}, optional
        Format to of the file to load.

        - 'binary': native graph format obtained from `SGraph.save`.
        - 'snap': tab or space separated edge list format with comments, used in
          the `Stanford Network Analysis Platform <http://snap.stanford.edu/snap/>`_.
        - 'csv': comma-separated edge list without header or comments.
        - 'tsv': tab-separated edge list without header or comments.

    delimiter : str, optional
        Specifying the Delimiter used in 'snap', 'csv' or 'tsv' format. Those
        format has default delimiter, but sometimes it is useful to
        overwrite the default delimiter.

    Returns
    -------
    out : SGraph
        Loaded SGraph.

    See Also
    --------
    SGraph, SGraph.save

    Examples
    --------
    >>> g = turicreate.SGraph().add_vertices([turicreate.Vertex(i) for i in range(5)])

    Save and load in binary format.

    >>> g.save('mygraph')
    >>> g2 = turicreate.load_sgraph('mygraph')
    """


    if not format in ['binary', 'snap', 'csv', 'tsv']:
        raise ValueError('Invalid format: %s' % format)

    with cython_context():
        g = None
        if format is 'binary':
            proxy = glconnect.get_unity().load_graph(_make_internal_url(filename))
            g = SGraph(_proxy=proxy)
        elif format is 'snap':
            if delimiter == 'auto':
                delimiter = '\t'
            sf = SFrame.read_csv(filename, comment_char='#', delimiter=delimiter,
                                 header=False, column_type_hints=int)
            g = SGraph().add_edges(sf, 'X1', 'X2')
        elif format is 'csv':
            if delimiter == 'auto':
                delimiter = ','
            sf = SFrame.read_csv(filename, header=False, delimiter=delimiter)
            g = SGraph().add_edges(sf, 'X1', 'X2')
        elif format is 'tsv':
            if delimiter == 'auto':
                delimiter = '\t'
            sf = SFrame.read_csv(filename, header=False, delimiter=delimiter)
            g = SGraph().add_edges(sf, 'X1', 'X2')
        g.summary()  # materialize
        return g


#/**************************************************************************/
#/*                                                                        */
#/*                            Helper Function                             */
#/*                                                                        */
#/**************************************************************************/
def _vertex_list_to_dataframe(ls, id_column_name):
    """
    Convert a list of vertices into dataframe.
    """
    assert HAS_PANDAS, 'Cannot use dataframe because Pandas is not available or version is too low.'
    cols = reduce(set.union, (set(v.attr.keys()) for v in ls))
    df = pd.DataFrame({id_column_name: [v.vid for v in ls]})
    for c in cols:
        df[c] = [v.attr.get(c) for v in ls]
    return df

def _vertex_list_to_sframe(ls, id_column_name):
    """
    Convert a list of vertices into an SFrame.
    """
    sf = SFrame()

    if type(ls) == list:
        cols = reduce(set.union, (set(v.attr.keys()) for v in ls))
        sf[id_column_name] = [v.vid for v in ls]
        for c in cols:
            sf[c] = [v.attr.get(c) for v in ls]

    elif type(ls) == Vertex:
        sf[id_column_name] = [ls.vid]
        for col, val in ls.attr.iteritems():
            sf[col] = [val]

    else:
        raise TypeError('Vertices type {} is Not supported.'.format(type(ls)))

    return sf

def _edge_list_to_dataframe(ls, src_column_name, dst_column_name):
    """
    Convert a list of edges into dataframe.
    """
    assert HAS_PANDAS, 'Cannot use dataframe because Pandas is not available or version is too low.'
    cols = reduce(set.union, (set(e.attr.keys()) for e in ls))
    df = pd.DataFrame({
        src_column_name: [e.src_vid for e in ls],
        dst_column_name: [e.dst_vid for e in ls]})
    for c in cols:
        df[c] = [e.attr.get(c) for e in ls]
    return df

def _edge_list_to_sframe(ls, src_column_name, dst_column_name):
    """
    Convert a list of edges into an SFrame.
    """
    sf = SFrame()

    if type(ls) == list:
        cols = reduce(set.union, (set(v.attr.keys()) for v in ls))
        sf[src_column_name] = [e.src_vid for e in ls]
        sf[dst_column_name] = [e.dst_vid for e in ls]
        for c in cols:
            sf[c] = [e.attr.get(c) for e in ls]

    elif type(ls) == Edge:
        sf[src_column_name] = [ls.src_vid]
        sf[dst_column_name] = [ls.dst_vid]

    else:
        raise TypeError('Edges type {} is Not supported.'.format(type(ls)))

    return sf

def _dataframe_to_vertex_list(df):
    """
    Convert dataframe into list of vertices, assuming that vertex ids are stored in _VID_COLUMN.
    """
    cols = df.columns
    if len(cols):
        assert _VID_COLUMN in cols, "Vertex DataFrame must contain column %s" % _VID_COLUMN
        df = df[cols].T
        ret = [Vertex(None, _series=df[col]) for col in df]
        return ret
    else:
        return []


def _dataframe_to_edge_list(df):
    """
    Convert dataframe into list of edges, assuming that source and target ids are stored in _SRC_VID_COLUMN, and _DST_VID_COLUMN respectively.
    """
    cols = df.columns
    if len(cols):
        assert _SRC_VID_COLUMN in cols, "Vertex DataFrame must contain column %s" % _SRC_VID_COLUMN
        assert _DST_VID_COLUMN in cols, "Vertex DataFrame must contain column %s" % _DST_VID_COLUMN
        df = df[cols].T
        ret = [Edge(None, None, _series=df[col]) for col in df]
        return ret
    else:
        return []


def _vertex_data_to_sframe(data, vid_field):
    """
    Convert data into a vertex data sframe. Using vid_field to identify the id
    column. The returned sframe will have id column name '__id'.
    """
    if isinstance(data, SFrame):
        # '__id' already in the sframe, and it is ok to not specify vid_field
        if vid_field is None and _VID_COLUMN in data.column_names():
            return data
        if vid_field is None:
            raise ValueError("vid_field must be specified for SFrame input")
        data_copy = copy.copy(data)
        data_copy.rename({vid_field: _VID_COLUMN}, inplace=True)
        return data_copy

    if type(data) == Vertex or type(data) == list:
        return _vertex_list_to_sframe(data, '__id')

    elif HAS_PANDAS and type(data) == pd.DataFrame:
        if vid_field is None:
            # using the dataframe index as vertex id
            if data.index.is_unique:
                if not ("index" in data.columns):
                    # pandas reset_index() will insert a new column of name "index".
                    sf = SFrame(data.reset_index())  # "index"
                    sf.rename({'index': _VID_COLUMN}, inplace=True)
                    return sf
                else:
                    # pandas reset_index() will insert a new column of name "level_0" if there exists a column named "index".
                    sf = SFrame(data.reset_index())  # "level_0"
                    sf.rename({'level_0': _VID_COLUMN}, inplace=True)
                    return sf
            else:
                raise ValueError("Index of the vertices dataframe is not unique, \
                        try specifying vid_field name to use a column for vertex ids.")
        else:
            sf = SFrame(data)
            if _VID_COLUMN in sf.column_names():
                raise ValueError('%s reserved vid column name already exists in the SFrame' % _VID_COLUMN)
            sf.rename({vid_field: _VID_COLUMN}, inplace=True)
            return sf
    else:
        raise TypeError('Vertices type %s is Not supported.' % str(type(data)))


def _edge_data_to_sframe(data, src_field, dst_field):
    """
    Convert data into an edge data sframe. Using src_field and dst_field to
    identify the source and target id column. The returned sframe will have id
    column name '__src_id', '__dst_id'
    """
    if isinstance(data, SFrame):
        # '__src_vid' and '__dst_vid' already in the sframe, and
        # it is ok to not specify src_field and dst_field
        if src_field is None and dst_field is None and \
           _SRC_VID_COLUMN in data.column_names() and \
           _DST_VID_COLUMN in data.column_names():
            return data
        if src_field is None:
            raise ValueError("src_field must be specified for SFrame input")
        if dst_field is None:
            raise ValueError("dst_field must be specified for SFrame input")
        data_copy = copy.copy(data)
        if src_field == _DST_VID_COLUMN and dst_field == _SRC_VID_COLUMN:
            # special case when src_field = "__dst_id" and dst_field = "__src_id"
            # directly renaming will cause name collision
            dst_id_column = data_copy[_DST_VID_COLUMN]
            del data_copy[_DST_VID_COLUMN]
            data_copy.rename({_SRC_VID_COLUMN: _DST_VID_COLUMN}, inplace=True)
            data_copy[_SRC_VID_COLUMN] = dst_id_column
        else:
            data_copy.rename({src_field: _SRC_VID_COLUMN, dst_field: _DST_VID_COLUMN}, inplace=True)
        return data_copy

    elif HAS_PANDAS and type(data) == pd.DataFrame:
        if src_field is None:
            raise ValueError("src_field must be specified for Pandas input")
        if dst_field is None:
            raise ValueError("dst_field must be specified for Pandas input")
        sf = SFrame(data)
        if src_field == _DST_VID_COLUMN and dst_field == _SRC_VID_COLUMN:
            # special case when src_field = "__dst_id" and dst_field = "__src_id"
            # directly renaming will cause name collision
            dst_id_column = data_copy[_DST_VID_COLUMN]
            del sf[_DST_VID_COLUMN]
            sf.rename({_SRC_VID_COLUMN: _DST_VID_COLUMN}, inplace=True)
            sf[_SRC_VID_COLUMN] = dst_id_column
        else:
            sf.rename({src_field: _SRC_VID_COLUMN, dst_field: _DST_VID_COLUMN}, inplace=True)
        return sf

    elif type(data) == Edge:
        return _edge_list_to_sframe([data], _SRC_VID_COLUMN, _DST_VID_COLUMN)

    elif type(data) == list:
        return _edge_list_to_sframe(data, _SRC_VID_COLUMN, _DST_VID_COLUMN)

    else:
        raise TypeError('Edges type %s is Not supported.' % str(type(data)))

## Hack: overriding GFrame class name to make it appears as SFrame##
GFrame.__name__ = SFrame.__name__
GFrame.__module__ = SFrame.__module__
