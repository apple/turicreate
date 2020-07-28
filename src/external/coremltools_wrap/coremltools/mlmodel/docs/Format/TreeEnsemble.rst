Each tree is a collection of nodes,
each of which is identified by a unique identifier.

Each node is either a branch or a leaf node.
A branch node evaluates a value according to a behavior;
if true, the node identified by ``true_child_node_id`` is evaluated next,
if false, the node identified by ``false_child_node_id`` is evaluated next.
A leaf node adds the evaluation value to the base prediction value
to get the final prediction.

A tree must have exactly one root node,
which has no parent node.
A tree must not terminate on a branch node.
All leaf nodes must be accessible
by evaluating one or more branch nodes in sequence,
starting from the root node.



TreeEnsembleParameters
________________________________________________________________________________

Tree ensemble parameters.


.. code-block:: proto

	message TreeEnsembleParameters {
	    message TreeNode {
	        uint64 treeId = 1;
	        uint64 nodeId = 2;
	
	        enum TreeNodeBehavior {
	            BranchOnValueLessThanEqual = 0;
	            BranchOnValueLessThan = 1;
	            BranchOnValueGreaterThanEqual = 2;
	            BranchOnValueGreaterThan = 3;
	            BranchOnValueEqual = 4;
	            BranchOnValueNotEqual = 5;
	            LeafNode = 6;
	        }
	
	        TreeNodeBehavior nodeBehavior = 3;
	
	        uint64 branchFeatureIndex = 10;
	        double branchFeatureValue = 11;
	        uint64 trueChildNodeId = 12;
	        uint64 falseChildNodeId = 13;
	        bool missingValueTracksTrueChild = 14;
	
	        message EvaluationInfo {
	           uint64 evaluationIndex = 1;
	           double evaluationValue = 2;
	        }
	
	        repeated EvaluationInfo evaluationInfo = 20;
	
	        double relativeHitRate = 30;
	    }
	
	    repeated TreeNode nodes = 1;
	
	    uint64 numPredictionDimensions = 2;
	
	    repeated double basePredictionValue = 3;
	}






TreeEnsembleParameters.TreeNode
--------------------------------------------------------------------------------




.. code-block:: proto

	    message TreeNode {
	        uint64 treeId = 1;
	        uint64 nodeId = 2;
	
	        enum TreeNodeBehavior {
	            BranchOnValueLessThanEqual = 0;
	            BranchOnValueLessThan = 1;
	            BranchOnValueGreaterThanEqual = 2;
	            BranchOnValueGreaterThan = 3;
	            BranchOnValueEqual = 4;
	            BranchOnValueNotEqual = 5;
	            LeafNode = 6;
	        }
	
	        TreeNodeBehavior nodeBehavior = 3;
	
	        uint64 branchFeatureIndex = 10;
	        double branchFeatureValue = 11;
	        uint64 trueChildNodeId = 12;
	        uint64 falseChildNodeId = 13;
	        bool missingValueTracksTrueChild = 14;
	
	        message EvaluationInfo {
	           uint64 evaluationIndex = 1;
	           double evaluationValue = 2;
	        }
	
	        repeated EvaluationInfo evaluationInfo = 20;
	
	        double relativeHitRate = 30;
	    }






TreeEnsembleParameters.TreeNode.EvaluationInfo
--------------------------------------------------------------------------------

The leaf mode.

If ``nodeBahavior`` == ``LeafNode``,
then the evaluationValue is added to the base prediction value
in order to get the final prediction.
To support multiclass classification
as well as regression and binary classification,
the evaluation value is encoded here as a sparse vector,
with evaluationIndex being the index of the base vector
that evaluation value is added to.
In the single class case,
it is expected that evaluationIndex is exactly 0.


.. code-block:: proto

	        message EvaluationInfo {
	           uint64 evaluationIndex = 1;
	           double evaluationValue = 2;
	        }






TreeEnsembleClassifier
________________________________________________________________________________

A tree ensemble classifier.


.. code-block:: proto

	message TreeEnsembleClassifier {
	    TreeEnsembleParameters treeEnsemble = 1;
	    TreeEnsemblePostEvaluationTransform postEvaluationTransform = 2;
	
	    // Required class label mapping
	    oneof ClassLabels {
	        StringVector stringClassLabels = 100;
	        Int64Vector int64ClassLabels = 101;
	    }
	}






TreeEnsembleRegressor
________________________________________________________________________________

A tree ensemble regressor.


.. code-block:: proto

	message TreeEnsembleRegressor {
	    TreeEnsembleParameters treeEnsemble = 1;
	    TreeEnsemblePostEvaluationTransform postEvaluationTransform = 2;
	}










TreeEnsembleParameters.TreeNode.TreeNodeBehavior
--------------------------------------------------------------------------------



.. code-block:: proto

	        enum TreeNodeBehavior {
	            BranchOnValueLessThanEqual = 0;
	            BranchOnValueLessThan = 1;
	            BranchOnValueGreaterThanEqual = 2;
	            BranchOnValueGreaterThan = 3;
	            BranchOnValueEqual = 4;
	            BranchOnValueNotEqual = 5;
	            LeafNode = 6;
	        }



TreeEnsemblePostEvaluationTransform
________________________________________________________________________________

A tree ensemble post-evaluation transform.

.. code-block:: proto

	enum TreeEnsemblePostEvaluationTransform {
	    NoTransform = 0;
	    Classification_SoftMax = 1;
	    Regression_Logistic = 2;
	    Classification_SoftMaxWithZeroClassReference = 3;
	}