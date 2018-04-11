# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import turicreate as tc
from turicreate.toolkits._internal_utils import _raise_error_if_not_sframe
import turicreate.toolkits._main as _toolkits_main
from turicreate.toolkits._internal_utils import _map_unity_proxy_to_object
from turicreate.toolkits._supervised_learning import select_default_missing_value_policy

class TreeModelMixin(object):
    """
    Implements common methods among tree models:
    - BoostedTreesClassifier
    - BoostedTreesRegression
    - RandomForestClassifier
    - RandomForestRegression
    - DecisionTreeClassifier
    - DecisionTreeRegression
    """

    def get_feature_importance(self):
        """
        Get the importance of features used by the model.

        The measure of importance of feature X
        is determined by the sum of occurrence of X
        as a branching node in all trees.

        When X is a categorical feature, e.g. "Gender",
        the index column contains the value of the feature, e.g. "M" or "F".
        When X is a numerical feature, index of X is None.

        Returns
        -------
        out : SFrame
            A table with three columns: name, index, count,
            ordered by 'count' in descending order.

        Examples
        --------
        >>> m.get_feature_importance()
        Rows: 31
        Data:
            +-----------------------------+-------+-------+
            |             name            | index | count |
            +-----------------------------+-------+-------+
            | DER_mass_transverse_met_lep |  None |   66  |
            |         DER_mass_vis        |  None |   65  |
            |          PRI_tau_pt         |  None |   61  |
            |         DER_mass_MMC        |  None |   59  |
            |      DER_deltar_tau_lep     |  None |   58  |
            |          DER_pt_tot         |  None |   41  |
            |           PRI_met           |  None |   38  |
            |     PRI_jet_leading_eta     |  None |   30  |
            |     DER_deltaeta_jet_jet    |  None |   27  |
            |       DER_mass_jet_jet      |  None |   24  |
            +-----------------------------+-------+-------+
            [31 rows x 3 columns]
        """
        metric_name = '.'.join([self.__module__, 'get_feature_importance'])
        return tc.extensions._xgboost_feature_importance(self.__proxy__)

    def extract_features(self, dataset, missing_value_action='auto'):
        """
        For each example in the dataset, extract the leaf indices of
        each tree as features.

        For multiclass classification, each leaf index contains #num_class
        numbers.

        The returned feature vectors can be used as input to train another
        supervised learning model such as a
        :py:class:`~turicreate.logistic_classifier.LogisticClassifier`,
        an :py:class:`~turicreate.svm_classifier.SVMClassifier`, or a

        Parameters
        ----------
        dataset : SFrame
            Dataset of new observations. Must include columns with the same
            names as the features used for model training, but does not require
            a target column. Additional columns are ignored.

        missing_value_action: str, optional
            Action to perform when missing values are encountered. This can be
            one of:

            - 'auto': Choose a model dependent missing value policy.
            - 'impute': Proceed with evaluation by filling in the missing
                        values with the mean of the training data. Missing
                        values are also imputed if an entire column of data is
                        missing during evaluation.
            - 'none': Treat missing value as is. Model must be able to handle
                      missing value.
            - 'error' : Do not proceed with prediction and terminate with
                        an error message.

        Returns
        -------
        out : SArray
            An SArray of dtype array.array containing extracted features.

        Examples
        --------
        >>> data =  turicreate.SFrame(
            'https://static.turi.com/datasets/regression/houses.csv')

        >>> # Regression Tree Models
        >>> data['regression_tree_features'] = model.extract_features(data)

        >>> # Classification Tree Models
        >>> data['classification_tree_features'] = model.extract_features(data)
        """
        metric_name = '.'.join([self.__module__, 'extract_features'])
        _raise_error_if_not_sframe(dataset, "dataset")
        if missing_value_action == 'auto':
            missing_value_action = select_default_missing_value_policy(self,
                    'extract_features')

        options = dict()
        options.update({'model': self.__proxy__,
                        'model_name': self.__name__,
                        'missing_value_action': missing_value_action,
                        'dataset': dataset})
        target = _toolkits_main.run('supervised_learning_feature_extraction',
                                    options)
        return _map_unity_proxy_to_object(target['extracted'])

    def _extract_features_with_missing(self, dataset, tree_id = 0,
            missing_value_action = 'auto'):
        """
        Extract features along with all the missing features associated with
        a dataset.

        Parameters
        ----------
        dataset: bool
            Dataset on which to make predictions.

        missing_value_action: str, optional
            Action to perform when missing values are encountered. This can be
            one of:

            - 'auto': Choose a model dependent missing value policy.
            - 'impute': Proceed with evaluation by filling in the missing
                        values with the mean of the training data. Missing
                        values are also imputed if an entire column of data is
                        missing during evaluation.
            - 'none': Treat missing value as is. Model must be able to handle
                      missing value.
            - 'error' : Do not proceed with prediction and terminate with
                        an error message.

        Returns
        -------
        out : SFrame
            A table with two columns:
              - leaf_id          : Leaf id of the corresponding tree.
              - missing_features : A list of missing feature, index pairs
        """

        # Extract the features from only one tree.
        sf = dataset
        sf['leaf_id'] = self.extract_features(dataset, missing_value_action)\
                             .vector_slice(tree_id)\
                             .astype(int)

        tree = self._get_tree(tree_id)
        type_map = dict(zip(dataset.column_names(), dataset.column_types()))

        def get_missing_features(row):
            x = row['leaf_id']
            path = tree.get_prediction_path(x)
            missing_id = [] # List of "missing_id" children.

            # For each node in the prediction path.
            for p in path:
                fname = p['feature']
                idx = p['index']
                f = row[fname]
                if type_map[fname] in [int, float]:
                    if f is None:
                        missing_id.append(p['child_id'])

                elif type_map[fname] in [dict]:
                    if f is None:
                        missing_id.append(p['child_id'])

                    if idx not in f:
                        missing_id.append(p['child_id'])
                else:
                    pass
            return missing_id

        sf['missing_id'] = sf.apply(get_missing_features, list)
        return sf[['leaf_id', 'missing_id']]


    def _dump_to_text(self, with_stats):
        """
        Dump the models into a list of strings. Each
        string is a text representation of a tree.

        Parameters
        ----------
        with_stats : bool
            If true, include node statistics in the output.

        Returns
        -------
        out : SFrame
            A table with two columns: feature, count,
            ordered by 'count' in descending order.
        """
        return tc.extensions._xgboost_dump_model(self.__proxy__, with_stats=with_stats, format='text')

    def _dump_to_json(self, with_stats):
        """
        Dump the models into a list of strings. Each
        string is a text representation of a tree.

        Parameters
        ----------
        with_stats : bool
            If true, include node statistics in the output.

        Returns
        -------
        out : SFrame
            A table with two columns: feature, count,
            ordered by 'count' in descending order.
        """
        import json
        trees_json_str = tc.extensions._xgboost_dump_model(self.__proxy__, with_stats=with_stats, format='json')
        trees_json = [json.loads(x) for x in trees_json_str]

        # To avoid lose precision when using libjson, _dump_model with json format encode
        # numerical values in hexadecimal (little endian).
        # Now we need to convert them back to floats, using unpack. '<f' means single precision float
        # in little endian
        import struct
        import sys
        def hexadecimal_to_float(s):
            if sys.version_info[0] >= 3:
                return struct.unpack('<f', bytes.fromhex(s))[0] # unpack always return a tuple
            else:
                return struct.unpack('<f', s.decode('hex'))[0] # unpack always return a tuple

        for d in trees_json:
            nodes = d['vertices']
            for n in nodes:
                if 'value_hexadecimal' in n:
                    n['value'] = hexadecimal_to_float(n['value_hexadecimal'])
        return trees_json

    def _get_tree(self, tree_id = 0):
        """
        A simple pure python wrapper around a single (or ensemble) of Turi
        create decision trees.

        The tree can be obtained directly from the `trees_json` parameter in
        any GLC tree model objects (boosted trees, random forests, and decision
        trees).

        Examples
        --------
        .. sourcecode:: python

            # Train a tree ensemble.
            >>> url = 'https://static.turi.com/datasets/xgboost/mushroom.csv'
            >>> data = tc.SFrame.read_csv(url)

            >>> train, test = data.random_split(0.8)
            >>> model = tc.boosted_trees_classifier.create(train,
            ...                  target='label', validation_set = None)

            # Obtain the model as a tree object.
            >>> tree = model._get_tree()

            >>> tree = DecisionTree(model, tree_id = 0)

        """
        from . import _decision_tree
        return _decision_tree.DecisionTree.from_model(self, tree_id)

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
        data_fields = [
            ('Number of examples', 'num_examples'),
            ('Number of feature columns', 'num_features'),
            ('Number of unpacked features', 'num_unpacked_features')]
        if 'num_classes' in self._list_fields():
            data_fields.append(('Number of classes', 'num_classes'))

        training_fields = [
            ("Number of trees", 'num_trees'),
            ("Max tree depth", 'max_depth'),
            ("Training time (sec)", 'training_time')]

        for m in ['accuracy', 'log_loss', 'auc', 'rmse', 'max_error']:
            required_fields = ['training_%s' % m, 'validation_%s' %m]
            if (all(i in self._list_fields() for i in required_fields)):
                training_fields.append(('Training %s' % m, 'training_%s' % m))
                training_fields.append(('Validation %s' % m, 'validation_%s' % m))

        return ([data_fields, training_fields], ["Schema", "Settings"])

    def _export_coreml_impl(self, filename, context):
        tc.extensions._xgboost_export_as_model_asset(self.__proxy__, filename, context)

