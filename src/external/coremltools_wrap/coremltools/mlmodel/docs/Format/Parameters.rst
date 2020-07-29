Int64Parameter
________________________________________________________________________________

Int64 parameter,
consisting of a default int64 value, and allowed range or set of values
value is unbounded if AllowedValues is not set.


.. code-block:: proto

	message Int64Parameter {
	    int64 defaultValue = 1;
	    oneof AllowedValues {
	        Int64Range range = 10;
	        Int64Set set = 11;
	    }
	}






DoubleParameter
________________________________________________________________________________

Double parameter,
consisting of a default double value, and allowed range of values
value is unbounded if AllowedValues is not set.


.. code-block:: proto

	message DoubleParameter {
	    double defaultValue = 1;
	    oneof AllowedValues {
	        DoubleRange range = 10;
	    }
	}






StringParameter
________________________________________________________________________________

String parameter,
A default string value must be provided


.. code-block:: proto

	message StringParameter {
	    string defaultValue = 1;
	}






BoolParameter
________________________________________________________________________________

String parameter,
A default bool value must be provided


.. code-block:: proto

	message BoolParameter {
	    bool defaultValue = 1;
	}