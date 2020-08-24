OneHotEncoder
________________________________________________________________________________

Transforms a categorical feature into an array. The array will be all
zeros expect a single entry of one.

Each categorical value will map to an index, this mapping is given by
either the ``stringCategories`` parameter or the ``int64Categories``
parameter.


.. code-block:: proto

	message OneHotEncoder {
	    enum HandleUnknown {
	        ErrorOnUnknown = 0;
	        IgnoreUnknown = 1;   // Output will be all zeros for unknown values.
	    }
	
	    oneof CategoryType {
	        StringVector stringCategories = 1;
	        Int64Vector int64Categories = 2;
	    }
	
	    // Output can be a dictionary with only one entry, instead of an array.
	    bool outputSparse = 10;
	
	    HandleUnknown handleUnknown = 11;
	}










OneHotEncoder.HandleUnknown
--------------------------------------------------------------------------------



.. code-block:: proto

	    enum HandleUnknown {
	        ErrorOnUnknown = 0;
	        IgnoreUnknown = 1;   // Output will be all zeros for unknown values.
	    }