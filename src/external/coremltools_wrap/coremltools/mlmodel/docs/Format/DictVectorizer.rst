DictVectorizer
________________________________________________________________________________

Uses an index mapping to convert a dictionary to an array.

The output array will be equal in length to the index mapping vector parameter.
All keys in the input dictionary must be present in the index mapping vector.

For each item in the input dictionary, insert its value in the output array.
The position of the insertion is determined by the position of the item's key
in the index mapping. Any keys not present in the input dictionary, will be
zero in the output array.

For example: if the ``stringToIndex`` parameter is set to ``["a", "c", "b", "z"]``,
then an input of ``{"a": 4, "c": 8}`` will produce an output of ``[4, 8, 0, 0]``.


.. code-block:: proto

	message DictVectorizer {
	    oneof Map {
	        StringVector stringToIndex = 1;
	
	        Int64Vector int64ToIndex = 2;
	    }
	}