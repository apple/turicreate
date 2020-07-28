FeatureVectorizer
________________________________________________________________________________

A FeatureVectorizer puts one or more features into a single array.

The ordering of features in the output array is determined by
``inputList``.

``inputDimensions`` is a zero based index.


.. code-block:: proto

	message FeatureVectorizer {
	    message InputColumn {
	        string inputColumn = 1;
	        uint64 inputDimensions = 2;
	    }
	
	    repeated InputColumn inputList = 1;
	}






FeatureVectorizer.InputColumn
--------------------------------------------------------------------------------




.. code-block:: proto

	    message InputColumn {
	        string inputColumn = 1;
	        uint64 inputDimensions = 2;
	    }