#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil.mil import get_new_symbol
from ._op_reqs import *


@register_op(doc_str="TODO")
class gru(Operation):
    input_spec = InputSpec(
        x=TensorInputType(),
        initial_h=TensorInputType(),
        weight=TensorInputType(const=True),
        bias=TensorInputType(const=True, optional=True, default=None),
        direction=StringInputType(const=True, default="forward"),
        output_sequence=BoolInputType(const=True, default=False),
        activations=TupleInputType(const=True, default=("sigmoid", "tanh")),
    )

    def __init__(self, **kwargs):
        super(gru, self).__init__(**kwargs)

    def type_inference(self):
        if self.x.rank != 3:
            raise ValueError(
                "Invalid input shape. Expecting Rank 3 input, got {}".format(
                    len(self.x.shape)
                )
            )

        sequence_length, batch_size, input_size = self.x.shape

        if self.weight.rank != 2:
            raise ValueError(
                "Invalid weight shape. Expecting Rank 2 input, got {}".format(
                    len(self.weight.shape)
                )
            )

        input_hidden_size, hidden_dim = self.weight.shape
        hidden_size = input_hidden_size - input_size

        direction = self.direction.val
        valid_directions = {"forward", "reverse"}
        if direction not in valid_directions:
            raise ValueError(
                "Direction {} not supported. Supported directions: {}".format(
                    direction, valid_directions
                )
            )

        dim_factor = 3
        if hidden_size != (hidden_dim // dim_factor):
            raise ValueError(
                "Incorrect weight matrix: hidden dim size mismatch. \
                              Provided  {}. Expecting <b, 3*H>".format(
                    self.weight.shape
                )
            )

        out_seq_len = sequence_length if self.output_sequence.val else 1
        output_shape = [out_seq_len, batch_size, hidden_size]
        output_h_shape = [batch_size, hidden_size]
        return (
            types.tensor(self.x.dtype, tuple(output_shape)),
            types.tensor(self.x.dtype, tuple(output_h_shape)),
        )


@register_op(doc_str="TODO")
class lstm(Operation):
    input_spec = InputSpec(
        x=TensorInputType(),
        initial_h=TensorInputType(),
        initial_c=TensorInputType(),
        weight=TensorInputType(const=True),  # ifoz layout
        bias=TensorInputType(const=True, optional=True, default=None),  # ifoz layout
        direction=StringInputType(const=True, default="forward"),
        output_sequence=BoolInputType(const=True, default=False),
        activations=TupleInputType(const=True, default=("sigmoid", "tanh", "tanh")),
        peephole=TensorInputType(const=True, optional=True, default=None),  # ifo layout
        clip=FloatInputType(const=True, optional=True, default=None),
    )

    def __init__(self, **kwargs):
        super(lstm, self).__init__(**kwargs)

    def type_inference(self):
        if self.x.rank != 3:
            raise ValueError(
                "Invalid input shape. Expecting Rank 3 input, got {}".format(
                    len(self.x.shape)
                )
            )

        sequence_length, batch_size, input_size = self.x.shape

        if self.weight.rank != 2:
            raise ValueError(
                "Invalid weight shape. Expecting Rank 2 input, got {}".format(
                    len(self.weight.shape)
                )
            )

        input_hidden_size, hidden_dim = self.weight.shape
        hidden_size = input_hidden_size - input_size

        direction = self.direction.val
        valid_directions = {"forward", "reverse", "bidirectional"}
        if direction not in valid_directions:
            raise ValueError(
                "Direction {} not supported. Supported directions: {}".format(
                    direction, valid_directions
                )
            )

        dim_factor = 8 if direction == "bidirectional" else 4
        if hidden_size != (hidden_dim // dim_factor):
            raise ValueError(
                "Incorrect weight matrix: hidden dim size mismatch. \
                              Provided  {}. Expecting <b, 4*DIRECTION*H>".format(
                    self.weight.shape
                )
            )

        out_seq_len = sequence_length if self.output_sequence.val else 1
        num_directions = dim_factor // 4
        output_shape = [out_seq_len, batch_size, num_directions * hidden_size]
        output_h_shape = [batch_size, num_directions * hidden_size]
        output_c_shape = [batch_size, num_directions * hidden_size]
        return (
            types.tensor(self.x.dtype, tuple(output_shape)),
            types.tensor(self.x.dtype, tuple(output_h_shape)),
            types.tensor(self.x.dtype, tuple(output_c_shape)),
        )


@register_op(doc_str="TODO")
class rnn(Operation):
    input_spec = InputSpec(
        x=TensorInputType(),
        initial_h=TensorInputType(),
        weight=TensorInputType(const=True),
        bias=TensorInputType(const=True, optional=True, default=None),
        direction=StringInputType(const=True, default="forward"),
        output_sequence=BoolInputType(const=True, default=False),
        activation=StringInputType(const=True, default="tanh"),
    )

    def __init__(self, **kwargs):
        super(rnn, self).__init__(**kwargs)

    def type_inference(self):
        if self.x.rank != 3:
            raise ValueError(
                "Invalid input shape. Expecting Rank 3 input, got {}".format(
                    len(self.x.shape)
                )
            )

        sequence_length, batch_size, input_size = self.x.shape

        if self.weight.rank != 2:
            raise ValueError(
                "Invalid weight shape. Expecting Rank 2 input, got {}".format(
                    len(self.weight.shape)
                )
            )

        _, hidden_size = self.weight.shape

        direction = self.direction.val
        valid_directions = {"forward", "reverse"}
        if direction not in valid_directions:
            raise ValueError(
                "Direction {} not supported. Supported directions: {}".format(
                    direction, valid_directions
                )
            )

        out_seq_len = sequence_length if self.output_sequence.val else 1
        output_shape = [out_seq_len, batch_size, hidden_size]
        output_h_shape = [batch_size, hidden_size]
        return (
            types.tensor(self.x.dtype, tuple(output_shape)),
            types.tensor(self.x.dtype, tuple(output_h_shape)),
        )
