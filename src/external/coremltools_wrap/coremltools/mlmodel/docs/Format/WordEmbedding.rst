WordEmbedding
________________________________________________________________________________

A model which maps a set of strings into a finite-dimensional real vector space.


.. code-block:: proto

	message WordEmbedding {
	
	    uint32 revision = 1;
	    
	    string language = 10;
	
	    bytes modelParameterData = 100;
	    
	}