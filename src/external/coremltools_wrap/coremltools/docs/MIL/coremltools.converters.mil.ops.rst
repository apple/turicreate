**********
MIL Ops
**********

The list of operators supported by MIL.


.. automodule:: coremltools.converters.mil.mil.ops.defs.activation

  .. autoclass:: clamped_relu
  .. autoclass:: elu
  .. autoclass:: gelu
  .. autoclass:: leaky_relu
  .. autoclass:: linear_activation
  .. autoclass:: prelu
  .. autoclass:: relu
  .. autoclass:: relu6
  .. autoclass:: sigmoid
  .. autoclass:: sigmoid_hard
  .. autoclass:: softplus
  .. autoclass:: softplus_parametric
  .. autoclass:: softsign
  .. autoclass:: thresholded_relu

.. automodule:: coremltools.converters.mil.mil.ops.defs.control_flow

  .. autoclass:: cond
  .. autoclass:: const
  .. autoclass:: select
  .. autoclass:: while_loop
  .. autoclass:: identity
  .. autoclass:: make_list
  .. autoclass:: list_length
  .. autoclass:: list_write
  .. autoclass:: list_read
  .. autoclass:: list_gather
  .. autoclass:: list_scatter

.. automodule:: coremltools.converters.mil.mil.ops.defs.conv

  .. autoclass:: conv
  .. autoclass:: conv_transpose

.. automodule:: coremltools.converters.mil.mil.ops.defs.elementwise_binary
  
  .. autoclass:: add
  .. autoclass:: equal
  .. autoclass:: floor_div
  .. autoclass:: greater
  .. autoclass:: greater_equal
  .. autoclass:: less
  .. autoclass:: less_equal
  .. autoclass:: logical_and
  .. autoclass:: logical_or
  .. autoclass:: logical_xor
  .. autoclass:: maximum
  .. autoclass:: minimum
  .. autoclass:: mul
  .. autoclass:: not_equal
  .. autoclass:: real_div
  .. autoclass:: pow
  .. autoclass:: sub

.. automodule:: coremltools.converters.mil.mil.ops.defs.elementwise_unary

  .. autoclass:: abs
  .. autoclass:: acos
  .. autoclass:: asin
  .. autoclass:: atan
  .. autoclass:: atanh
  .. autoclass:: ceil
  .. autoclass:: clip
  .. autoclass:: cos
  .. autoclass:: cosh
  .. autoclass:: erf
  .. autoclass:: exp
  .. autoclass:: exp2
  .. autoclass:: floor
  .. autoclass:: inverse
  .. autoclass:: log
  .. autoclass:: logical_not
  .. autoclass:: round
  .. autoclass:: rsqrt
  .. autoclass:: sign
  .. autoclass:: sin
  .. autoclass:: sinh
  .. autoclass:: sqrt
  .. autoclass:: square
  .. autoclass:: tan
  .. autoclass:: tanh
  .. autoclass:: threshold
  .. autoclass:: cast

.. automodule:: coremltools.converters.mil.mil.ops.defs.image_resizing

  .. autoclass:: upsample_nearest_neighbor
  .. autoclass:: upsample_bilinear
  .. autoclass:: resize_bilinear
  .. autoclass:: crop_resize
  .. autoclass:: crop
  
.. automodule:: coremltools.converters.mil.mil.ops.defs.linear

  .. autoclass:: linear
  .. autoclass:: matmul

.. automodule:: coremltools.converters.mil.mil.ops.defs.normalization

  .. autoclass:: batch_norm
  .. autoclass:: instance_norm
  .. autoclass:: l2_norm
  .. autoclass:: local_response_norm

.. automodule:: coremltools.converters.mil.mil.ops.defs.pool

  .. autoclass:: avg_pool
  .. autoclass:: l2_pool
  .. autoclass:: max_pool
  
.. automodule:: coremltools.converters.mil.mil.ops.defs.random

  .. autoclass:: random_bernoulli
  .. autoclass:: random_categorical
  .. autoclass:: random_normal
  .. autoclass:: random_uniform

.. automodule:: coremltools.converters.mil.mil.ops.defs.recurrent

  .. autoclass:: gru
  .. autoclass:: lstm
  .. autoclass:: rnn

.. automodule:: coremltools.converters.mil.mil.ops.defs.reduction

  .. autoclass:: reduce_arg
  .. autoclass:: reduce_argmax
  .. autoclass:: reduce_argmin
  .. autoclass:: reduce_l2_norm
  .. autoclass:: reduce_log_sum
  .. autoclass:: reduce_max
  .. autoclass:: reduce_mean
  .. autoclass:: reduce_min
  .. autoclass:: reduce_prod
  .. autoclass:: reduce_sum
  .. autoclass:: reduce_sum_square
  
.. automodule:: coremltools.converters.mil.mil.ops.defs.scatter_gather

  .. autoclass:: gather
  .. autoclass:: scatter
  .. autoclass:: gather_along_axis
  .. autoclass:: scatter_along_axis
  .. autoclass:: gather_nd
  .. autoclass:: scatter_nd

.. automodule:: coremltools.converters.mil.mil.ops.defs.slicend

  .. autoclass:: slice_by_index
  
.. automodule:: coremltools.converters.mil.mil.ops.defs.tensor_operation

  .. autoclass:: band_part
  .. autoclass:: cumsum
  .. autoclass:: fill
  .. autoclass:: non_maximum_suppression
  .. autoclass:: non_zero
  .. autoclass:: one_hot
  .. autoclass:: pad
  .. autoclass:: range_1d
  .. autoclass:: tile
  .. autoclass:: argsort
  .. autoclass:: topk
  .. autoclass:: flatten
  .. autoclass:: shape
  .. autoclass:: concat
  .. autoclass:: split
  .. autoclass:: stack
  .. autoclass:: addn

.. automodule:: coremltools.converters.mil.mil.ops.defs.tensor_transformation

  .. autoclass:: depth_to_space
  .. autoclass:: expand_dims
  .. autoclass:: reshape
  .. autoclass:: reverse
  .. autoclass:: reverse_sequence
  .. autoclass:: slice_by_size
  .. autoclass:: space_to_depth
  .. autoclass:: squeeze
  .. autoclass:: transpose
  .. autoclass:: pixel_shuffle
  .. autoclass:: sliding_windows
