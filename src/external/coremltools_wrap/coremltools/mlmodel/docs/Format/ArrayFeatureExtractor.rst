ArrayFeatureExtractor
________________________________________________________________________________

An array feature extractor.

Given an index, extracts the value at that index from its array input.
Indexes are zero-based.


.. code-block:: proto

	message ArrayFeatureExtractor {
	    repeated uint64 extractIndex = 1;
	}