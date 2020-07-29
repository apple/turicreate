VisionFeaturePrint
________________________________________________________________________________

A model which takes an input image and outputs array(s) of features
according to the specified feature types


.. code-block:: proto

	message VisionFeaturePrint {

	    // Specific vision feature print types

	    // Scene extracts features useful for identifying contents of natural images
	    // in both indoor and outdoor environments
	    message Scene {
	        enum SceneVersion {
	            SCENE_VERSION_INVALID = 0;
	            // VERSION_1 is available on iOS,tvOS 12.0+, macOS 10.14+
	            // It uses a 299x299 input image and yields a 2048 float feature vector
	            SCENE_VERSION_1 = 1;
	        }

	        SceneVersion version = 1;
	    }

	    // Object extracts features useful for identifying and localizing
	    // objects in natural images
	    message Object {
	        enum ObjectVersion {
	            OBJECT_VERSION_INVALID = 0;
	            // VERSION_1 is available on iOS,tvOS 14.0+, macOS 11.0+
	            // It uses a 299x299 input image and yields two multiarray
	            // features: one at high resolution of shape (288, 35, 35)
	            // the other at low resolution of shape (768, 17, 17)
	            OBJECT_VERSION_1 = 1;
	        }

	        ObjectVersion version = 1;

	        repeated string output = 100;
	    }

	    // Vision feature print type
	    oneof VisionFeaturePrintType {
	        Scene scene = 20;
	        Object object = 21;
	    }

	}






VisionFeaturePrint.Scene
--------------------------------------------------------------------------------




.. code-block:: proto

	    message Scene {
	        enum SceneVersion {
	            SCENE_VERSION_INVALID = 0;
	            // VERSION_1 is available on iOS,tvOS 12.0+, macOS 10.14+
	            // It uses a 299x299 input image and yields a 2048 float feature vector
	            SCENE_VERSION_1 = 1;
	        }

	        SceneVersion version = 1;
	    }






VisionFeaturePrint.Object
--------------------------------------------------------------------------------




.. code-block:: proto

	    message Object {
	        enum ObjectVersion {
	            OBJECT_VERSION_INVALID = 0;
	            // VERSION_1 is available on iOS,tvOS 14.0+, macOS 11.0+
	            // It uses a 299x299 input image and yields two multiarray
	            // features: one at high resolution of shape (288, 35, 35)
	            // the other at low resolution of shape (768, 17, 17)
	            OBJECT_VERSION_1 = 1;
	        }

	        ObjectVersion version = 1;

	        repeated string output = 100;
	    }










VisionFeaturePrint.Object.ObjectVersion
--------------------------------------------------------------------------------



.. code-block:: proto

	        enum ObjectVersion {
	            OBJECT_VERSION_INVALID = 0;
	            // VERSION_1 is available on iOS,tvOS 14.0+, macOS 11.0+
	            // It uses a 299x299 input image and yields two multiarray
	            // features: one at high resolution of shape (288, 35, 35)
	            // the other at low resolution of shape (768, 17, 17)
	            OBJECT_VERSION_1 = 1;
	        }



VisionFeaturePrint.Scene.SceneVersion
--------------------------------------------------------------------------------



.. code-block:: proto

	        enum SceneVersion {
	            SCENE_VERSION_INVALID = 0;
	            // VERSION_1 is available on iOS,tvOS 12.0+, macOS 10.14+
	            // It uses a 299x299 input image and yields a 2048 float feature vector
	            SCENE_VERSION_1 = 1;
	        }
