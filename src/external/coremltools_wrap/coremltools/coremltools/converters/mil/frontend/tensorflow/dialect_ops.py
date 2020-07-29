#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.mil import types
from coremltools.converters.mil.mil import Operation
from coremltools.converters.mil.mil.input_type import *
from coremltools.converters.mil.mil.ops.registry import SSAOpRegistry

register_op = SSAOpRegistry.register_op


# This file contains the TF dialect of SSA. Briefly, these ops are only
# understandable in the TF frontend and not acceptable in the standard op set.
# No backend would support any of the op here. These ops exist to facilitate
# frontend SSA passes, but must be replaced with standard ops during SSA
# passes.

# All tf op must start with 'tf_' prefix.
#
# tf_make_list allows elem_shape to be unspecified. core op make_list does
# not allow that.
@register_op(doc_str="TODO", namespace="tf")
class tf_make_list(Operation):
    input_spec = InputSpec(
        init_length=IntInputType(optional=True, default=1),
        dynamic_length=BoolInputType(optional=True, default=True),
        elem_shape=TensorInputType(const=True, optional=True),
        dtype=StringInputType(const=True, optional=True, default="fp32"),
    )

    def __init__(self, **kwargs):
        super(tf_make_list, self).__init__(**kwargs)

    def type_inference(self):
        init_length = self.init_length.val
        if self.elem_shape is None or self.elem_shape.sym_val is None:
            return types.list(
                types.unknown,
                init_length=init_length,
                dynamic_length=self.dynamic_length.val,
            )
        builtin_dtype = types.string_to_builtin(self.dtype.val)
        if builtin_dtype is None:
            raise ValueError("Unsupported dtype {}".format(self.dtype.val))
        elem_type = types.tensor(builtin_dtype, self.elem_shape.sym_val)
        return types.list(
            elem_type, init_length=init_length, dynamic_length=self.dynamic_length.val
        )


class TfLSTMBase(Operation):
    """
    Common LSTM inputs for BlockLSTMCell and BlockLSTM.
    """

    input_spec = InputSpec(
        c_prev=TensorInputType(),  # [batch, hidden_dim]
        h_prev=TensorInputType(),  # [batch, hidden_dim]
        # weight: [input_dim + hidden_dim, 4*hidden_dim] (icfo layout)
        weight=TensorInputType(const=True),
        forget_bias=FloatInputType(const=True, optional=True, default=1.0),
        # cell_clip == None implies not using cell clip
        cell_clip=FloatInputType(const=True, optional=True),
        # If use_peephole == False, weight_peep_* is ignored
        use_peephole=BoolInputType(const=True, optional=True, default=False),
        weight_peep_i=TensorInputType(const=True, optional=True),  # [hidden_dim,]
        weight_peep_f=TensorInputType(const=True, optional=True),  # [hidden_dim,]
        weight_peep_o=TensorInputType(const=True, optional=True),  # [hidden_dim,]
        bias=TensorInputType(const=True),  # [4*hidden_dim] (icfo layout)
    )

    def _check_peephole_weights(self):
        # Check weight_peep_*
        if self.use_peephole.val:
            if (
                self.weight_peep_i is None
                or self.weight_peep_f is None
                or self.weight_peep_o is None
            ):
                raise ValueError(
                    "weight_peep_* cannot be None when use_peephole is True"
                )


@register_op(
    doc_str="""
                     xh = [x, h_prev]
                     [i, ci, f, o] = xh * w + b
                     f = f + forget_bias
                     if not use_peephole:
                       wci = wcf = wco = 0
                     i = sigmoid(cs_prev .* wci + i)
                     f = sigmoid(cs_prev .* wcf + f)
                     ci = tanh(ci)
                     cs = ci .* i + cs_prev .* f
                     cs = clip(cs, cell_clip)
                     o = sigmoid(cs * wco + o)
                     co = tanh(cs)
                     h = co .* o
                     """,
    namespace="tf",
)
class tf_lstm_block_cell(TfLSTMBase):
    input_spec = (
        InputSpec(x=TensorInputType(),) + TfLSTMBase.input_spec  # [batch, input_dim]
    )

    def __init__(self, **kwargs):
        super(tf_lstm_block_cell, self).__init__(**kwargs)

    def type_inference(self):
        self._check_peephole_weights()
        # all return shapes are [batch, hidden_dim]
        ret_shape = self.c_prev.shape
        dtype = self.x.dtype
        # See
        # https://www.tensorflow.org/api_docs/python/tf/raw_ops/LSTMBlockCell
        # All returned shapes are [batch, hidden_dim]
        return (
            types.tensor(dtype, ret_shape),  # i
            types.tensor(dtype, ret_shape),  # cs
            types.tensor(dtype, ret_shape),  # f
            types.tensor(dtype, ret_shape),  # o
            types.tensor(dtype, ret_shape),  # ci
            types.tensor(dtype, ret_shape),  # co
            types.tensor(dtype, ret_shape),
        )  # h


@register_op(
    doc_str="""
                     Apply LSTM to an input sequence
                     """,
    namespace="tf",
)
class tf_lstm_block(TfLSTMBase):
    input_spec = (
        InputSpec(
            seq_len=IntInputType(),  # int
            x=TensorInputType(),  # [padded_len, batch, input_dim]
        )
        + TfLSTMBase.input_spec
    )

    def __init__(self, **kwargs):
        super(tf_lstm_block, self).__init__(**kwargs)

    def type_inference(self):
        self._check_peephole_weights()
        padded_len = self.x.shape[0]
        ret_shape = [padded_len] + list(self.c_prev.shape)
        dtype = self.x.dtype
        # All returned shapes are [padded_len, b, hidden_dim]
        return (
            types.tensor(dtype, ret_shape),  # i
            types.tensor(dtype, ret_shape),  # cs
            types.tensor(dtype, ret_shape),  # f
            types.tensor(dtype, ret_shape),  # o
            types.tensor(dtype, ret_shape),  # ci
            types.tensor(dtype, ret_shape),  # co
            types.tensor(dtype, ret_shape),
        )  # h
