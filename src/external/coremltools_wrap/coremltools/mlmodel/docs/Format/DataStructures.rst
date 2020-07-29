StringToInt64Map
________________________________________________________________________________

A mapping from a string
to a 64-bit integer.


.. code-block:: proto

	message StringToInt64Map {
	    map<string, int64> map = 1;
	}








Int64ToStringMap
________________________________________________________________________________

A mapping from a 64-bit integer
to a string.


.. code-block:: proto

	message Int64ToStringMap {
	    map<int64, string> map = 1;
	}








StringToDoubleMap
________________________________________________________________________________

A mapping from a string
to a double-precision floating point number.


.. code-block:: proto

	message StringToDoubleMap {
	    map<string, double> map = 1;
	}








Int64ToDoubleMap
________________________________________________________________________________

A mapping from a 64-bit integer
to a double-precision floating point number.


.. code-block:: proto

	message Int64ToDoubleMap {
	    map<int64, double> map = 1;
	}








StringVector
________________________________________________________________________________

A vector of strings.


.. code-block:: proto

	message StringVector {
	    repeated string vector = 1;
	}






Int64Vector
________________________________________________________________________________

A vector of 64-bit integers.


.. code-block:: proto

	message Int64Vector {
	    repeated int64 vector = 1;
	}






FloatVector
________________________________________________________________________________

A vector of floating point numbers.


.. code-block:: proto

	message FloatVector {
	    repeated float vector = 1;
	}






DoubleVector
________________________________________________________________________________

A vector of double-precision floating point numbers.


.. code-block:: proto

	message DoubleVector {
	    repeated double vector = 1;
	}






Int64Range
________________________________________________________________________________

A range of int64 values


.. code-block:: proto

	message Int64Range {
	    int64 minValue = 1;
	    int64 maxValue = 2;
	}






Int64Set
________________________________________________________________________________

A set of int64 values


.. code-block:: proto

	message Int64Set {
	    repeated int64 values = 1;
	}






DoubleRange
________________________________________________________________________________

A range of double values


.. code-block:: proto

	message DoubleRange {
	    double minValue = 1;
	    double maxValue = 2;
	}