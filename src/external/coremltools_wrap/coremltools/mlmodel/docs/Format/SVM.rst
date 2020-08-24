LinearKernel
________________________________________________________________________________

A linear kernel.

This function has the following formula:

.. math::
    K(\boldsymbol{x}, \boldsymbol{x'}) = \boldsymbol{x}^T \boldsymbol{x'}


.. code-block:: proto

	message LinearKernel {
	}






RBFKernel
________________________________________________________________________________

A Gaussian radial basis function (RBF) kernel.

This function has the following formula:

.. math::
    K(\boldsymbol{x}, \boldsymbol{x'}) = \
         \exp(-\gamma || \boldsymbol{x} - \boldsymbol{x'} ||^2 )


.. code-block:: proto

	message RBFKernel {
	    double gamma = 1;
	}






PolyKernel
________________________________________________________________________________

A polynomial kernel.

This function has the following formula:

.. math::
    K(\boldsymbol{x}, \boldsymbol{x'}) = \
          (\gamma \boldsymbol{x}^T \boldsymbol{x'} + c)^{degree}


.. code-block:: proto

	message PolyKernel {
	    int32 degree = 1;
	    double c = 2;
	    double gamma = 3;
	}






SigmoidKernel
________________________________________________________________________________

A sigmoid kernel.

This function has the following formula:

.. math::
    K(\boldsymbol{x}, \boldsymbol{x'}) = \
          \tanh(\gamma \boldsymbol{x}^T \boldsymbol{x'} + c)


.. code-block:: proto

	message SigmoidKernel {
	    double gamma = 1;
	    double c = 2;
	}






Kernel
________________________________________________________________________________

A kernel.


.. code-block:: proto

	message Kernel {
	    oneof kernel {
	        LinearKernel linearKernel = 1;
	        RBFKernel rbfKernel = 2;
	        PolyKernel polyKernel = 3;
	        SigmoidKernel sigmoidKernel = 4;
	    }
	}






SparseNode
________________________________________________________________________________

A sparse node.


.. code-block:: proto

	message SparseNode {
	    int32 index = 1; // 1-based indexes, like libsvm
	    double value = 2;
	}






SparseVector
________________________________________________________________________________

A sparse vector.


.. code-block:: proto

	message SparseVector {
	    repeated SparseNode nodes = 1;
	}






SparseSupportVectors
________________________________________________________________________________

One or more sparse support vectors.


.. code-block:: proto

	message SparseSupportVectors {
	    repeated SparseVector vectors = 1;
	}






DenseVector
________________________________________________________________________________

A dense vector.


.. code-block:: proto

	message DenseVector {
	    repeated double values = 1;
	}






DenseSupportVectors
________________________________________________________________________________

One or more dense support vectors.


.. code-block:: proto

	message DenseSupportVectors {
	    repeated DenseVector vectors = 1;
	}






Coefficients
________________________________________________________________________________

One or more coefficients.


.. code-block:: proto

	message Coefficients {
	    repeated double alpha = 1;
	}






SupportVectorRegressor
________________________________________________________________________________

A support vector regressor.


.. code-block:: proto

	message SupportVectorRegressor {
	    Kernel kernel = 1;
	
	    // Support vectors, either sparse or dense format
	    oneof supportVectors {
	        SparseSupportVectors sparseSupportVectors = 2;
	        DenseSupportVectors denseSupportVectors = 3;
	    }
	
	    // Coefficients, one for each support vector
	    Coefficients coefficients = 4;
	
	    double rho = 5;
	}






SupportVectorClassifier
________________________________________________________________________________

A support vector classifier


.. code-block:: proto

	message SupportVectorClassifier {
	    Kernel kernel = 1;
	
	    repeated int32 numberOfSupportVectorsPerClass = 2;
	
	    oneof supportVectors {
	        SparseSupportVectors sparseSupportVectors = 3;
	        DenseSupportVectors denseSupportVectors = 4;
	    }
	
	    repeated Coefficients coefficients = 5;
	
	    repeated double rho = 6;
	
	    repeated double probA = 7;
	    repeated double probB = 8;
	
	    oneof ClassLabels {
	        StringVector stringClassLabels = 100;
	        Int64Vector int64ClassLabels = 101;
	    }
	}