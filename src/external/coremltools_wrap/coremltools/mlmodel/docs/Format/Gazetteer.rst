Gazetteer
________________________________________________________________________________

A model which uses an efficient probabilistic representation
for assigning labels to a set of strings.


.. code-block:: proto

	message Gazetteer {
	
	    uint32 revision = 1;
	    
	    string language = 10;
	
	    bytes modelParameterData = 100;
	    
	    oneof ClassLabels {
	        StringVector stringClassLabels = 200;
	    }
	    
	}