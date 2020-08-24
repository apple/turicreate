LinkedModel
________________________________________________________________________________

A model which wraps another (compiled) model external to this one


.. code-block:: proto

	message LinkedModel {
	
	    oneof LinkType {
	        // A model located via a file system path
	        LinkedModelFile linkedModelFile = 1;
	    }
	}






LinkedModelFile
________________________________________________________________________________




.. code-block:: proto

	message LinkedModelFile {
	
	    // Model file name: e.g. "MyFetureExtractor.mlmodelc"
	    StringParameter linkedModelFileName = 1;
	
	    // Search path to find the linked model file
	    // Multiple paths can be searched using the unix-style path separator ":"
	    // Each path can be relative (to this model) or absolute
	    //
	    // An empty string is the same as teh relative search path "."
	    // which searches in the same location as this model file
	    //
	    // There are some special paths which start with $
	    // - $BUNDLE_MAIN - Indicates to look in the main bundle
	    // - $BUNDLE_IDENTIFIER(identifier) - Looks in Bunde with given identifer
	    StringParameter linkedModelSearchPath = 2;
	}