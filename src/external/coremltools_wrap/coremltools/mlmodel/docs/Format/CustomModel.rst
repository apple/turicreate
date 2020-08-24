CustomModel
________________________________________________________________________________

A parameterized model whose function is defined in code


.. code-block:: proto

	message CustomModel {
	
	    message CustomModelParamValue {
	        oneof value {
	            double doubleValue = 10;
	            string stringValue = 20;
	            int32 intValue = 30;
	            int64 longValue = 40;
	            bool boolValue = 50;
	            bytes bytesValue = 60;
	        }
	    }
	
	    string className = 10; // The name of the class (conforming to MLCustomModel) corresponding to this model
	    map<string, CustomModelParamValue> parameters = 30;
	    string description = 40; // An (optional) description provided by the model creator. This information is displayed when viewing the model, but does not affect the model's execution on device.
	}






CustomModel.CustomModelParamValue
--------------------------------------------------------------------------------




.. code-block:: proto

	    message CustomModelParamValue {
	        oneof value {
	            double doubleValue = 10;
	            string stringValue = 20;
	            int32 intValue = 30;
	            int64 longValue = 40;
	            bool boolValue = 50;
	            bytes bytesValue = 60;
	        }
	    }






CustomModel.ParametersEntry
--------------------------------------------------------------------------------




.. code-block:: proto

	    message CustomModelParamValue {
	        oneof value {
	            double doubleValue = 10;
	            string stringValue = 20;
	            int32 intValue = 30;
	            int64 longValue = 40;
	            bool boolValue = 50;
	            bytes bytesValue = 60;
	        }
	    }