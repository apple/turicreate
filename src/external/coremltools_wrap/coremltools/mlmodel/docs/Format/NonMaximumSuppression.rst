NonMaximumSuppression
________________________________________________________________________________




.. code-block:: proto

	message NonMaximumSuppression {
	    // Suppression methods:
	    message PickTop {
	        bool perClass = 1;
	    }
	
	    oneof SuppressionMethod {
	        PickTop pickTop = 1;
	    }
	
	    oneof ClassLabels {
	        StringVector stringClassLabels = 100;
	        Int64Vector int64ClassLabels = 101;
	    }
	
	    double iouThreshold = 110;
	
	           it means there is a 60% (0.2 + 0.4) confidence that an object is
	           present)
	    double confidenceThreshold = 111;
	
	    string confidenceInputFeatureName = 200;
	
	    string coordinatesInputFeatureName = 201;
	
	    string iouThresholdInputFeatureName = 202;
	
	    string confidenceThresholdInputFeatureName = 203;
	
	    string confidenceOutputFeatureName = 210;
	
	    string coordinatesOutputFeatureName = 211;
	}






NonMaximumSuppression.PickTop
--------------------------------------------------------------------------------




.. code-block:: proto

	    message PickTop {
	        bool perClass = 1;
	    }