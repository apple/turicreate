WordTagger
________________________________________________________________________________

A model which takes a single input string and outputs a
sequence of tokens, tags for tokens, along with their
locations and lengths, in the original string.


.. code-block:: proto

	message WordTagger {
	
	    uint32 revision = 1;
	
	    string language = 10;
	
	    string tokensOutputFeatureName = 20;
	
	    string tokenTagsOutputFeatureName = 21;
	
	    string tokenLocationsOutputFeatureName = 22;
	
	    string tokenLengthsOutputFeatureName = 23;
	
	    bytes modelParameterData = 100;
	
	    oneof Tags {
	        StringVector stringTags = 200;
	    }
	
	
	    
	}