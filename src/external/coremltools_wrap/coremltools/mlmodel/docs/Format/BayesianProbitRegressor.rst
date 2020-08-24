BayesianProbitRegressor
________________________________________________________________________________




.. code-block:: proto

	message BayesianProbitRegressor {
	
	    message Gaussian {
	        double mean = 1;
	        double precision = 2; // inverse of the variance
	    }
	
	    message FeatureValueWeight {
	        uint32 featureValue = 1;
	        Gaussian featureWeight = 2;
	    }
	
	    message FeatureWeight {
	        uint32 featureId = 1;
	        repeated FeatureValueWeight weights = 2;
	    }
	
	    uint32 numberOfFeatures = 1;
	
	    Gaussian bias = 2;  // bias term
	
	    repeated FeatureWeight features = 3;  // feature weights
	
	    string regressionInputFeatureName = 10;
	
	    string optimismInputFeatureName = 11;
	
	    string samplingScaleInputFeatureName = 12;
	
	    string samplingTruncationInputFeatureName = 13;
	
	    string meanOutputFeatureName = 20;
	
	    string varianceOutputFeatureName = 21;
	
	    string pessimisticProbabilityOutputFeatureName = 22;
	
	    string sampledProbabilityOutputFeatureName = 23;
	}






BayesianProbitRegressor.Gaussian
--------------------------------------------------------------------------------




.. code-block:: proto

	    message Gaussian {
	        double mean = 1;
	        double precision = 2; // inverse of the variance
	    }






BayesianProbitRegressor.FeatureValueWeight
--------------------------------------------------------------------------------




.. code-block:: proto

	    message FeatureValueWeight {
	        uint32 featureValue = 1;
	        Gaussian featureWeight = 2;
	    }






BayesianProbitRegressor.FeatureWeight
--------------------------------------------------------------------------------




.. code-block:: proto

	    message FeatureWeight {
	        uint32 featureId = 1;
	        repeated FeatureValueWeight weights = 2;
	    }