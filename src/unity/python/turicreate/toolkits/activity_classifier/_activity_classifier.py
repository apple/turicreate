# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Class definition and utilities for the activity classification toolkit.
"""
from __future__ import absolute_import as _
from __future__ import print_function as _
from __future__ import division as _

import numpy as _np
import time as _time
import sys as _sys
import six as _six

from turicreate import SArray as _SArray, SFrame as _SFrame
from turicreate import aggregate as _agg

import turicreate.toolkits._internal_utils as _tkutl
from turicreate.toolkits import _coreml_utils
import turicreate.toolkits._feature_engineering._internal_utils as _fe_tkutl
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits import evaluation as _evaluation
from .. import _mxnet_utils

from turicreate.toolkits._model import CustomModel as _CustomModel
from turicreate.toolkits._model import PythonProxy as _PythonProxy

from .util import random_split_by_session as _random_split_by_session


def create(dataset, session_id, target, features=None, prediction_window=100,
           validation_set='auto', max_iterations=10, batch_size=32, verbose=True):
    """
    Create an :class:`ActivityClassifier` model.

    Parameters
    ----------
    dataset : SFrame
        Input data which consists of `sessions` of data where each session is
        a sequence of data. The data must be in `stacked` format, grouped by
        session. Within each session, the data is assumed to be sorted
        temporally. Columns in `features` will be used to train a model that
        will make a prediction using labels in the `target` column.

    session_id : string
        Name of the column that contains a unique ID for each session.

    target : string
        Name of the column containing the target variable. The values in this
        column must be of string or integer type. Use `model.classes` to
        retrieve the order in which the classes are mapped.

    features : list[string], optional
        Name of the columns containing the input features that will be used
        for classification. If set to `None`, all columns except `session_id`
        and `target` will be used.

    prediction_window : int, optional
        Number of time units between predictions. For example, if your input
        data is sampled at 100Hz, and the `prediction_window` is set to 100,
        then this model will make a prediction every 1 second.

    validation_set : SFrame, optional
        A dataset for monitoring the model's generalization performance to
        prevent the model from overfitting to the training data.

        For each row of the progress table, accuracy is measured over the
        provided training dataset and the `validation_set`. The format of this
        SFrame must be the same as the training set.

        When set to 'auto', a validation set is automatically sampled from the
        training data (if the training data has > 100 sessions). If
        validation_set is set to None, then all the data will be used for
        training.

    max_iterations : int , optional
        Maximum number of iterations/epochs made over the data during the
        training phase.

    batch_size : int, optional
        Number of sequence chunks used per training step. Must be greater than
        the number of GPUs in use.

    verbose : bool, optional
        If True, print progress updates and model details.

    Returns
    -------
    out : ActivityClassifier
        A trained :class:`ActivityClassifier` model.

    Examples
    --------
    .. sourcecode:: python

        >>> import turicreate as tc

        # Training on dummy data
        >>> data = tc.SFrame({
        ...    'accelerometer_x': [0.1, 0.2, 0.3, 0.4, 0.5] * 10,
        ...    'accelerometer_y': [0.5, 0.4, 0.3, 0.2, 0.1] * 10,
        ...    'accelerometer_z': [0.01, 0.01, 0.02, 0.02, 0.01] * 10,
        ...    'session_id': [0, 0, 0] * 10 + [1, 1] * 10,
        ...    'activity': ['walk', 'run', 'run'] * 10 + ['swim', 'swim'] * 10
        ... })

        # Create an activity classifier
        >>> model = tc.activity_classifier.create(train,
        ...     session_id='session_id', target='activity',
        ...     features=['accelerometer_x', 'accelerometer_y', 'accelerometer_z'])

        # Make predictions (as probability vector, or class)
        >>> predictions = model.predict(data)
        >>> predictions = model.predict(data, output_type='probability_vector')

        # Get both predictions and classes together
        >>> predictions = model.classify(data)

        # Get topk predictions (instead of only top-1) if your labels have more
        # 2 classes
        >>> predictions = model.predict_topk(data, k = 3)

        # Evaluate the model
        >>> results = model.evaluate(data)

    See Also
    --------
    ActivityClassifier, util.random_split_by_session
    """
    _tkutl._raise_error_if_not_sframe(dataset, "dataset")
    from ._model_architecture import _net_params
    from ._model_architecture import _define_model, _fit_model
    from ._sframe_sequence_iterator import SFrameSequenceIter as _SFrameSequenceIter
    from ._sframe_sequence_iterator import prep_data as _prep_data

    if not isinstance(target, str):
        raise _ToolkitError('target must be of type str')
    if not isinstance(session_id, str):
        raise _ToolkitError('session_id must be of type str')
    _tkutl._raise_error_if_sframe_empty(dataset, 'dataset')
    _tkutl._numeric_param_check_range('prediction_window', prediction_window, 1, 400)
    _tkutl._numeric_param_check_range('max_iterations', max_iterations, 0, _six.MAXSIZE)

    if features is None:
        features = _fe_tkutl.get_column_names(dataset,
                                              interpret_as_excluded=True,
                                              column_names=[session_id, target])
    if not hasattr(features, '__iter__'):
        raise TypeError("Input 'features' must be a list.")
    if not all([isinstance(x, str) for x in features]):
        raise TypeError("Invalid feature %s: Feature names must be of type str." % x)
    if len(features) == 0:
        raise TypeError("Input 'features' must contain at least one column name.")

    start_time = _time.time()
    dataset = _tkutl._toolkits_select_columns(dataset, features + [session_id, target])
    _tkutl._raise_error_if_sarray_not_expected_dtype(dataset[target], target, [str, int])
    _tkutl._raise_error_if_sarray_not_expected_dtype(dataset[session_id], session_id, [str, int])

    # Encode the target column to numerical values
    use_target = target is not None
    dataset, target_map = _encode_target(dataset, target)

    predictions_in_chunk = 20
    chunked_data, num_sessions = _prep_data(dataset, features, session_id, prediction_window,
                                            predictions_in_chunk, target=target, verbose=verbose)

    if isinstance(validation_set, str) and validation_set == 'auto':
        if num_sessions < 100:
            validation_set = None
        else:
            dataset, validation_set = _random_split_by_session(dataset, session_id)

    # Create data iterators
    num_gpus = _mxnet_utils.get_num_gpus_in_use(max_devices=num_sessions)
    user_provided_batch_size = batch_size
    batch_size = max(batch_size, num_gpus, 1)
    data_iter = _SFrameSequenceIter(chunked_data, len(features),
                                    prediction_window, predictions_in_chunk,
                                    batch_size, use_target=use_target)

    if validation_set is not None:
        _tkutl._raise_error_if_not_sframe(validation_set, 'validation_set')
        _tkutl._raise_error_if_sframe_empty(validation_set, 'validation_set')
        validation_set = _tkutl._toolkits_select_columns(
            validation_set, features + [session_id, target])
        validation_set = validation_set.filter_by(target_map.keys(), target)
        validation_set, mapping = _encode_target(validation_set, target, target_map)
        chunked_validation_set, _ = _prep_data(validation_set, features, session_id, prediction_window,
                                            predictions_in_chunk, target=target, verbose=False)

        valid_iter = _SFrameSequenceIter(chunked_validation_set, len(features),
                                    prediction_window, predictions_in_chunk,
                                    batch_size, use_target=use_target)
    else:
        valid_iter = None

    # Define model architecture
    context = _mxnet_utils.get_mxnet_context(max_devices=num_sessions)
    loss_model, pred_model = _define_model(features, target_map, prediction_window,
                                           predictions_in_chunk, context)

    # Train the model
    log = _fit_model(loss_model, data_iter, valid_iter,
                     max_iterations, num_gpus, verbose)

    # Set up prediction model
    pred_model.bind(data_shapes=data_iter.provide_data, label_shapes=None,
                    for_training=False)
    arg_params, aux_params = loss_model.get_params()
    pred_model.init_params(arg_params=arg_params, aux_params=aux_params)

    # Save the model
    state = {
        '_pred_model': pred_model,
        'verbose': verbose,
        'training_time': _time.time() - start_time,
        'target': target,
        'classes': sorted(target_map.keys()),
        'features': features,
        'session_id': session_id,
        'prediction_window': prediction_window,
        'max_iterations': max_iterations,
        'num_examples': len(dataset),
        'num_sessions': num_sessions,
        'num_classes': len(target_map),
        'num_features': len(features),
        'training_accuracy': log['train_acc'],
        'training_log_loss': log['train_loss'],
        '_target_id_map': target_map,
        '_id_target_map': {v: k for k, v in target_map.items()},
        '_predictions_in_chunk': predictions_in_chunk,
        '_recalibrated_batch_size': data_iter.batch_size,
        'batch_size' : user_provided_batch_size
    }

    if validation_set is not None:
        state['valid_accuracy'] = log['valid_acc']
        state['valid_log_loss'] = log['valid_loss']

    model = ActivityClassifier(state)
    return model


def _encode_target(data, target, mapping=None):
    """ Encode targets to integers in [0, num_classes - 1] """
    if mapping is None:
        mapping = {t: i for i, t in enumerate(sorted(data[target].unique()))}

    data[target] = data[target].apply(lambda t: mapping[t])
    return data, mapping

class ActivityClassifier(_CustomModel):
    """
    A trained model that is ready to use for classification or export to
    CoreML.

    This model should not be constructed directly.
    """

    _PYTHON_ACTIVITY_CLASSIFIER_VERSION = 2

    def __init__(self, state):
        self.__proxy__ = _PythonProxy(state)

    def _get_native_state(self):
        state = self.__proxy__.get_state()
        state['_pred_model'] = _mxnet_utils.get_mxnet_state(state['_pred_model'])
        return state

    @classmethod
    def _load_version(cls, state, version):
        _tkutl._model_version_check(version, cls._PYTHON_ACTIVITY_CLASSIFIER_VERSION)

        data_seq_len = state['prediction_window'] * state['_predictions_in_chunk']
        data = {'data': (state['_recalibrated_batch_size'], data_seq_len, len(state['features']))}
        labels = [
            ('target', (state['_recalibrated_batch_size'], state['_predictions_in_chunk'], 1)),
            ('weights', (state['_recalibrated_batch_size'], state['_predictions_in_chunk'], 1))
        ]

        from ._model_architecture import _define_model
        import mxnet as _mx
        context = _mxnet_utils.get_mxnet_context(max_devices=state['num_sessions'])
        _, _pred_model = _define_model(state['features'], state['_target_id_map'], 
                                       state['prediction_window'],
                                       state['_predictions_in_chunk'], context)

        batch_size = state['batch_size']
        preds_in_chunk = state['_predictions_in_chunk']
        win = state['prediction_window'] * preds_in_chunk
        num_features = len(state['features'])
        data_shapes = [('data', (batch_size, win, num_features))]
        target_shape= (batch_size, preds_in_chunk, 1)

        _pred_model.bind(data_shapes=data_shapes, label_shapes=None,
                         for_training=False)
        arg_params = _mxnet_utils.params_from_dict(state['_pred_model']['arg_params'])
        aux_params = _mxnet_utils.params_from_dict(state['_pred_model']['aux_params'])
        _pred_model.init_params(arg_params=arg_params, aux_params=aux_params)
        state['_pred_model'] = _pred_model

        return ActivityClassifier(state)

    @classmethod
    def _native_name(cls):
        return "activity_classifier"

    def _get_version(self):
        return self._PYTHON_ACTIVITY_CLASSIFIER_VERSION

    def export_coreml(self, filename):
        """
        Save the model in Core ML format.

        See Also
        --------
        save

        Examples
        --------
        >>> model.export_coreml('myModel.mlmodel')
        """
        import coremltools as _cmt
        import mxnet as _mx
        from ._model_architecture import _define_model, _fit_model, _net_params

        prob_name = self.target + 'Probability'
        label_name = self.target

        input_features = [
            ('features', _cmt.models.datatypes.Array(*(1, self.prediction_window, self.num_features)))
        ]
        output_features = [
            (prob_name, _cmt.models.datatypes.Array(*(self.num_classes,)))
        ]

        model_params = self._pred_model.get_params()
        weights = {k: v.asnumpy() for k, v in model_params[0].items()}
        weights = _mx.rnn.LSTMCell(num_hidden=_net_params['lstm_h']).unpack_weights(weights)
        moving_weights = {k: v.asnumpy() for k, v in model_params[1].items()}

        builder = _cmt.models.neural_network.NeuralNetworkBuilder(
            input_features,
            output_features,
            mode='classifier'
        )

        # Conv
        # (1,1,W,C) -> (1,C,1,W)
        builder.add_permute(name='permute_layer', dim=(0, 3, 1, 2),
                            input_name='features', output_name='conv_in')
        W = _np.expand_dims(weights['conv_weight'], axis=0).transpose((2, 3, 1, 0))
        builder.add_convolution(name='conv_layer',
                                kernel_channels=self.num_features,
                                output_channels=_net_params['conv_h'],
                                height=1, width=self.prediction_window,
                                stride_height=1, stride_width=self.prediction_window,
                                border_mode='valid', groups=1,
                                W=W, b=weights['conv_bias'], has_bias=True,
                                input_name='conv_in', output_name='relu0_in')
        builder.add_activation(name='relu_layer0', non_linearity='RELU',
                               input_name='relu0_in', output_name='lstm_in')

        # LSTM
        builder.add_optionals([('lstm_h_in', _net_params['lstm_h']),
                               ('lstm_c_in', _net_params['lstm_h'])],
                              [('lstm_h_out', _net_params['lstm_h']),
                               ('lstm_c_out', _net_params['lstm_h'])])

        W_x = [weights['lstm_i2h_i_weight'], weights['lstm_i2h_f_weight'],
               weights['lstm_i2h_o_weight'], weights['lstm_i2h_c_weight']]
        W_h = [weights['lstm_h2h_i_weight'], weights['lstm_h2h_f_weight'],
               weights['lstm_h2h_o_weight'], weights['lstm_h2h_c_weight']]
        bias = [weights['lstm_h2h_i_bias'], weights['lstm_h2h_f_bias'],
                weights['lstm_h2h_o_bias'], weights['lstm_h2h_c_bias']]

        builder.add_unilstm(name='lstm_layer',
                            W_h=W_h, W_x=W_x, b=bias,
                            input_size=_net_params['conv_h'],
                            hidden_size=_net_params['lstm_h'],
                            input_names=['lstm_in', 'lstm_h_in', 'lstm_c_in'],
                            output_names=['dense0_in', 'lstm_h_out', 'lstm_c_out'],
                            inner_activation='SIGMOID')

        # Dense
        builder.add_inner_product(name='dense_layer',
                                  W=weights['dense0_weight'], b=weights['dense0_bias'],
                                  input_channels=_net_params['lstm_h'],
                                  output_channels=_net_params['dense_h'],
                                  has_bias=True,
                                  input_name='dense0_in',
                                  output_name='bn_in')

        builder.add_batchnorm(name='bn_layer',
                              channels=_net_params['dense_h'],
                              gamma=weights['bn_gamma'], beta=weights['bn_beta'],
                              mean=moving_weights['bn_moving_mean'],
                              variance=moving_weights['bn_moving_var'],
                              input_name='bn_in', output_name='relu1_in',
                              epsilon=0.001)
        builder.add_activation(name='relu_layer1', non_linearity='RELU',
                               input_name='relu1_in', output_name='dense1_in')

        # Softmax
        builder.add_inner_product(name='dense_layer1',
                                  W=weights['dense1_weight'], b=weights['dense1_bias'],
                                  has_bias=True,
                                  input_channels=_net_params['dense_h'],
                                  output_channels=self.num_classes,
                                  input_name='dense1_in', output_name='softmax_in')

        builder.add_softmax(name=prob_name,
                            input_name='softmax_in',
                            output_name=prob_name)


        labels = list(map(str, sorted(self._target_id_map.keys())))
        builder.set_class_labels(labels)
        mlmodel = _cmt.models.MLModel(builder.spec)
        model_type = 'activity classifier'
        mlmodel.short_description = _coreml_utils._mlmodel_short_description(model_type)
        # Add useful information to the mlmodel
        features_str = ', '.join(self.features)
        mlmodel.input_description['features'] = u'Window \xd7 [%s]' % features_str
        mlmodel.input_description['lstm_h_in'] = 'LSTM hidden state input'
        mlmodel.input_description['lstm_c_in'] = 'LSTM cell state input'
        mlmodel.output_description[prob_name] = 'Activity prediction probabilities'
        mlmodel.output_description['classLabel'] = 'Class label of top prediction'
        mlmodel.output_description['lstm_h_out'] = 'LSTM hidden state output'
        mlmodel.output_description['lstm_c_out'] = 'LSTM cell state output'
        _coreml_utils._set_model_metadata(mlmodel, self.__class__.__name__, {
                'prediction_window': str(self.prediction_window),
                'session_id': self.session_id,
                'target': self.target,
                'features': ','.join(self.features),
                'max_iterations': str(self.max_iterations),
            }, version=ActivityClassifier._PYTHON_ACTIVITY_CLASSIFIER_VERSION)
        spec = mlmodel.get_spec()
        _cmt.models.utils.rename_feature(spec, 'classLabel', label_name)
        _cmt.models.utils.rename_feature(spec, 'lstm_h_in', 'hiddenIn')
        _cmt.models.utils.rename_feature(spec, 'lstm_c_in', 'cellIn')
        _cmt.models.utils.rename_feature(spec, 'lstm_h_out', 'hiddenOut')
        _cmt.models.utils.rename_feature(spec, 'lstm_c_out', 'cellOut')
        _cmt.utils.save_spec(spec, filename)

    def predict(self, dataset, output_type='class', output_frequency='per_row'):
        """
        Return predictions for ``dataset``, using the trained activity classifier.
        Predictions can be generated as class labels, or as a probability
        vector with probabilities for each class.

        The activity classifier generates a single prediction for each
        ``prediction_window`` rows in ``dataset``, per ``session_id``. Thus the
        number of predictions is smaller than the length of ``dataset``. By
        default each prediction is replicated by ``prediction_window`` to return
        a prediction for each row of ``dataset``. Use ``output_frequency`` to
        get the unreplicated predictions.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        output_type : {'class', 'probability_vector'}, optional
            Form of each prediction which is one of:

            - 'probability_vector': Prediction probability associated with each
              class as a vector. The probability of the first class (sorted
              alphanumerically by name of the class in the training set) is in
              position 0 of the vector, the second in position 1 and so on.
            - 'class': Class prediction. This returns the class with maximum
              probability.

        output_frequency : {'per_row', 'per_window'}, optional
            The frequency of the predictions which is one of:

            - 'per_window': Return a single prediction for each
              ``prediction_window`` rows in ``dataset`` per ``session_id``.
            - 'per_row': Convenience option to make sure the number of
              predictions match the number of rows in the dataset. Each
              prediction from the model is repeated ``prediction_window``
              times during that window.

        Returns
        -------
        out : SArray | SFrame
            If ``output_frequency`` is 'per_row' return an SArray with predictions
            for each row in ``dataset``.
            If ``output_frequency`` is 'per_window' return an SFrame with
            predictions for ``prediction_window`` rows in ``dataset``.

        See Also
        ----------
        create, evaluate, classify

        Examples
        --------

        .. sourcecode:: python

            # One prediction per row
            >>> probability_predictions = model.predict(
            ...     data, output_type='probability_vector', output_frequency='per_row')[:4]
            >>> probability_predictions

            dtype: array
            Rows: 4
            [array('d', [0.01857384294271469, 0.0348394550383091, 0.026018327102065086]),
             array('d', [0.01857384294271469, 0.0348394550383091, 0.026018327102065086]),
             array('d', [0.01857384294271469, 0.0348394550383091, 0.026018327102065086]),
             array('d', [0.01857384294271469, 0.0348394550383091, 0.026018327102065086])]

            # One prediction per window
            >>> class_predictions = model.predict(
            ...     data, output_type='class', output_frequency='per_window')
            >>> class_predictions

            +---------------+------------+-----+
            | prediction_id | session_id |class|
            +---------------+------------+-----+
            |       0       |     3      |  5  |
            |       1       |     3      |  5  |
            |       2       |     3      |  5  |
            |       3       |     3      |  5  |
            |       4       |     3      |  5  |
            |       5       |     3      |  5  |
            |       6       |     3      |  5  |
            |       7       |     3      |  4  |
            |       8       |     3      |  4  |
            |       9       |     3      |  4  |
            |      ...      |    ...     | ... |
            +---------------+------------+-----+
        """
        _tkutl._raise_error_if_not_sframe(dataset, 'dataset')
        _tkutl._check_categorical_option_type(
            'output_frequency', output_frequency, ['per_window', 'per_row'])
        _tkutl._check_categorical_option_type(
            'output_type', output_type, ['probability_vector', 'class'])
        from ._sframe_sequence_iterator import SFrameSequenceIter as _SFrameSequenceIter
        from ._sframe_sequence_iterator import prep_data as _prep_data

        from ._sframe_sequence_iterator import _ceil_dev

        prediction_window = self.prediction_window
        chunked_dataset, _ = _prep_data(dataset, self.features, self.session_id, prediction_window,
                                     self._predictions_in_chunk, verbose=False)
        data_iter = _SFrameSequenceIter(chunked_dataset, len(self.features),
                                        prediction_window, self._predictions_in_chunk,
                                        self._recalibrated_batch_size, use_pad=True)

        chunked_data = data_iter.dataset
        preds = self._pred_model.predict(data_iter).asnumpy()

        if output_frequency == 'per_row':
            # Replicate each prediction times prediction_window
            preds = preds.repeat(prediction_window, axis=1)

            # Remove predictions for padded rows
            unpadded_len = chunked_data['chunk_len'].to_numpy()
            preds = [p[:unpadded_len[i]] for i, p in enumerate(preds)]

            # Reshape from (num_of_chunks, chunk_size, num_of_classes)
            # to (ceil(length / prediction_window), num_of_classes)
            # chunk_size is DIFFERENT between chunks - since padding was removed.
            out = _np.concatenate(preds)
            out = out.reshape((-1, len(self._target_id_map)))
            out = _SArray(out)

            if output_type == 'class':
                id_target_map = self._id_target_map
                out = out.apply(lambda c: id_target_map[_np.argmax(c)])

        elif output_frequency == 'per_window':
            # Calculate the number of expected predictions and
            # remove predictions for padded data
            unpadded_len = chunked_data['chunk_len'].apply(
                lambda l: _ceil_dev(l, prediction_window)).to_numpy()
            preds = [list(p[:unpadded_len[i]]) for i, p in enumerate(preds)]

            out = _SFrame({
                self.session_id: chunked_data['session_id'],
                'preds': _SArray(preds, dtype=list)
            }).stack('preds', new_column_name='probability_vector')

            # Calculate the prediction index per session
            out = out.add_row_number(column_name='prediction_id')
            start_sess_idx = out.groupby(
                self.session_id, {'start_idx': _agg.MIN('prediction_id')})
            start_sess_idx = start_sess_idx.unstack(
                [self.session_id, 'start_idx'], new_column_name='idx')['idx'][0]

            if output_type == 'class':
                id_target_map = self._id_target_map
                out['probability_vector'] = out['probability_vector'].apply(
                    lambda c: id_target_map[_np.argmax(c)])
                out = out.rename({'probability_vector': 'class'})

        return out

    def evaluate(self, dataset, metric='auto'):
        """
        Evaluate the model by making predictions of target values and comparing
        these to actual values.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the session_id, target and features used for model training.
            Additional columns are ignored.

        metric : str, optional
            Name of the evaluation metric.  Possible values are:

            - 'auto'             : Returns all available metrics.
            - 'accuracy'         : Classification accuracy (micro average).
            - 'auc'              : Area under the ROC curve (macro average)
            - 'precision'        : Precision score (macro average)
            - 'recall'           : Recall score (macro average)
            - 'f1_score'         : F1 score (macro average)
            - 'log_loss'         : Log loss
            - 'confusion_matrix' : An SFrame with counts of possible
                                   prediction/true label combinations.
            - 'roc_curve'        : An SFrame containing information needed for an
                                   ROC curve

        Returns
        -------
        out : dict
            Dictionary of evaluation results where the key is the name of the
            evaluation metric (e.g. `accuracy`) and the value is the evaluation
            score.

        See Also
        ----------
        create, predict

        Examples
        ----------
        .. sourcecode:: python

          >>> results = model.evaluate(data)
          >>> print results['accuracy']
        """

        avail_metrics = ['accuracy', 'auc', 'precision', 'recall',
                         'f1_score', 'log_loss', 'confusion_matrix', 'roc_curve']
        _tkutl._check_categorical_option_type(
            'metric', metric, avail_metrics + ['auto'])

        if metric == 'auto':
            metrics = avail_metrics
        else:
            metrics = [metric]

        probs = self.predict(dataset, output_type='probability_vector')
        classes = self.predict(dataset, output_type='class')

        ret = {}
        if 'accuracy' in metrics:
            ret['accuracy'] = _evaluation.accuracy(dataset[self.target], classes)
        if 'auc' in metrics:
            ret['auc'] = _evaluation.auc(dataset[self.target], probs)
        if 'precision' in metrics:
            ret['precision'] = _evaluation.precision(dataset[self.target], classes)
        if 'recall' in metrics:
            ret['recall'] = _evaluation.recall(dataset[self.target], classes)
        if 'f1_score' in metrics:
            ret['f1_score'] = _evaluation.f1_score(dataset[self.target], classes)
        if 'log_loss' in metrics:
            ret['log_loss'] = _evaluation.log_loss(dataset[self.target], probs)
        if 'confusion_matrix' in metrics:
            ret['confusion_matrix'] = _evaluation.confusion_matrix(dataset[self.target], classes)
        if 'roc_curve' in metrics:
            ret['roc_curve'] = _evaluation.roc_curve(dataset[self.target], probs)

        return ret

    def classify(self, dataset, output_frequency='per_row'):
        """
        Return a classification, for each ``prediction_window`` examples in the
        ``dataset``, using the trained activity classification model. The output
        SFrame contains predictions as both class labels as well as probabilities 
        that the predicted value is the associated label.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the features and session id used for model training, but
            does not require a target column. Additional columns are ignored.

        output_frequency : {'per_row', 'per_window'}, optional
            The frequency of the predictions which is one of:

            - 'per_row': Each prediction is returned ``prediction_window`` times.
            - 'per_window': Return a single prediction for each 
              ``prediction_window`` rows in ``dataset`` per ``session_id``.

        Returns
        -------
        out : SFrame
            An SFrame with model predictions i.e class labels and probabilities.

        See Also
        ----------
        create, evaluate, predict

        Examples
        ----------
        >>> classes = model.classify(data)
        """
        _tkutl._check_categorical_option_type(
            'output_frequency', output_frequency, ['per_window', 'per_row'])
        id_target_map = self._id_target_map
        preds = self.predict(
            dataset, output_type='probability_vector', output_frequency=output_frequency)

        if output_frequency == 'per_row':
            return _SFrame({
                'class': preds.apply(lambda p: id_target_map[_np.argmax(p)]),
                'probability': preds.apply(_np.max)
            })
        elif output_frequency == 'per_window':
            preds['class'] = preds['probability_vector'].apply(
                lambda p: id_target_map[_np.argmax(p)])
            preds['probability'] = preds['probability_vector'].apply(_np.max)
            preds = preds.remove_column('probability_vector')
            return preds

    def predict_topk(self, dataset, output_type='probability', k=3, output_frequency='per_row'):
        """
        Return top-k predictions for the ``dataset``, using the trained model.
        Predictions are returned as an SFrame with three columns: `prediction_id`, 
        `class`, and `probability`, or `rank`, depending on the ``output_type``
        parameter.

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the features and session id used for model training, but
            does not require a target column. Additional columns are ignored.

        output_type : {'probability', 'rank'}, optional
            Choose the return type of the prediction:

            - `probability`: Probability associated with each label in the prediction.
            - `rank`       : Rank associated with each label in the prediction.

        k : int, optional
            Number of classes to return for each input example.

        output_frequency : {'per_row', 'per_window'}, optional
            The frequency of the predictions which is one of:

            - 'per_row': Each prediction is returned ``prediction_window`` times.
            - 'per_window': Return a single prediction for each 
              ``prediction_window`` rows in ``dataset`` per ``session_id``.

        Returns
        -------
        out : SFrame
            An SFrame with model predictions.

        See Also
        --------
        predict, classify, evaluate

        Examples
        --------
        >>> pred = m.predict_topk(validation_data, k=3)
        >>> pred
        +---------------+-------+-------------------+
        |     row_id    | class |    probability    |
        +---------------+-------+-------------------+
        |       0       |   4   |   0.995623886585  |
        |       0       |   9   |  0.0038311756216  |
        |       0       |   7   | 0.000301006948575 |
        |       1       |   1   |   0.928708016872  |
        |       1       |   3   |  0.0440889261663  |
        |       1       |   2   |  0.0176190119237  |
        |       2       |   3   |   0.996967732906  |
        |       2       |   2   |  0.00151345680933 |
        |       2       |   7   | 0.000637513934635 |
        |       3       |   1   |   0.998070061207  |
        |      ...      |  ...  |        ...        |
        +---------------+-------+-------------------+
        """
        _tkutl._check_categorical_option_type('output_type', output_type, ['probability', 'rank'])
        id_target_map = self._id_target_map
        preds = self.predict(
            dataset, output_type='probability_vector', output_frequency=output_frequency)

        if output_frequency == 'per_row':
            probs = preds
        elif output_frequency == 'per_window':
            probs = preds['probability_vector']

        if output_type == 'rank':
            probs = probs.apply(lambda p: [
                {'class': id_target_map[i],
                 'rank': i}
                for i in reversed(_np.argsort(p)[-k:])]
            )
        elif output_type == 'probability':
            probs = probs.apply(lambda p: [
                {'class': id_target_map[i],
                 'probability': p[i]}
                for i in reversed(_np.argsort(p)[-k:])]
            )

        if output_frequency == 'per_row':
            output = _SFrame({'probs': probs})
            output = output.add_row_number(column_name='row_id')
        elif output_frequency == 'per_window':
            output = _SFrame({
                'probs': probs,
                self.session_id: preds[self.session_id],
                'prediction_id': preds['prediction_id']
            })

        output = output.stack('probs', new_column_name='probs')
        output = output.unpack('probs', column_name_prefix='')
        return output

    def __str__(self):
        """
        Return a string description of the model to the ``print`` method.

        Returns
        -------
        out : string
            A description of the ActivityClassifier.
        """
        return self.__repr__()

    def __repr__(self):
        """
        Print a string description of the model when the model name is entered
        in the terminal.
        """
        width = 40
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
            ('Number of sessions', 'num_sessions'),
            ('Number of classes', 'num_classes'),
            ('Number of feature columns', 'num_features'),
            ('Prediction window', 'prediction_window'),
        ]
        training_fields = [
            ('Log-likelihood', 'training_log_loss'),
            ('Training time (sec)', 'training_time'),
        ]

        section_titles = ['Schema', 'Training summary']
        return([model_fields, training_fields], section_titles)
