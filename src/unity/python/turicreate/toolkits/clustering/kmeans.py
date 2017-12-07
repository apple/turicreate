# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
This module contains the K-Means clustering algorithm, including the KmeansModel
class which provides methods for inspecting the returned cluster information.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import logging as _logging
from array import array as _array
import turicreate as _tc
from turicreate.toolkits._model import Model as _Model
from turicreate.data_structures.sframe import SFrame as _SFrame
import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits._private_utils import _robust_column_name
from turicreate.toolkits._private_utils import _validate_row_label
from turicreate.toolkits._private_utils import _summarize_accessible_fields
from turicreate.toolkits._main import ToolkitError as _ToolkitError

def _validate_dataset(dataset):
    """
    Validate the main Kmeans dataset.

    Parameters
    ----------
    dataset: SFrame
        Input dataset.
    """
    if not (isinstance(dataset, _SFrame)):
        raise TypeError("Input 'dataset' must be an SFrame.")

    if dataset.num_rows() == 0 or dataset.num_columns() == 0:
        raise ValueError("Input 'dataset' has no data.")


def _validate_initial_centers(initial_centers):
    """
    Validate the initial centers.

    Parameters
    ----------
    initial_centers : SFrame
        Initial cluster center locations, in SFrame form.
    """
    if not (isinstance(initial_centers, _SFrame)):
        raise TypeError("Input 'initial_centers' must be an SFrame.")

    if initial_centers.num_rows() == 0 or initial_centers.num_columns() == 0:
        raise ValueError("An 'initial_centers' argument is provided " +
                         "but has no data.")


def _validate_num_clusters(num_clusters, initial_centers, num_rows):
    """
    Validate the combination of the `num_clusters` and `initial_centers`
    parameters in the Kmeans model create function. If the combination is
    valid, determine and return the correct number of clusters.

    Parameters
    ----------
    num_clusters : int
        Specified number of clusters.

    initial_centers : SFrame
        Specified initial cluster center locations, in SFrame form. If the
        number of rows in this SFrame does not match `num_clusters`, there is a
        problem.

    num_rows : int
        Number of rows in the input dataset.

    Returns
    -------
    _num_clusters : int
        The correct number of clusters to use going forward
    """

    ## Basic validation
    if num_clusters is not None and not isinstance(num_clusters, int):
        raise _ToolkitError("Parameter 'num_clusters' must be an integer.")

    ## Determine the correct number of clusters.
    if initial_centers is None:
        if num_clusters is None:
            raise ValueError("Number of clusters cannot be determined from " +
                             "'num_clusters' or 'initial_centers'. You must " +
                             "specify one of these arguments.")
        else:
            _num_clusters = num_clusters

    else:
        num_centers = initial_centers.num_rows()

        if num_clusters is None:
            _num_clusters = num_centers
        else:
            if num_clusters != num_centers:
                raise ValueError("The value of 'num_clusters' does not match " +
                                 "the number of provided initial centers. " +
                                 "Please provide only one of these arguments " +
                                 "or ensure the values match.")
            else:
                _num_clusters = num_clusters

    if _num_clusters > num_rows:
        raise ValueError("The desired number of clusters exceeds the number " +
                         "of data points. Please set 'num_clusters' to be " +
                         "smaller than the number of data points.")

    return _num_clusters


def _validate_features(features, column_type_map, valid_types, label):
    """
    Identify the subset of desired `features` that are valid for the Kmeans
    model. A warning is emitted for each feature that is excluded.

    Parameters
    ----------
    features : list[str]
        Desired feature names.

    column_type_map : dict[str, type]
        Dictionary mapping each column name to the type of values in the
        column.

    valid_types : list[type]
        Exclude features whose type is not in this list.

    label : str
        Name of the row label column.

    Returns
    -------
    valid_features : list[str]
        Names of features to include in the model.
    """
    # logger = _logging.getLogger(__name__)

    if not isinstance(features, list):
        raise TypeError("Input 'features' must be a list, if specified.")

    if len(features) == 0:
        raise ValueError("If specified, input 'features' must contain " +
                         "at least one column name.")

    ## Remove duplicates
    num_original_features = len(features)
    features = set(features)

    if len(features) < num_original_features:
        _logging.warning("Duplicates have been removed from the list of features")

    ## Remove the row label
    if label in features:
        features.remove(label)
        _logging.warning("The row label has been removed from the list of features.")

    ## Check the type of each feature against the list of valid types
    valid_features = []

    for ftr in features:
        if not isinstance(ftr, str):
            _logging.warning("Feature '{}' excluded. ".format(ftr) +
                           "Features must be specified as strings " +
                           "corresponding to column names in the input dataset.")

        elif ftr not in column_type_map.keys():
            _logging.warning("Feature '{}' excluded because ".format(ftr) +
                           "it is not in the input dataset.")

        elif column_type_map[ftr] not in valid_types:
            _logging.warning("Feature '{}' excluded because of its type. ".format(ftr) +
                           "Kmeans features must be int, float, dict, or array.array type.")

        else:
            valid_features.append(ftr)

    if len(valid_features) == 0:
        raise _ToolkitError("All specified features have been excluded. " +
                            "Please specify valid features.")

    return valid_features


class KmeansModel(_Model):
    """
    K-means clustering model. A k-means model object contains the results of
    running kmeans clustering on a dataset.  Queryable fields include a cluster
    id for each vertex, as well as the centers of the clusters.

    This model cannot be constructed directly.  Instead, use
    :func:`turicreate.kmeans.create` to create an instance of this model. A
    detailed list of parameter options and code samples are available in the
    documentation for the create function.
    """
    def __init__(self, model):
        '''__init__(self)'''
        self.__proxy__ = model
        self.__name__ = self.__class__._native_name()

    @classmethod
    def _native_name(cls):
        return "kmeans"

    def predict(self, dataset, output_type='cluster_id', verbose=True):
        """
        Return predicted cluster label for instances in the new 'dataset'.
        K-means predictions are made by assigning each new instance to the
        closest cluster center.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include the features used for
            model training; additional columns are ignored.

        output_type : {'cluster_id', 'distance'}, optional
            Form of the prediction. 'cluster_id' (the default) returns the
            cluster label assigned to each input instance, while 'distance'
            returns the Euclidean distance between the instance and its
            assigned cluster's center.

        verbose : bool, optional
            If True, print progress updates to the screen.

        Returns
        -------
        out : SArray
            Model predictions. Depending on the specified `output_type`, either
            the assigned cluster label or the distance of each point to its
            closest cluster center. The order of the predictions is the same as
            order of the input data rows.

        See Also
        --------
        create

        Examples
        --------
        >>> sf = turicreate.SFrame({
        ...     'x1': [0.6777, -9.391, 7.0385, 2.2657, 7.7864, -10.16, -8.162,
        ...            8.8817, -9.525, -9.153, 2.0860, 7.6619, 6.5511, 2.7020],
        ...     'x2': [5.6110, 8.5139, 5.3913, 5.4743, 8.3606, 7.8843, 2.7305,
        ...            5.1679, 6.7231, 3.7051, 1.7682, 7.4608, 3.1270, 6.5624]})
        ...
        >>> model = turicreate.kmeans.create(sf, num_clusters=3)
        ...
        >>> sf_new = turicreate.SFrame({'x1': [-5.6584, -1.0167, -9.6181],
        ...                           'x2': [-6.3803, -3.7937, -1.1022]})
        >>> clusters = model.predict(sf_new, output_type='cluster_id')
        >>> print clusters
        [1, 0, 1]
        """

        ## Validate the input dataset.
        _tkutl._raise_error_if_not_sframe(dataset, "dataset")
        _tkutl._raise_error_if_sframe_empty(dataset, "dataset")

        ## Validate the output type.
        if not isinstance(output_type, str):
            raise TypeError("The 'output_type' parameter must be a string.")

        if not output_type in ('cluster_id', 'distance'):
            raise ValueError("The 'output_type' parameter must be either " +
                             "'cluster_label' or 'distance'.")

        ## Get model features.
        ref_features = self.features
        sf_features = _tkutl._toolkits_select_columns(dataset, ref_features)

        ## Compute predictions.
        opts = {'model': self.__proxy__,
                'model_name': self.__name__,
                'dataset': sf_features}

        result = _tc.toolkits._main.run('kmeans_predict', opts, verbose)
        sf_result = _tc.SFrame(None, _proxy=result['predictions'])

        if output_type == 'distance':
            return sf_result['distance']
        else:
            return sf_result['cluster_id']

    def _get(self, field):
        """
        Return the value of a given field.

        +-----------------------+----------------------------------------------+
        |      Field            | Description                                  |
        +=======================+==============================================+
        | batch_size            | Number of randomly chosen examples to use in |
        |                       | each training iteration.                     |
        +-----------------------+----------------------------------------------+
        | cluster_id            | Cluster assignment for each data point and   |
        |                       | Euclidean distance to the cluster center     |
        +-----------------------+----------------------------------------------+
        | cluster_info          | Cluster centers, sum of squared Euclidean    |
        |                       | distances from each cluster member to the    |
        |                       | assigned center, and the number of data      |
        |                       | points belonging to the cluster              |
        +-----------------------+----------------------------------------------+
        | features              | Names of feature columns                     |
        +-----------------------+----------------------------------------------+
        | max_iterations        | Maximum number of iterations to perform      |
        +-----------------------+----------------------------------------------+
        | method                | Algorithm used to train the model.           |
        +-----------------------+----------------------------------------------+
        | num_clusters          | Number of clusters                           |
        +-----------------------+----------------------------------------------+
        | num_examples          | Number of examples in the dataset            |
        +-----------------------+----------------------------------------------+
        | num_features          | Number of feature columns used               |
        +-----------------------+----------------------------------------------+
        | num_unpacked_features | Number of features unpacked from the         |
        |                       | feature columns                              |
        +-----------------------+----------------------------------------------+
        | training_iterations   | Total number of iterations performed         |
        +-----------------------+----------------------------------------------+
        | training_time         | Total time taken to cluster the data         |
        +-----------------------+----------------------------------------------+
        | unpacked_features     | Names of features unpacked from the          |
        |                       | feature columns                              |
        +-----------------------+----------------------------------------------+

        Parameters
        ----------
        field : str
            The name of the field to query.

        Returns
        -------
        out
            Value of the requested field
        """
        opts = {'model': self.__proxy__,
                'model_name': self.__name__,
                'field': field}
        response = _tc.toolkits._main.run('kmeans_get_value', opts)

        # cluster_id and cluster_info both return a unity SFrame. Cast to an SFrame.
        if field == 'cluster_id' or field == 'cluster_info':
            return _SFrame(None, _proxy=response['value'])
        else:
            return response['value']

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the KMeansModel.
        """
        return self.__repr__()

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
            ('Number of clusters', 'num_clusters'),
            ('Number of examples', 'num_examples'),
            ('Number of feature columns', 'num_features'),
            ('Number of unpacked features', 'num_unpacked_features'),
            ('Row label name', 'row_label_name')]

        training_fields = [
            ('Training method', 'method'),
            ('Number of training iterations', 'training_iterations'),
            ('Batch size'   , 'batch_size'),
            ('Total training time (seconds)', 'training_time')]

        section_titles = [ 'Schema', 'Training Summary']

        return ([model_fields, training_fields], section_titles)

    def __repr__(self):
        """
        Print a string description of the model when the model name is entered
        in the terminal.
        """

        width = 32
        key_str = "{:<{}}: {}"

        (sections, section_titles) = self._get_summary_struct()
        accessible_fields = {
            "cluster_id": "An SFrame containing the cluster assignments.",
            "cluster_info": "An SFrame containing the cluster centers."}

        out = _tkutl._toolkit_repr_print(self, sections, section_titles,
                                         width=width)
        out2 = _summarize_accessible_fields(accessible_fields, width=width)
        return out + "\n" + out2

def create(dataset, num_clusters=None, features=None, label=None,
           initial_centers=None, max_iterations=10, batch_size=None,
           verbose=True):
    """
    Create a k-means clustering model. The KmeansModel object contains the
    computed cluster centers and the cluster assignment for each instance in
    the input 'dataset'.

    Given a number of clusters, k-means iteratively chooses the best cluster
    centers and assigns nearby points to the best cluster. If no points change
    cluster membership between iterations, the algorithm terminates.

    Parameters
    ----------
    dataset : SFrame
        Each row in the SFrame is an observation.

    num_clusters : int
        Number of clusters. This is the 'k' in k-means.

    features : list[str], optional
        Names of feature columns to use in computing distances between
        observations and cluster centers. 'None' (the default) indicates that
        all columns should be used as features. Columns may be of the following
        types:

        - *Numeric*: values of numeric type integer or float.

        - *Array*: list of numeric (int or float) values. Each list element
          is treated as a distinct feature in the model.

        - *Dict*: dictionary of keys mapped to numeric values. Each unique key
          is treated as a distinct feature in the model.

        Note that columns of type *list* are not supported. Convert them to
        array columns if all entries in the list are of numeric types.

    label : str, optional
        Name of the column to use as row labels in the Kmeans output. The
        values in this column must be integers or strings. If not specified,
        row numbers are used by default.

    initial_centers : SFrame, optional
        Initial centers to use when starting the K-means algorithm. If
        specified, this parameter overrides the *num_clusters* parameter. The
        'initial_centers' SFrame must contain the same features used in the
        input 'dataset'.

        If not specified (the default), initial centers are chosen
        intelligently with the K-means++ algorithm.

    max_iterations : int, optional
        The maximum number of iterations to run. Prints a warning if the
        algorithm does not converge after max_iterations iterations. If set to
        0, the model returns clusters defined by the initial centers and
        assignments to those centers.

    batch_size : int, optional
        Number of randomly-chosen data points to use in each iteration. If
        'None' (the default) or greater than the number of rows in 'dataset',
        then this parameter is ignored: all rows of `dataset` are used in each
        iteration and model training terminates once point assignments stop
        changing or `max_iterations` is reached.

    verbose : bool, optional
        If True, print model training progress to the screen.

    Returns
    -------
    out : KmeansModel
        A Model object containing a cluster id for each vertex, and the centers
        of the clusters.

    See Also
    --------
    KmeansModel

    Notes
    -----
    - Integer features in the 'dataset' or 'initial_centers' inputs are
      converted internally to float type, and the corresponding features in the
      output centers are float-typed.

    - It can be important for the K-means model to standardize the features so
      they have the same scale. This function does *not* standardize
      automatically.

    References
    ----------
    - `Wikipedia - k-means clustering
      <http://en.wikipedia.org/wiki/K-means_clustering>`_

    - Artuhur, D. and Vassilvitskii, S. (2007) `k-means++: The Advantages of
      Careful Seeding <http://ilpubs.stanford.edu:8090/778/1/2006-13.pdf>`_. In
      Proceedings of the Eighteenth Annual ACM-SIAM Symposium on Discrete
      Algorithms. pp. 1027-1035.

    - Elkan, C. (2003) `Using the triangle inequality to accelerate k-means
      <http://www.aaai.org/Papers/ICML/2003/ICML03-022.pdf>`_. In Proceedings
      of the Twentieth International Conference on Machine Learning, Volume 3,
      pp. 147-153.

    - Sculley, D. (2010) `Web Scale K-Means Clustering
      <http://www.eecs.tufts.edu/~dsculley/papers/fastkmeans.pdf>`_. In
      Proceedings of the 19th International Conference on World Wide Web. pp.
      1177-1178

    Examples
    --------
    >>> sf = turicreate.SFrame({
    ...     'x1': [0.6777, -9.391, 7.0385, 2.2657, 7.7864, -10.16, -8.162,
    ...            8.8817, -9.525, -9.153, 2.0860, 7.6619, 6.5511, 2.7020],
    ...     'x2': [5.6110, 8.5139, 5.3913, 5.4743, 8.3606, 7.8843, 2.7305,
    ...            5.1679, 6.7231, 3.7051, 1.7682, 7.4608, 3.1270, 6.5624]})
    ...
    >>> model = turicreate.kmeans.create(sf, num_clusters=3)
    """

    logger = _logging.getLogger(__name__)

    opts = {'model_name': 'kmeans',
            'max_iterations': max_iterations,
            }

    ## Validate the input dataset and initial centers.
    _validate_dataset(dataset)

    if initial_centers is not None:
        _validate_initial_centers(initial_centers)

    ## Validate and determine the correct number of clusters.
    opts['num_clusters'] = _validate_num_clusters(num_clusters,
                                                  initial_centers,
                                                  dataset.num_rows())

    ## Validate the row label
    col_type_map = {c: dataset[c].dtype for c in dataset.column_names()}

    if label is not None:
        _validate_row_label(label, col_type_map)

        if label in ['cluster_id', 'distance']:
            raise ValueError("Row label column name cannot be 'cluster_id' " +
                             "or 'distance'; these are reserved for other " +
                             "columns in the Kmeans model's output.")

        opts['row_labels'] = dataset[label]
        opts['row_label_name'] = label

    else:
        opts['row_labels'] = _tc.SArray.from_sequence(dataset.num_rows())
        opts['row_label_name'] = 'row_id'


    ## Validate the features relative to the input dataset.
    if features is None:
        features = dataset.column_names()

    valid_features = _validate_features(features, col_type_map,
                                        valid_types=[_array, dict, int, float],
                                        label=label)

    sf_features = dataset.select_columns(valid_features)
    opts['features'] = sf_features

    ## Validate the features in the initial centers (if provided)
    if initial_centers is not None:
        try:
            initial_centers = initial_centers.select_columns(valid_features)
        except:
            raise ValueError("Specified features cannot be extracted from " +
                             "the provided initial centers.")

        if initial_centers.column_types() != sf_features.column_types():
            raise TypeError("Feature types are different in the dataset and " +
                            "initial centers.")

    else:
        initial_centers = _tc.SFrame()

    opts['initial_centers'] = initial_centers

    ## Validate the batch size and determine the training method.
    if batch_size is None:
        opts['method'] = 'elkan'
        opts['batch_size'] = dataset.num_rows()

    else:
        opts['method'] = 'minibatch'
        opts['batch_size'] = batch_size

    ## Create and return the model
    params = _tc.toolkits._main.run('kmeans_train', opts, verbose)
    return KmeansModel(params['model'])
