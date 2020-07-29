Scaler
________________________________________________________________________________

A scaling operation.

This function has the following formula:

.. math::
    f(x) = scaleValue \cdot (x + shiftValue)

If the ``scaleValue`` is not given, the default value 1 is used.
If the ``shiftValue`` is not given, the default value 0 is used.

If ``scaleValue`` and ``shiftValue`` are each a single value
and the input is an array, then the scale and shift are applied
to each element of the array.

If the input is an integer, then it is converted to a double to
perform the scaling operation. If the output type is an integer,
then it is cast to an integer. If that cast is lossy, then an
error is generated.


.. code-block:: proto

	message Scaler {
	    repeated double shiftValue = 1;
	    repeated double scaleValue = 2;
	}