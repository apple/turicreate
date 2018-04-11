# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Class definition and utilities for the image similarity toolkit.
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import copy as _copy
import time as _time
import sys as _sys

import turicreate as _tc
from turicreate.toolkits._model import CustomModel as _CustomModel
import turicreate.toolkits._internal_utils as _tkutl
import turicreate.toolkits._private_utils as _pvtutl
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._model import ExposeAttributesFromProxy as _ExposeAttributesFromProxy
from turicreate.toolkits._model import PythonProxy as _PythonProxy
from .. import _pre_trained_models
from .. import _image_feature_extractor
from turicreate.toolkits._internal_utils import (_raise_error_if_not_sframe,
                                                 _numeric_param_check_range)

def create(dataset, label = None, feature = None, model = 'resnet-50', verbose = True):
    """
    Create a :class:`ImageSimilarityModel` model.

    Parameters
    ----------
    dataset : SFrame
        Input data. The column named by the 'feature' parameter will be
        extracted for modeling.

    label : string
        Name of the SFrame column with row labels to be used as uuid's to
        identify the data. If 'label' is set to None, row numbers are used to
        identify reference dataset rows when the model is queried.

    feature : string
        indicates that the SFrame has only column of Image type and that will
        Name of the column containing the input images. 'None' (the default)
        be used for similarity.

    model: string, optional
        Uses a pretrained model to bootstrap an image similarity model

           - "resnet-50" : Uses a pretrained resnet model.

        Models are downloaded from the internet if not available locally. Once
        downloaded, the models are cached for future use.

    verbose : bool, optional
        If True, print progress updates and model details.

    Returns
    -------
    out : ImageSimilarityModel
        A trained :class:`ImageSimilarityModel` model.

    See Also
    --------
    ImageSimilarityModel

    Examples
    --------
    .. sourcecode:: python

        # Train an image similarity model
        >>> model = turicreate.image_similarity.create(data)

        # Query the model for similar images
        >>> similar_images = model.query(data)
        +-------------+-----------------+-------------------+------+
        | query_label | reference_label |      distance     | rank |
        +-------------+-----------------+-------------------+------+
        |      0      |        0        |        0.0        |  1   |
        |      0      |       519       |   12.5319706301   |  2   |
        |      0      |       1619      |   12.5563764596   |  3   |
        |      0      |       186       |   12.6132604915   |  4   |
        |      0      |       1809      |   12.9180964745   |  5   |
        |      1      |        1        | 2.02304872852e-06 |  1   |
        |      1      |       1579      |   11.4288186151   |  2   |
        |      1      |       1237      |   12.3764325949   |  3   |
        |      1      |        80       |   12.7264363676   |  4   |
        |      1      |        58       |   12.7675058558   |  5   |
        +-------------+-----------------+-------------------+------+
        [500 rows x 4 columns]
    """
    start_time = _time.time()

    # Check parameters
    _tkutl._check_categorical_option_type('model', model, _pre_trained_models.MODELS.keys())
    if len(dataset) == 0:
        raise _ToolkitError('Unable to train on empty dataset')
    if (label is not None) and (label not in dataset.column_names()):
        raise _ToolkitError("Row label column '%s' does not exist" % label)
    if (feature is not None) and (feature not in dataset.column_names()):
        raise _ToolkitError("Image feature column '%s' does not exist" % feature)

    # Set defaults
    if feature is None:
        feature = _tkutl._find_only_image_column(dataset)

    # Load pre-trained model & feature extractor
    ptModel = _pre_trained_models.MODELS[model]()
    feature_extractor = _image_feature_extractor.MXFeatureExtractor(ptModel)

    # Extract features
    extracted_features = _tc.SFrame({
        '__image_features__': feature_extractor.extract_features(dataset, feature, verbose=verbose),
        })

    # Train a similarity model using the extracted features
    if label is not None:
        extracted_features[label] = dataset[label]
    nn_model = _tc.nearest_neighbors.create(extracted_features, label = label,
            features = ['__image_features__'], verbose = verbose)

    # Save the model
    state = {
        'similarity_model': nn_model,
        'model': model,
        'feature_extractor': feature_extractor,
        'input_image_shape': ptModel.input_image_shape,
        'label': label,
        'feature': feature,
        'num_features': 1,
        'num_examples': nn_model.num_examples,
        'training_time': _time.time() - start_time,
    }
    return ImageSimilarityModel(state)


class ImageSimilarityModel(_CustomModel):
    """
    An trained model that is ready to use for similarity. This model should not
    be constructed directly.
    """

    _PYTHON_IMAGE_SIMILARITY_VERSION = 1

    def __init__(self, state):
        self.__proxy__ = _PythonProxy(state)

    @classmethod
    def _native_name(cls):
        return "image_similarity"

    def _get_version(self):
        return self._PYTHON_IMAGE_SIMILARITY_VERSION

    def _get_native_state(self):
        """
        Save the model as a directory, which can be loaded with the
        :py:func:`~turicreate.load_model` method.

        Parameters
        ----------
        pickler : GLPickler
            An opened GLPickle archive (Do not close the archive).

        See Also
        --------
        turicreate.load_model

        Examples
        --------
        >>> model.save('my_model_file')
        >>> loaded_model = turicreate.load_model('my_model_file')
        """
        state = self.__proxy__.get_state()
        state['similarity_model'] = state['similarity_model'].__proxy__
        del state['feature_extractor']
        return state

    @classmethod
    def _load_version(cls, state, version):
        """
        A function to load a previously saved ImageClassifier
        instance.

        Parameters
        ----------
        unpickler : GLUnpickler
            A GLUnpickler file handler.

        version : int
            Version number maintained by the class writer.
        """
        _tkutl._model_version_check(version, cls._PYTHON_IMAGE_SIMILARITY_VERSION)
        from turicreate.toolkits.nearest_neighbors import NearestNeighborsModel
        state['similarity_model'] = NearestNeighborsModel(state['similarity_model'])
        # Load pre-trained model & feature extractor
        ptModel = _pre_trained_models.MODELS[state['model']]()
        feature_extractor = _image_feature_extractor.MXFeatureExtractor(ptModel)
        state['feature_extractor'] = feature_extractor
        state['input_image_shape'] = tuple([int(i) for i in state['input_image_shape']])
        return ImageSimilarityModel(state)

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the ImageSimilarityModel.
        """
        return self.__repr__()

    def __repr__(self):
        """
        Print a string description of the model when the model name is entered
        in the terminal.
        """

        width = 40
        key_str = "{:<{}}: {}"

        sections, section_titles = self._get_summary_struct()
        out = _tkutl._toolkit_repr_print(self, sections, section_titles,
                                         width=width)
        return out

    def _get_summary_struct(self):
        """
        Returns a structured description of the model, including (where
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
            ('Input image shape', 'input_image_shape'),
        ]
        training_fields = [
            ("Training time (sec)", 'training_time'),
        ]

        section_titles = ['Schema', 'Training summary']
        return([model_fields, training_fields], section_titles)

    def _extract_features(self, dataset):
        return _tc.SFrame({
            '__image_features__': self.feature_extractor.extract_features(dataset, self.feature)
            })

    def query(self, dataset, label=None, k=5, radius=None, verbose=True):
        """
        For each image, retrieve the nearest neighbors from the model's stored
        data. In general, the query dataset does not need to be the same as
        the reference data stored in the model.

        Parameters
        ----------
        dataset : SFrame | SArray | turicreate.Image
            Query data.
            If dataset is an SFrame, it must contain columns with the same
            names and types as the features used to train the model.
            Additional columns are ignored.

        label : str, optional
            Name of the query SFrame column with row labels. If 'label' is not
            specified, row numbers are used to identify query dataset rows in
            the output SFrame.

        k : int, optional
            Number of nearest neighbors to return from the reference set for
            each query observation. The default is 5 neighbors, but setting it
            to ``None`` will return all neighbors within ``radius`` of the
            query point.

        radius : float, optional
            Only neighbors whose distance to a query point is smaller than this
            value are returned. The default is ``None``, in which case the
            ``k`` nearest neighbors are returned for each query point,
            regardless of distance.

        verbose: bool, optional
            If True, print progress updates and model details.

        Returns
        -------
        out : SFrame
            An SFrame with the k-nearest neighbors of each query observation.
            The result contains four columns: the first is the label of the
            query observation, the second is the label of the nearby reference
            observation, the third is the distance between the query and
            reference observations, and the fourth is the rank of the reference
            observation among the query's k-nearest neighbors.

        See Also
        --------
        similarity_graph

        Notes
        -----
        - If both ``k`` and ``radius`` are set to ``None``, each query point
          returns all of the reference set. If the reference dataset has
          :math:`n` rows and the query dataset has :math:`m` rows, the output
          is an SFrame with :math:`nm` rows.

        Examples
        --------
        >>> model.query(queries, 'label', k=2)
        +-------------+-----------------+----------------+------+
        | query_label | reference_label |    distance    | rank |
        +-------------+-----------------+----------------+------+
        |      0      |        2        | 0.305941170816 |  1   |
        |      0      |        1        | 0.771556867638 |  2   |
        |      1      |        1        | 0.390128184063 |  1   |
        |      1      |        0        | 0.464004310325 |  2   |
        |      2      |        0        | 0.170293863659 |  1   |
        |      2      |        1        | 0.464004310325 |  2   |
        +-------------+-----------------+----------------+------+
        """
        if not isinstance(dataset, (_tc.SFrame, _tc.SArray, _tc.Image)):
            raise TypeError('dataset must be either an SFrame, SArray or turicreate.Image')

        if isinstance(dataset, _tc.SArray):
            dataset = _tc.SFrame({self.feature: dataset})
        elif isinstance(dataset, _tc.Image):
            dataset = _tc.SFrame({self.feature: [dataset]})

        extracted_features = self._extract_features(dataset)
        if label is not None:
            extracted_features[label] = dataset[label]
        return self.similarity_model.query(extracted_features, label, k, radius, verbose)

    def similarity_graph(self, k=5, radius=None, include_self_edges=False,
                         output_type='SGraph', verbose=True):
        """
        Construct the similarity graph on the reference dataset, which is
        already stored in the model to find the top `k` similar images for each
        image in your input dataset.

        This is conceptually very similar to running `query` with the reference
        set, but this method is optimized for the purpose, syntactically
        simpler, and automatically removes self-edges.

        WARNING: This method can take time.

        Parameters
        ----------
        k : int, optional
            Maximum number of neighbors to return for each point in the
            dataset. Setting this to ``None`` deactivates the constraint, so
            that all neighbors are returned within ``radius`` of a given point.

        radius : float, optional
            For a given point, only neighbors within this distance are
            returned. The default is ``None``, in which case the ``k`` nearest
            neighbors are returned for each query point, regardless of
            distance.

        include_self_edges : bool, optional
            For most distance functions, each point in the model's reference
            dataset is its own nearest neighbor. If this parameter is set to
            False, this result is ignored, and the nearest neighbors are
            returned *excluding* the point itself.

        output_type : {'SGraph', 'SFrame'}, optional
            By default, the results are returned in the form of an SGraph,
            where each point in the reference dataset is a vertex and an edge A
            -> B indicates that vertex B is a nearest neighbor of vertex A. If
            'output_type' is set to 'SFrame', the output is in the same form as
            the results of the 'query' method: an SFrame with columns
            indicating the query label (in this case the query data is the same
            as the reference data), reference label, distance between the two
            points, and the rank of the neighbor.

        verbose : bool, optional
            If True, print progress updates and model details.

        Returns
        -------
        out : SFrame or SGraph
            The type of the output object depends on the 'output_type'
            parameter. See the parameter description for more detail.

        Notes
        -----
        - If both ``k`` and ``radius`` are set to ``None``, each data point is
          matched to the entire dataset. If the reference dataset has
          :math:`n` rows, the output is an SFrame with :math:`n^2` rows (or an
          SGraph with :math:`n^2` edges).

        Examples
        --------
        Unlike the ``query`` method, there is no need for a second dataset with
        ``similarity_graph``.

        >>> graph = model.similarity_graph(k=1)  # an SGraph

        # Most similar image for each image in the input dataset
        >>> graph.edges
        +----------+----------+----------------+------+
        | __src_id | __dst_id |    distance    | rank |
        +----------+----------+----------------+------+
        |    0     |    1     | 0.376430604494 |  1   |
        |    2     |    1     | 0.55542776308  |  1   |
        |    1     |    0     | 0.376430604494 |  1   |
        +----------+----------+----------------+------+
        """
        return self.similarity_model.similarity_graph(k, radius, include_self_edges, output_type, verbose)

    def export_coreml(self, filename):
        """
        Save the model in Core ML format.
        The exported model calculates the distance between a query image and
        each row of the model's stored data. It does not sort and retrieve
        the k nearest neighbors of the query image.

        See Also
        --------
        save

        Examples
        --------
        # Train an image similarity model
        >>> model = turicreate.image_similarity.create(data)

        # Query the model for similar images
        >>> similar_images = model.query(data)
        +-------------+-----------------+---------------+------+
        | query_label | reference_label |    distance   | rank |
        +-------------+-----------------+---------------+------+
        |      0      |        0        |      0.0      |  1   |
        |      0      |        2        | 24.9664942809 |  2   |
        |      0      |        1        | 28.4416069428 |  3   |
        |      1      |        1        |      0.0      |  1   |
        |      1      |        2        | 21.8715131191 |  2   |
        |      1      |        0        | 28.4416069428 |  3   |
        |      2      |        2        |      0.0      |  1   |
        |      2      |        1        | 21.8715131191 |  2   |
        |      2      |        0        | 24.9664942809 |  3   |
        +-------------+-----------------+---------------+------+
        [9 rows x 4 columns]

        # Export the model to Core ML format
        >>> model.export_coreml('myModel.mlmodel')

        # Load the Core ML model
        >>> import coremltools
        >>> ml_model = coremltools.models.MLModel('myModel.mlmodel')

        # Prepare the first image of reference data for consumption
        # by the Core ML model
        >>> import PIL
        >>> image = tc.image_analysis.resize(
                data['image'][0], *reversed(model.input_image_shape))
        >>> image = PIL.Image.fromarray(image.pixel_data)

        # Calculate distances using the Core ML model
        >>> ml_model.predict(data={'image': image})
        {'distance': array([ 0.      , 28.453125, 24.96875 ])}
        """
        import numpy as _np
        import coremltools as _cmt
        from coremltools.models import datatypes as _datatypes, neural_network as _neural_network
        from .._mxnet_to_coreml import _mxnet_converter
        from turicreate.toolkits import _coreml_utils

        # Get the reference data from the model
        proxy = self.similarity_model.__proxy__
        reference_data = _np.array(_tc.extensions._nearest_neighbors._nn_get_reference_data(proxy))
        num_examples, embedding_size = reference_data.shape

        # Get the input and output names
        input_name = self.feature_extractor.data_layer
        output_name = 'distance'
        input_features = [(input_name, _datatypes.Array(*(self.input_image_shape)))]
        output_features = [(output_name, _datatypes.Array(num_examples))]

        # Create a neural network
        builder = _neural_network.NeuralNetworkBuilder(
            input_features, output_features, mode=None)

        # Convert the feature extraction network
        mx_feature_extractor = self.feature_extractor._get_mx_module(
            self.feature_extractor.ptModel.mxmodel,
            self.feature_extractor.data_layer,
            self.feature_extractor.feature_layer,
            self.feature_extractor.context,
            self.input_image_shape
        )
        batch_input_shape = (1, ) + self.input_image_shape
        _mxnet_converter.convert(mx_feature_extractor, mode=None,
                                 input_shape={input_name: batch_input_shape},
                                 builder=builder, verbose=False)

        # To add the nearest neighbors model we add calculation of the euclidean 
        # distance between the newly extracted query features (denoted by the vector u)
        # and each extracted reference feature (denoted by the rows of matrix V).
        # Calculation of sqrt((v_i-u)^2) = sqrt(v_i^2 - 2v_i*u + u^2) ensues.
        V = reference_data
        v_squared = (V * V).sum(axis=1)

        feature_layer = self.feature_extractor.feature_layer
        builder.add_inner_product('v^2-2vu', W=-2 * V, b=v_squared, has_bias=True,
                                  input_channels=embedding_size, output_channels=num_examples,
                                  input_name=feature_layer, output_name='v^2-2vu')

        builder.add_unary('element_wise-u^2', mode='power', alpha=2,
                          input_name=feature_layer, output_name='element_wise-u^2')

        # Produce a vector of length num_examples with all values equal to u^2
        builder.add_inner_product('u^2', W=_np.ones((embedding_size, num_examples)),
                                  b=None, has_bias=False,
                                  input_channels=embedding_size, output_channels=num_examples,
                                  input_name='element_wise-u^2', output_name='u^2')

        builder.add_elementwise('v^2-2vu+u^2', mode='ADD',
                                input_names=['v^2-2vu', 'u^2'],
                                output_name='v^2-2vu+u^2')

        # v^2-2vu+u^2=(v-u)^2 is non-negative but some computations on GPU may result in
        # small negative values. Apply RELU so we don't take the square root of negative values.
        builder.add_activation('relu', non_linearity='RELU',
                               input_name='v^2-2vu+u^2', output_name='relu')
        builder.add_unary('sqrt', mode='sqrt', input_name='relu', output_name=output_name)

        # Finalize model
        _mxnet_converter._set_input_output_layers(builder, [input_name], [output_name])
        builder.set_input([input_name], [self.input_image_shape])
        builder.set_output([output_name], [(num_examples,)])
        _cmt.models.utils.rename_feature(builder.spec, input_name, self.feature)
        builder.set_pre_processing_parameters(image_input_names=self.feature)

        # Add metadata
        mlmodel = _cmt.models.MLModel(builder.spec)
        model_type = 'image similarity'
        mlmodel.short_description = _coreml_utils._mlmodel_short_description(model_type)
        mlmodel.input_description[self.feature] = u'Input image'
        mlmodel.output_description[output_name] = u'Distances between the input and reference images'
        mlmodel.save(filename)
