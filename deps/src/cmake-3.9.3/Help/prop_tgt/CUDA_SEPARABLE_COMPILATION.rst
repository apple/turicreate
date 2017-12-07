CUDA_SEPARABLE_COMPILATION
--------------------------

CUDA only: Enables separate compilation of device code

If set this will enable separable compilation for all CUDA files for
the given target.

For instance:

.. code-block:: cmake

  set_property(TARGET myexe PROPERTY CUDA_SEPARABLE_COMPILATION ON)
