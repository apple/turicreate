GLMClassifier
________________________________________________________________________________

A generalized linear model classifier.


.. code-block:: proto

	message GLMClassifier {
	    message DoubleArray {
	        repeated double value = 1;
	    }
	
	    enum PostEvaluationTransform {
	        Logit = 0;
	        Probit = 1; 
	    }
	
	    enum ClassEncoding {
	        ReferenceClass = 0; 
	        OneVsRest = 1; 
	    }
	
	    repeated DoubleArray weights = 1;
	    repeated double offset = 2;
	    PostEvaluationTransform postEvaluationTransform = 3;
	    ClassEncoding classEncoding = 4;
	
	    oneof ClassLabels {
	        StringVector stringClassLabels = 100;
	        Int64Vector int64ClassLabels = 101;
	    }
	}






GLMClassifier.DoubleArray
--------------------------------------------------------------------------------




.. code-block:: proto

	    message DoubleArray {
	        repeated double value = 1;
	    }










GLMClassifier.ClassEncoding
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum ClassEncoding {
	        ReferenceClass = 0; 
	        OneVsRest = 1; 
	    }



GLMClassifier.PostEvaluationTransform
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum PostEvaluationTransform {
	        Logit = 0;
	        Probit = 1; 
	    }