TextClassifier
________________________________________________________________________________

A model which takes a single input string and outputs a
label for the input.


.. code-block:: proto

	message TextClassifier {
	
	    uint32 revision = 1;
	    
	    string language = 10;
	
	    bytes modelParameterData = 100;
	    
	    oneof ClassLabels {
	        StringVector stringClassLabels = 200;
	    }
	    
	}