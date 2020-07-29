KNearestNeighborsClassifier
________________________________________________________________________________

A k-Nearest-Neighbor classifier


.. code-block:: proto

	message KNearestNeighborsClassifier {
	
	    NearestNeighborsIndex nearestNeighborsIndex = 1;
	
	    Int64Parameter numberOfNeighbors = 3;
	
	    oneof ClassLabels {
	        StringVector stringClassLabels = 100;
	        Int64Vector int64ClassLabels = 101;
	    }
	
	    oneof DefaultClassLabel {
	        string defaultStringLabel = 110;
	        int64 defaultInt64Label = 111;
	    }
	
	    oneof WeightingScheme {
	        UniformWeighting uniformWeighting = 200;
	        InverseDistanceWeighting inverseDistanceWeighting = 210;
	    }
	}






NearestNeighborsIndex
________________________________________________________________________________

The "core" attributes of a Nearest Neighbors model.


.. code-block:: proto

	message NearestNeighborsIndex {
	
	    int32 numberOfDimensions = 1;
	
	    repeated FloatVector floatSamples = 2;
	
	    oneof IndexType {
	        LinearIndex linearIndex = 100;
	        SingleKdTreeIndex singleKdTreeIndex = 110;
	    }
	
	    oneof DistanceFunction {
	        SquaredEuclideanDistance squaredEuclideanDistance = 200;
	    }
	
	}






UniformWeighting
________________________________________________________________________________

Specifies a uniform weighting scheme (i.e. each neighbor receives equal
voting power).


.. code-block:: proto

	message UniformWeighting {
	}






InverseDistanceWeighting
________________________________________________________________________________

Specifies a inverse-distance weighting scheme (i.e. closest neighbors receives higher
voting power). A nearest neighbor with highest sum of (1 / distance) is picked.


.. code-block:: proto

	message InverseDistanceWeighting {
	}






LinearIndex
________________________________________________________________________________

Specifies a flat index of data points to be searched by brute force.


.. code-block:: proto

	message LinearIndex {
	}






SingleKdTreeIndex
________________________________________________________________________________

Specifies a kd-tree backend for the nearest neighbors model.


.. code-block:: proto

	message SingleKdTreeIndex {
	
	    int32 leafSize = 1;
	
	}






SquaredEuclideanDistance
________________________________________________________________________________

Specifies the Squared Euclidean Distance function.


.. code-block:: proto

	message SquaredEuclideanDistance {
	}