CategoricalMapping
________________________________________________________________________________

A categorical mapping.

This allows conversion from integers to strings, or from strings to integers.


.. code-block:: proto

	message CategoricalMapping {
	    oneof MappingType {
	        // Conversion from strings to integers
	        StringToInt64Map stringToInt64Map = 1;
	
	        // Conversion from integer to string
	        Int64ToStringMap int64ToStringMap = 2;
	    }
	
	    oneof ValueOnUnknown {
	        // Default output when converting from an integer to a string.
	        string strValue = 101;
	
	        // Default output when converting from a string to an integer.
	        int64 int64Value = 102;
	    }
	}