GLMRegressor
________________________________________________________________________________

A generalized linear model regressor.


.. code-block:: proto

	message GLMRegressor {
	    message DoubleArray {
	        repeated double value = 1;
	    }
	
	    enum PostEvaluationTransform {
	        NoTransform = 0;
	        Logit = 1;
	        Probit = 2;
	    }
	
	    repeated DoubleArray weights = 1;
	    repeated double offset = 2;
	    PostEvaluationTransform postEvaluationTransform = 3;
	}






GLMRegressor.DoubleArray
--------------------------------------------------------------------------------




.. code-block:: proto

	    message DoubleArray {
	        repeated double value = 1;
	    }










GLMRegressor.PostEvaluationTransform
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum PostEvaluationTransform {
	        NoTransform = 0;
	        Logit = 1;
	        Probit = 2;
	    }