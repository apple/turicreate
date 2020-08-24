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



ItemSimilarityRecommender
________________________________________________________________________________

Item Similarity Recommender

 The Item Similarity recommender takes as input a list of items and scores,
 then uses that information and a table of item similarities to predict similarity
 scores for all items.  By default, the items predicted are most similar to the given
 items but not part of that item set.

 The predicted score for a given item k is
   sum_(i in observed items)   sim_(k,i) * (score_i - shift_k)

 Because only the most similar scores for each item i are stored,
 sim_(k,i) is often zero.

 For many models, the score adjustment parameter shift_j is zero -- it's occasionally used
 to counteract global biases for popular items.


 References:


.. code-block:: proto

	message ItemSimilarityRecommender {
	
	    message ConnectedItem {
	        uint64 itemId = 1;
	        double similarityScore = 2;
	    }
	
	    message SimilarItems {
	        uint64 itemId = 1;
	        repeated ConnectedItem similarItemList = 2;
	        double itemScoreAdjustment = 3;
	    }
	
	    repeated SimilarItems itemItemSimilarities = 1;
	
	    StringVector itemStringIds = 2;
	    Int64Vector itemInt64Ids = 3;
	
	
	    string recommendedItemListOutputFeatureName = 20;
	    string recommendedItemScoreOutputFeatureName = 21;
	
	}






ItemSimilarityRecommender.ConnectedItem
--------------------------------------------------------------------------------

The items similar to a given base item.


.. code-block:: proto

	    message ConnectedItem {
	        uint64 itemId = 1;
	        double similarityScore = 2;
	    }






ItemSimilarityRecommender.SimilarItems
--------------------------------------------------------------------------------

The formula for the score of a given model as given above, with shift_k
  parameter given by itemScoreAdjustment, and the similar item list filling in
  all the known sim(k,i) scores for i given by itemID and k given by the itemID parameter in
  the similarItemList.


.. code-block:: proto

	    message SimilarItems {
	        uint64 itemId = 1;
	        repeated ConnectedItem similarItemList = 2;
	        double itemScoreAdjustment = 3;
	    }