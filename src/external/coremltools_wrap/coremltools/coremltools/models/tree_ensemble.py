# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
Tree ensemble builder class to construct CoreML models.
"""
from .. import SPECIFICATION_VERSION
from ..proto import Model_pb2 as _Model_pb2
from ..proto import TreeEnsemble_pb2 as _TreeEnsemble_pb2
from ..proto import FeatureTypes_pb2 as _FeatureTypes_pb2

from ._interface_management import set_regressor_interface_params
from ._interface_management import set_classifier_interface_params
import collections as _collections

class TreeEnsembleBase(object):
    """
    Base class for the tree ensemble builder class.  This should be instantiated
    either through the :py:class:`TreeEnsembleRegressor` or 
    :py:class:`TreeEnsembleClassifier` classes. 
    """

    def __init__(self):
        """
        High level Python API to build a tree ensemble model for Core ML.
        """
        # Set inputs and outputs
        spec = _Model_pb2.Model()
        spec.specificationVersion = SPECIFICATION_VERSION

        # Save the spec in the protobuf
        self.spec = spec

    def set_default_prediction_value(self, values):
        """
        Set the default prediction value(s).

        The values given here form the base prediction value that the values 
        at activated leaves are added to.  If values is a scalar, then 
        the output of the tree must also be 1 dimensional; otherwise, values
        must be a list with length matching the dimension of values in the tree.

        Parameters
        ----------
        values: [int | double | list[double]]
            Default values for predictions.  
            
        """
        if type(values) is not list:
            values = [float(values)]
        self.tree_parameters.numPredictionDimensions = len(values)
        for value in values:
            self.tree_parameters.basePredictionValue.append(value)

    def set_post_evaluation_transform(self, value):
        r"""
        Set the post processing transform applied after the prediction value 
        from the tree ensemble. 

        Parameters
        ----------

        value: str

            A value denoting the transform applied.  Possible values are: 

            - "NoTransform" (default).  Do not apply a transform. 

            - "Classification_SoftMax".  

              Apply a softmax function to the outcome to produce normalized, 
              non-negative scores that sum to 1.  The transformation applied to
              dimension `i` is equivalent to: 
                
                .. math::

                    \frac{e^{x_i}}{\sum_j e^{x_j}}

              Note: This is the output transformation applied by the XGBoost package 
              with multiclass classification.

            - "Regression_Logistic". 

              Applies a logistic transform the predicted value, specifically: 

                .. math:: 

                    (1 + e^{-v})^{-1}

              This is the transformation used in binary classification.


        """
        self.tree_spec.postEvaluationTransform = \
                _TreeEnsemble_pb2.TreeEnsemblePostEvaluationTransform.Value(value)

    def add_branch_node(self, tree_id, node_id, feature_index, feature_value,
            branch_mode, true_child_id, false_child_id, relative_hit_rate = None,
            missing_value_tracks_true_child = False):
        """
        Add a branch node to the tree ensemble.

        Parameters
        ----------
        tree_id: int
            ID of the tree to add the node to.

        node_id: int
            ID of the node within the tree.

        feature_index: int
            Index of the feature in the input being split on.

        feature_value: double or int
            The value used in the feature comparison determining the traversal 
            direction from this node. 

        branch_mode: str
            Branch mode of the node, specifying the condition under which the node 
            referenced by `true_child_id` is called next.   

            Must be one of the following:

              - `"BranchOnValueLessThanEqual"`. Traverse to node `true_child_id`
                if `input[feature_index] <= feature_value`, and `false_child_id` 
                otherwise. 

              - `"BranchOnValueLessThan"`. Traverse to node `true_child_id`
                if `input[feature_index] < feature_value`, and `false_child_id` 
                otherwise. 

              - `"BranchOnValueGreaterThanEqual"`. Traverse to node `true_child_id`
                if `input[feature_index] >= feature_value`, and `false_child_id` 
                otherwise. 

              - `"BranchOnValueGreaterThan"`. Traverse to node `true_child_id`
                if `input[feature_index] > feature_value`, and `false_child_id` 
                otherwise. 

              - `"BranchOnValueEqual"`. Traverse to node `true_child_id`
                if `input[feature_index] == feature_value`, and `false_child_id` 
                otherwise. 

              - `"BranchOnValueNotEqual"`. Traverse to node `true_child_id`
                if `input[feature_index] != feature_value`, and `false_child_id` 
                otherwise. 

        true_child_id: int
            ID of the child under the true condition of the split.  An error will
            be raised at model validation if this does not match the `node_id`
            of a node instantiated by `add_branch_node` or `add_leaf_node` within
            this `tree_id`.

        false_child_id: int
            ID of the child under the false condition of the split.  An error will
            be raised at model validation if this does not match the `node_id`
            of a node instantiated by `add_branch_node` or `add_leaf_node` within
            this `tree_id`.

        relative_hit_rate: float [optional]
            When the model is converted compiled by CoreML, this gives hints to 
            Core ML about which node is more likely to be hit on evaluation, 
            allowing for additional optimizations. The values can be on any scale, 
            with the values between child nodes being compared relative to each 
            other.

        missing_value_tracks_true_child: bool [optional]
            If the training data contains NaN values or missing values, then this
            flag determines which direction a NaN value traverses.

        """
        spec_node = self.tree_parameters.nodes.add()
        spec_node.treeId = tree_id
        spec_node.nodeId = node_id
        spec_node.branchFeatureIndex = feature_index
        spec_node.branchFeatureValue = feature_value
        spec_node.trueChildNodeId = true_child_id
        spec_node.falseChildNodeId = false_child_id
        spec_node.nodeBehavior = \
           _TreeEnsemble_pb2.TreeEnsembleParameters.TreeNode.TreeNodeBehavior.Value(branch_mode)

        if relative_hit_rate is not None:
            spec_node.relativeHitRate = relative_hit_rate
        spec_node.missingValueTracksTrueChild = missing_value_tracks_true_child

    def add_leaf_node(self, tree_id, node_id, values, relative_hit_rate = None):
        """
        Add a leaf node to the tree ensemble.

        Parameters
        ----------
        tree_id: int
            ID of the tree to add the node to.

        node_id: int
            ID of the node within the tree. 

        values: [float | int | list | dict]
            Value(s) at the leaf node to add to the prediction when this node is 
            activated.  If the prediction dimension of the tree is 1, then the 
            value is specified as a float or integer value.  
            
            For multidimensional predictions, the values can be a list of numbers 
            with length matching the dimension of the predictions or a dictionary
            mapping index to value added to that dimension.

            Note that the dimension of any tree must match the dimension given
            when :py:meth:`set_default_prediction_value` is called. 

        """
        spec_node = self.tree_parameters.nodes.add()
        spec_node.treeId = tree_id
        spec_node.nodeId = node_id
        spec_node.nodeBehavior = \
           _TreeEnsemble_pb2.TreeEnsembleParameters.TreeNode.TreeNodeBehavior.Value('LeafNode')

        if not isinstance(values, _collections.Iterable):
            values = [values]

        if relative_hit_rate is not None:
            spec_node.relativeHitRate = relative_hit_rate
                
        if type(values) == dict:
            iter = values.items()
        else:
            iter = enumerate(values)

        for index, value in iter:
            ev_info = spec_node.evaluationInfo.add()
            ev_info.evaluationIndex = index
            ev_info.evaluationValue = float(value)
            spec_node.nodeBehavior = \
                _TreeEnsemble_pb2.TreeEnsembleParameters.TreeNode.TreeNodeBehavior.Value('LeafNode')

class TreeEnsembleRegressor(TreeEnsembleBase):
    """
    Tree Ensemble builder class to construct a Tree Ensemble regression model. 

    The TreeEnsembleRegressor class constructs a Tree Ensemble model incrementally
    using methods to add branch and leaf nodes specifying the behavior of the model. 

    Examples
    --------

    .. sourcecode:: python

        >>> # Required inputs
        >>> import coremltools
        >>> from coremltools.models import datatypes
        >>> from coremltools.models.tree_ensemble import TreeEnsembleRegressor
        >>> import numpy as np

        >>> # Define input features
        >>> input_features = [("a", datatypes.Array(3)), ("b", (datatypes.Double()))]

        >>> # Define output_features
        >>> output_features = [("predicted_values", datatypes.Double())]

        >>> tm = TreeEnsembleRegressor(features = input_features, target = output_features)

        >>> # Split on a[2] <= 3
        >>> tm.add_branch_node(0, 0, 2, 3, "BranchOnValueLessThanEqual", 1, 2)

        >>> # Add leaf to the true branch of node 0 that subtracts 1.
        >>> tm.add_leaf_node(0, 1, -1)

        >>> # Add split on b == 0 to the false branch of node 0, which is index 3
        >>> tm.add_branch_node(0, 2, 3, 0, "BranchOnValueEqual", 3, 4)

        >>> # Add leaf to the true branch of node 2 that adds 1 to the result.
        >>> tm.add_leaf_node(0, 3, 1)

        >>> # Add leaf to the false branch of node 2 that subtracts 1 from the result.
        >>> tm.add_leaf_node(0, 4, -1)

        >>> tm.set_default_prediction_value([0, 0])

        >>> # save the model to a .mlmodel file
        >>> model_path = './tree.mlmodel'
        >>> coremltools.models.utils.save_spec(tm.spec, model_path)

        >>> # load the .mlmodel
        >>> mlmodel = coremltools.models.MLModel(model_path)

        >>> # make predictions
        >>> test_input = {
        >>>     'a': np.array([0, 1, 2]).astype(np.float32),
        >>>     "b": 3.0,
        >>> }
        >>> predictions = mlmodel.predict(test_input)

    """


    def __init__(self, features, target):
        """
        Create a Tree Ensemble regression model that takes one or more input 
        features and maps them to an output feature. 

        Parameters
        ----------

        features: [list of features]
            Name(s) of the input features, given as a list of `('name', datatype)`
            tuples.  The features are one of :py:class:`models.datatypes.Int64`, 
            :py:class:`datatypes.Double`, or :py:class:`models.datatypes.Array`.  
            Feature indices in the nodes are counted sequentially from 0 through 
            the features. 
        
        target:  (default = None)
           Name of the target feature predicted. 
        """
        super(TreeEnsembleRegressor, self).__init__()
        spec = self.spec
        spec = set_regressor_interface_params(spec, features, target)
        self.tree_spec = spec.treeEnsembleRegressor
        self.tree_parameters = self.tree_spec.treeEnsemble

class TreeEnsembleClassifier(TreeEnsembleBase):
    """
    Tree Ensemble builder class to construct a Tree Ensemble classification model. 

    The TreeEnsembleClassifier class constructs a Tree Ensemble model incrementally
    using methods to add branch and leaf nodes specifying the behavior of the model. 


    Examples
    --------

    .. sourcecode:: python

        >>> input_features = [("a", datatypes.Array(3)), ("b", datatypes.Double())]

        >>> tm = TreeEnsembleClassifier(features = input_features, class_labels = [0, 1], 
                                        output_features = "predicted_class")

        >>> # Split on a[2] <= 3
        >>> tm.add_branch_node(0, 0, 2, 3, "BranchOnValueLessThanEqual", 1, 2)

        >>> # Add leaf to the true branch of node 0 that subtracts 1.
        >>> tm.add_leaf_node(0, 1, -1)

        >>> # Add split on b == 0 to the false branch of node 0. 
        >>> tm.add_branch_node(0, 2, 3, 0, "BranchOnValueEqual", 3, 4)

        >>> # Add leaf to the true branch of node 2 that adds 1 to the result.
        >>> tm.add_leaf_node(0, 3, 1)

        >>> # Add leaf to the false branch of node 2 that subtracts 1 from the result.
        >>> tm.add_leaf_node(0, 4, -1)

        >>> # Put in a softmax transform to translate these into probabilities. 
        >>> tm.set_post_evaluation_transform("Classification_SoftMax")

        >>> tm.set_default_prediction_value([0, 0])

        >>> # save the model to a .mlmodel file
        >>> model_path = './tree.mlmodel'
        >>> coremltools.models.utils.save_spec(tm.spec, model_path)

        >>> # load the .mlmodel
        >>> mlmodel = coremltools.models.MLModel(model_path)

        >>> # make predictions
        >>> test_input = {
        >>>     'a': np.array([0, 1, 2]).astype(np.float32),
        >>>     "b": 3.0,
        >>> }
        >>> predictions = mlmodel.predict(test_input)

    """


    def __init__(self, features, class_labels, output_features):
        """
        Create a tree ensemble classifier model.

        Parameters
        ----------
        features: [list of features]
            Name(s) of the input features, given as a list of `('name', datatype)`
            tuples.  The features are one of :py:class:`models.datatypes.Int64`, 
            :py:class:`datatypes.Double`, or :py:class:`models.datatypes.Array`.  
            Feature indices in the nodes are counted sequentially from 0 through 
            the features. 
        
        class_labels: [list]
            A list of string or integer class labels to use in making predictions. 
            The length of this must match the dimension of the tree model. 

        output_features: [list]
            A string or a list of two strings specifying the names of the two 
            output features, the first being a class label corresponding 
            to the class with the highest predicted score, and the second being 
            a dictionary mapping each class to its score. If `output_features` 
            is a string, it specifies the predicted class label and the class 
            scores is set to the default value of `"classProbability."` 
        """
        super(TreeEnsembleClassifier, self).__init__()
        spec = self.spec
        spec = set_classifier_interface_params(spec, features, class_labels,
                'treeEnsembleClassifier', output_features)
        self.tree_spec = spec.treeEnsembleClassifier
        self.tree_parameters = self.tree_spec.treeEnsemble

