Imputer
________________________________________________________________________________

A transformer that replaces missing values with a default value,
such as a statistically-derived value.

If ``ReplaceValue`` is set, then missing values of that type are
replaced with the corresponding value.

For example: if ``replaceDoubleValue`` is set to ``NaN``
and a single ``NaN`` double value is provided as input,
then it is replaced by ``imputedDoubleValue``. However
if the input is an array of doubles, then any instances
of ``NaN`` in the array is replaced with the corresponding
value in ``imputedDoubleArray``.


.. code-block:: proto

	message Imputer {
	    oneof ImputedValue {
	        double imputedDoubleValue = 1;
	        int64 imputedInt64Value = 2;
	        string imputedStringValue = 3;
	        DoubleVector imputedDoubleArray = 4;
	        Int64Vector imputedInt64Array = 5;
	        StringToDoubleMap imputedStringDictionary = 6;
	        Int64ToDoubleMap imputedInt64Dictionary = 7;
	    }
	
	    oneof ReplaceValue {
	        double replaceDoubleValue = 11;
	        int64 replaceInt64Value = 12;
	        string replaceStringValue = 13;
	    }
	}