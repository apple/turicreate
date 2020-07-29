Normalizer
________________________________________________________________________________

A normalization preprocessor.


.. code-block:: proto

	message Normalizer {
	    enum NormType {
	        LMax = 0;
	        L1 = 1;
	        L2 = 2;
	    }
	
	    NormType normType = 1;
	}










Normalizer.NormType
--------------------------------------------------------------------------------

There are three normalization modes,
which have the corresponding formulas:

Max
    .. math::
        max(x_i)

L1
    .. math::
        z = ||x||_1 = \sum_{i=1}^{n} |x_i|

L2
    .. math::
        z = ||x||_2 = \sqrt{\sum_{i=1}^{n} x_i^2}

.. code-block:: proto

	    enum NormType {
	        LMax = 0;
	        L1 = 1;
	        L2 = 2;
	    }