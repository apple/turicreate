# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Class definition and create method for DBSCAN clustering.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import time as _time
import logging as _logging
import turicreate as _tc
import turicreate.aggregate as _agg
from turicreate.toolkits._model import CustomModel as _CustomModel
import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits._internal_utils import _toolkit_repr_print
from turicreate.toolkits._private_utils import _summarize_accessible_fields
from turicreate.toolkits._model import PythonProxy as _PythonProxy


def create(dataset, features=None, distance=None, radius=1.,
           min_core_neighbors=10, verbose=True):
    """
    Create a DBSCAN clustering model. The DBSCAN method partitions the input
    dataset into three types of points, based on the estimated probability
    density at each point.

    - **Core** points have a large number of points within a given neighborhood.
      Specifically, `min_core_neighbors` must be within distance `radius` of a
      point for it to be considered a core point.

    - **Boundary** points are within distance `radius` of a core point, but
      don't have sufficient neighbors of their own to be considered core.

    - **Noise** points comprise the remainder of the data. These points have too
      few neighbors to be considered core points, and are further than distance
      `radius` from all core points.

    Clusters are formed by connecting core points that are neighbors of each
    other, then assigning boundary points to their nearest core neighbor's
    cluster.

    Parameters
    ----------
    dataset : SFrame
        Training data, with each row corresponding to an observation. Must
        include all features specified in the `features` parameter, but may have
        additional columns as well.

    features : list[str], optional
        Name of the columns with features to use in comparing records. 'None'
        (the default) indicates that all columns of the input `dataset` should
        be used to train the model. All features must be numeric, i.e. integer
        or float types.

    distance : str or list[list], optional
        Function to measure the distance between any two input data rows. This
        may be one of two types:

        - *String*: the name of a standard distance function. One of
          'euclidean', 'squared_euclidean', 'manhattan', 'levenshtein',
          'jaccard', 'weighted_jaccard', 'cosine', 'dot_product' (deprecated),
          or 'transformed_dot_product'.

        - *Composite distance*: the weighted sum of several standard distance
          functions applied to various features. This is specified as a list of
          distance components, each of which is itself a list containing three
          items:

          1. list or tuple of feature names (str)

          2. standard distance name (str)

          3. scaling factor (int or float)

        For more information about Turi Create distance functions, please
        see the :py:mod:`~turicreate.toolkits.distances` module.

        For sparse vectors, missing keys are assumed to have value 0.0.

        If 'distance' is left unspecified, a composite distance is constructed
        automatically based on feature types.

    radius : int or float, optional
        Size of each point's neighborhood, with respect to the specified
        distance function.

    min_core_neighbors : int, optional
        Number of neighbors that must be within distance `radius` of a point in
        order for that point to be considered a "core point" of a cluster.

    verbose : bool, optional
        If True, print progress updates and model details during model creation.

    Returns
    -------
    out : DBSCANModel
        A model containing a cluster label for each row in the input `dataset`.
        Also contains the indices of the core points, cluster boundary points,
        and noise points.

    See Also
    --------
    DBSCANModel, turicreate.toolkits.distances

    Notes
    -----
    - Our implementation of DBSCAN first computes the similarity graph on the
      input dataset, which can be a computationally intensive process. In the
      current implementation, some distances are substantially faster than
      others; in particular "euclidean", "squared_euclidean", "cosine", and
      "transformed_dot_product" are quite fast, while composite distances can be
      slow.

    - Any distance function in the GL Create library may be used with DBSCAN but
      the results may be poor for distances that violate the standard metric
      properties, i.e. symmetry, non-negativity, triangle inequality, and
      identity of indiscernibles. In particular, the DBSCAN algorithm is based
      on the concept of connecting high-density points that are *close* to each
      other into a single cluster, but the notion of *close* may be very
      counterintuitive if the chosen distance function is not a valid metric.
      The distances "euclidean", "manhattan", "jaccard", and "levenshtein" will
      likely yield the best results.

    References
    ----------
    - Ester, M., et al. (1996) `A Density-Based Algorithm for Discovering
      Clusters in Large Spatial Databases with Noise
      <https://www.aaai.org/Papers/KDD/1996/KDD96-037.pdf>`_. In Proceedings of the
      Second International Conference on Knowledge Discovery and Data Mining.
      pp. 226-231.

    - `Wikipedia - DBSCAN <https://en.wikipedia.org/wiki/DBSCAN>`_

    - `Visualizing DBSCAN Clustering
      <http://www.naftaliharris.com/blog/visualizing-dbscan-clustering/>`_

    Examples
    --------
    >>> sf = turicreate.SFrame({
    ...     'x1': [0.6777, -9.391, 7.0385, 2.2657, 7.7864, -10.16, -8.162,
    ...            8.8817, -9.525, -9.153, 2.0860, 7.6619, 6.5511, 2.7020],
    ...     'x2': [5.6110, 8.5139, 5.3913, 5.4743, 8.3606, 7.8843, 2.7305,
    ...            5.1679, 6.7231, 3.7051, 1.7682, 7.4608, 3.1270, 6.5624]})
    ...
    >>> model = turicreate.dbscan.create(sf, radius=4.25, min_core_neighbors=3)
    >>> model.cluster_id.print_rows(15)
    +--------+------------+----------+
    | row_id | cluster_id |   type   |
    +--------+------------+----------+
    |   8    |     0      |   core   |
    |   7    |     2      |   core   |
    |   0    |     1      |   core   |
    |   2    |     2      |   core   |
    |   3    |     1      |   core   |
    |   11   |     2      |   core   |
    |   4    |     2      |   core   |
    |   1    |     0      | boundary |
    |   6    |     0      | boundary |
    |   5    |     0      | boundary |
    |   9    |     0      | boundary |
    |   12   |     2      | boundary |
    |   10   |     1      | boundary |
    |   13   |     1      | boundary |
    +--------+------------+----------+
    [14 rows x 3 columns]
    """

    ## Start the training time clock and instantiate an empty model
    logger = _logging.getLogger(__name__)
    start_time = _time.time()


    ## Validate the input dataset
    _tkutl._raise_error_if_not_sframe(dataset, "dataset")
    _tkutl._raise_error_if_sframe_empty(dataset, "dataset")


    ## Validate neighborhood parameters
    if not isinstance(min_core_neighbors, int) or min_core_neighbors < 0:
        raise ValueError("Input 'min_core_neighbors' must be a non-negative " +
                         "integer.")

    if not isinstance(radius, (int, float)) or radius < 0:
        raise ValueError("Input 'radius' must be a non-negative integer " +
                         "or float.")


    ## Compute all-point nearest neighbors within `radius` and count
    #  neighborhood sizes
    knn_model = _tc.nearest_neighbors.create(dataset, features=features,
                                             distance=distance,
                                             method='brute_force',
                                             verbose=verbose)

    knn = knn_model.similarity_graph(k=None, radius=radius,
                                     include_self_edges=False,
                                     output_type='SFrame',
                                     verbose=verbose)

    neighbor_counts = knn.groupby('query_label', _agg.COUNT)


    ### NOTE: points with NO neighbors are already dropped here!

    ## Identify core points and boundary candidate points. Not all of the
    #  boundary candidates will be boundary points - some are in small isolated
    #  clusters.
    if verbose:
        logger.info("Identifying noise points and core points.")

    boundary_mask = neighbor_counts['Count'] < min_core_neighbors
    core_mask = 1 - boundary_mask

    # this includes too small clusters
    boundary_idx = neighbor_counts[boundary_mask]['query_label']
    core_idx = neighbor_counts[core_mask]['query_label']


    ## Build a similarity graph on the core points
    ## NOTE: careful with singleton core points - the second filter removes them
    #  from the edge set so they have to be added separately as vertices.
    if verbose:
        logger.info("Constructing the core point similarity graph.")

    core_vertices = knn.filter_by(core_idx, 'query_label')
    core_edges = core_vertices.filter_by(core_idx, 'reference_label')

    core_graph = _tc.SGraph()
    core_graph = core_graph.add_vertices(core_vertices[['query_label']],
                                         vid_field='query_label')
    core_graph = core_graph.add_edges(core_edges, src_field='query_label',
                                      dst_field='reference_label')


    ## Compute core point connected components and relabel to be consecutive
    #  integers
    cc = _tc.connected_components.create(core_graph, verbose=verbose)
    cc_labels = cc.component_size.add_row_number('__label')
    core_assignments = cc.component_id.join(cc_labels, on='component_id',
                                               how='left')[['__id', '__label']]
    core_assignments['type'] = 'core'


    ## Join potential boundary points to core cluster labels (points that aren't
    #  really on a boundary are implicitly dropped)
    if verbose:
        logger.info("Processing boundary points.")

    boundary_edges = knn.filter_by(boundary_idx, 'query_label')

    # separate real boundary points from points in small isolated clusters
    boundary_core_edges = boundary_edges.filter_by(core_idx, 'reference_label')

    # join a boundary point to its single closest core point.
    boundary_assignments = boundary_core_edges.groupby('query_label',
                    {'reference_label': _agg.ARGMIN('rank', 'reference_label')})

    boundary_assignments = boundary_assignments.join(core_assignments,
                                                 on={'reference_label': '__id'})

    boundary_assignments = boundary_assignments.rename({'query_label': '__id'}, inplace=True)
    boundary_assignments = boundary_assignments.remove_column('reference_label', inplace=True)
    boundary_assignments['type'] = 'boundary'


    ## Identify boundary candidates that turned out to be in small clusters but
    #  not on real cluster boundaries
    small_cluster_idx = set(boundary_idx).difference(
                                                   boundary_assignments['__id'])


    ## Identify individual noise points by the fact that they have no neighbors.
    noise_idx = set(range(dataset.num_rows())).difference(
                                                 neighbor_counts['query_label'])

    noise_idx = noise_idx.union(small_cluster_idx)

    noise_assignments = _tc.SFrame({'row_id': _tc.SArray(list(noise_idx), int)})
    noise_assignments['cluster_id'] = None
    noise_assignments['cluster_id'] = noise_assignments['cluster_id'].astype(int)
    noise_assignments['type'] = 'noise'


    ## Append core, boundary, and noise results to each other.
    master_assignments = _tc.SFrame()
    num_clusters = 0

    if core_assignments.num_rows() > 0:
        core_assignments = core_assignments.rename({'__id': 'row_id',
                                                    '__label': 'cluster_id'}, inplace=True)
        master_assignments = master_assignments.append(core_assignments)
        num_clusters = len(core_assignments['cluster_id'].unique())

    if boundary_assignments.num_rows() > 0:
        boundary_assignments = boundary_assignments.rename({'__id': 'row_id',
                                                       '__label': 'cluster_id'}, inplace=True)
        master_assignments = master_assignments.append(boundary_assignments)

    if noise_assignments.num_rows() > 0:
        master_assignments = master_assignments.append(noise_assignments)


    ## Post-processing and formatting
    state = {'verbose': verbose,
             'radius': radius,
             'min_core_neighbors': min_core_neighbors,
             'distance': knn_model.distance,
             'num_distance_components': knn_model.num_distance_components,
             'num_examples': dataset.num_rows(),
             'features': knn_model.features,
             'num_features': knn_model.num_features,
             'unpacked_features': knn_model.unpacked_features,
             'num_unpacked_features': knn_model.num_unpacked_features,
             'cluster_id': master_assignments,
             'num_clusters': num_clusters,
             'training_time': _time.time() - start_time}

    return DBSCANModel(state)


class DBSCANModel(_CustomModel):
    """
    DBSCAN clustering model. The DBSCAN model contains the results of DBSCAN
    clustering, which finds clusters by identifying "core points" of high
    probability density and building clusters around them.

    This model should not be constructed directly. Instead, use
    :func:`turicreate.clustering.dbscan.create` to create an instance of this
    model.
    """
    _PYTHON_DBSCAN_MODEL_VERSION = 1

    def __init__(self, state):
        self.__proxy__ = _PythonProxy(state)

    @classmethod
    def _native_name(cls):
        return "dbscan"

    def _get_version(self):
        return self._PYTHON_DBSCAN_MODEL_VERSION

    def _get_native_state(self):
        return self.__proxy__.state

    @classmethod
    def _load_version(self, state, version):
        """
        A function to load a previously created DBSCANModel instance.

        Parameters
        ----------
        unpickler : GLUnpickler
            A GLUnpickler file handler.

        version : int
            Version number maintained by the class writer.
        """
        state = _PythonProxy(state)
        return DBSCANModel(state)

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : str
            A description of the DBSCANModel.
        """
        return self.__repr__()

    def __repr__(self):
        """
        Print a string description of the model when the model name is entered
        in the terminal.
        """
        width = 40

        sections, section_titles = self._get_summary_struct()
        accessible_fields = {
            "cluster_id": "Cluster label for each row in the input dataset."}

        out = _toolkit_repr_print(self, sections, section_titles, width=width)
        out2 = _summarize_accessible_fields(accessible_fields, width=width)
        return out + "\n" + out2

    def _get_summary_struct(self):
        """
        Return a structured description of the model. This includes (where
        relevant) the schema of the training data, description of the training
        data, training statistics, and model hyperparameters.

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
        model_fields = [
            ('Number of examples', 'num_examples'),
            ('Number of feature columns', 'num_features'),
            ('Max distance to a neighbor (radius)', 'radius'),
            ('Min number of neighbors for core points', 'min_core_neighbors'),
            ('Number of distance components', 'num_distance_components')]

        training_fields = [
            ('Total training time (seconds)', 'training_time'),
            ('Number of clusters', 'num_clusters')]

        section_titles = ["Schema", "Training summary"]
        return([model_fields, training_fields], section_titles)
