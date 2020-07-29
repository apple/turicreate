#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from collections import Counter, OrderedDict
import copy
import logging
import numpy as _np
from six import string_types as _string_types
from . import SPACES, types
from .var import Var, InternalVar, ListVar
from .visitors.dot_visitor import DotVisitor
from .types.symbolic import (
    k_used_symbols,
    k_num_internal_syms,
    any_symbolic,
    is_symbolic,
)
from .input_type import TupleInputType

# BLOCK_STACK[-1] is the current block
BLOCK_STACK = []
DEBUG = False

def curr_block():
    if len(BLOCK_STACK) == 0:
        raise ValueError("Must call Builder inside an Function" + " or Block")
    return BLOCK_STACK[-1]


def _check_is_compatible_type(type1, type2):
    if not types.is_subtype(type1, type2):
        is_comp, _ = types.is_tensor_and_is_compatible(type1, type2)
        return is_comp
    return True


class InvalidBlockStateError(Exception):
    pass


class Block(object):
    __slots__ = [
        "name",
        "_block_inputs",
        "_outputs",
        "operations",
        "_internal_vars",
        "outer_op",
    ]

    counter = 0

    @classmethod
    def _get_new_name(cls):
        curr_val = cls.counter
        cls.counter += 1
        return "block" + str(curr_val)

    def __init__(self, block_inputs=None, outer_op=None, name=None):
        """
        Inputs:

        block_inputs: python tuple[Var].

            block_inputs is None except when the block represents loop. By
            convention block_inputs should have name ending in '.x', and the
            Variable are not produced by any op (block_inputs[i]._op is None).

            Ex:

            #    main(%a: (1, 2, fp32),
            #         %b: (1, 2, fp32),
            #         %c: (1, 2, fp32)) {
            #      block0() {
            #        %const1: (1, fp32) = const(...)
            #        %loop:0: (1, 2, fp32), %loop:1: (1, 2, fp32) = \
            #        while_loop(loop_vars=(%a, %b))
            #          loop_cond(%a.x, %b.x) {
            #            %blah: (bool) = some_op(x=%a.x, y=%b.x)
            #            %cond_var: (bool) = some_op2(x=%a.x, y=%blah)
            #          } -> (%cond_var)
            #          loop_body(%a.x, %b.x) {
            #            %add_0: (1, 2, fp32) = add(x=%a.x, y=%b.x)
            #          } -> (%add_0, %b.x)
            #        %linear: (1, fp32) = linear(...)
            #      } -> (%loop:0, %loop:1)
            #    }

            %a.x, %b.x are block_inputs.

            `some_op` in `loop_cond` block can access %a, %b, %a.x, %b.x.
            `some_op`, however, cannot take %linear as input.

        outer_op: Operation
            The enclosing op. None iff this Block is an Function.

        function_inputs: tuple[Var]
            function_inputs are always visible for this block and all blocks
            nested within. If function_inputs is None, get it from
            `outer_op.block`
        """
        self.name = name
        if self.name is None:
            self.name = Block._get_new_name()

        # list[Operation]. Topologically sorted.
        self.operations = []

        # Must be set before self.validate()
        self.outer_op = outer_op


        self._block_inputs = block_inputs
        if self._block_inputs is None:
            self._block_inputs = tuple()

        # list[Var]. This is converted to str when generating MIL proto.
        self._outputs = []

        # If we create const, whose inputs (mode, val) cannot be const
        # (infinite recursion). They must be considered as always visible.
        self._internal_vars = set()

        if self.outer_op is None and not isinstance(self, Function):
            msg = "Block {} is not Function and thus outer_op cannot be None"
            raise ValueError(msg.format(self.name))

        self.validate()

    def validate(self):
        """
        Basic validation to protect against some invalid state.
        """
        if not DEBUG:
            return

        for op in self.operations:
            for b in op.blocks:
                b.validate()
            if op.outputs is None:
                raise InvalidBlockStateError()

            # Check the input output relationships
            # from outputs -> inputs
            for ov in op.outputs:
                child_op_count = Counter(ov.child_ops)
                for next_op, c in child_op_count.items():
                    c_actual = next_op.get_flattened_inputs().count(ov)
                    if c_actual != c:
                        msg = (
                            "Var {} should be consumed by op {} {}"
                            + " times, but op {} uses it {} times.\n{}"
                        )
                        raise InvalidBlockStateError(
                            msg.format(
                                ov.name,
                                next_op.name,
                                c,
                                next_op.name,
                                c_actual,
                                next_op,
                            )
                        )

            # from inputs -> outputs
            input_var_count = Counter(op.get_flattened_inputs())
            for iv, c in input_var_count.items():
                c_actual = iv.child_ops.count(op)
                if c_actual != c:
                    msg = (
                        "Var {} should be consumed by op {} {}"
                        + " times, but op {} uses it {} times.\n{}"
                    )
                    raise InvalidBlockStateError(
                        msg.format(iv.name, op.name, c_actual, op.name, c, op)
                    )

        # 1 to 1 mapping between Block outputs and Var.consuming_blocks
        for op in self.operations:
            for ov in op.outputs:
                for b in ov.consuming_blocks:
                    if ov not in b.outputs:
                        msg = "Var {} should be output of block {}: {}"
                        raise ValueError(msg.format(ov.name, b.name, b))

        for v in self.outputs:
            if self not in v.consuming_blocks:
                msg = "Var {} should be output of block {}: {}"
                raise ValueError(msg.format(ov.name, b.name, b))

    def remove_inputs(self, curr_input_vars):
        """
        curr_input_vars: list[Var], whose elements must be in
        self._block_inputs.
        """
        self.validate()
        remove_idx = [self._block_inputs.index(v) for v in curr_input_vars]
        self._block_inputs = [
            v for i, v in enumerate(self._block_inputs) if i not in remove_idx
        ]

    def find_ops(self, prefix=None, op_type=None):
        """
        Return list of ops with name matching `prefix` if specified and
        op_type, if specified. At least one of {prefix, op_type} must be specified.

        prefix: str

        Return list[Operation]. Empty list if no op satisfies.
        """
        if prefix is None and op_type is None:
            raise ValueError("Must specify one of {prefix, op_type}")
        found_ops = []
        for op in self.operations:
            prefix_match = prefix is None or op.name[: len(prefix)] == prefix
            op_type_match = op_type is None or op.op_type == op_type
            if prefix_match and op_type_match:
                found_ops.append(op)
            for b in op.blocks:
                found_ops.extend(b.find_ops(prefix=prefix, op_type=op_type))
        return found_ops

    def add_internal_var(self, internal_var):
        if not isinstance(internal_var, InternalVar):
            raise ValueError("Only InternalVar can be manually added to Block.")
        self._internal_vars.add(internal_var)

    @property
    def inputs(self):
        return self._block_inputs

    @property
    def outputs(self):
        return self._outputs

    def set_outputs(self, outputs):
        """
        outputs: list[Var]
        """
        if not isinstance(outputs, list):
            raise ValueError("Outputs must be list of Vars")

        self.validate()
        visible_vars = self._visible_vars_from_enclosing_block()
        _, visible_vars_in_block = self._visible_vars_in_block()
        visible_vars.update(visible_vars_in_block)
        for ov in outputs:
            if ov not in visible_vars:
                msg = (
                    "Var {} is not visible in block {} and thus cannot "
                    + "be a block output.\n{}"
                )
                raise ValueError(msg.format(ov.name, self.name, self))

        for ov in self._outputs:
            ov.consuming_blocks.remove(self)

        # Need to copy, or block's output would be completely tied to a var's
        # output and we cannot replace a block output with another var's
        # output.
        self._outputs = copy.copy(outputs)
        for ov in outputs:
            ov.consuming_blocks.append(self)

    def __enter__(self):
        global BLOCK_STACK
        BLOCK_STACK.append(self)
        return self

    def __exit__(self, type, value, traceback):
        global BLOCK_STACK
        BLOCK_STACK = BLOCK_STACK[:-1]

    def _visible_vars_in_block(self, target_op=None, inclusive=True):
        """
        Returns:

        index (int) of target_op in self.operations if target_op not None,
        undefined otherwise.

        Raises:

        ValueError if target_op not None and not found in self.operations.

        visible_vars: set[Var]
            Vars returned by ops in the block (self) visible (and equal to
            if inclusive==True) target_op.  If target_op is not found or None,
            include all vars output by self.operations. Examples:

        #    main(%a: (1, 2, fp32),
        #         %b: (1, 2, fp32),
        #         %c: (1, 2, fp32)) {
        #      block0() {
        #        %const1: (1, fp32) = const(...)
        #        %loop:0: (1, 2, fp32), %loop:1: (1, 2, fp32) = \
        #        while_loop(loop_vars=(%a, %b))
        #          loop_cond(%a.x, %b.x) {
        #            %blah: (bool) = some_op(x=%a.x, y=%b.x)
        #            %cond_var: (bool) = some_op2(x=%a.x, y=%blah)
        #          } -> (%cond_var)
        #          loop_body(%a.x, %b.x) {
        #            %add_0: (1, 2, fp32) = add(x=%a.x, y=%b.x)
        #          } -> (%add_0, %b.x)
        #        %linear: (1, fp32) = linear(...)
        #      } -> (%loop:0, %loop:1)
        #    }
        #

        Let V0 and V1 be the set of internal_vars of block0 and loop_cond
        block that supplies const vals (for const).

        Ex1: self = block0, target_op = linear.
        idx = 2
        visible_vars = {%const1, %loop:0, %loop:1, %linear, V0}

        Ex2: self = loop_cond, target_op = None.
        idx = undefined
        visible_vars = {%a.x, %b.x, %blah, %cond_var, V1}

        Ex3: self = loop_cond, target_op = some_op.
        idx = 0
        visible_vars = {%a.x, %b.x, %blah, V1}

        Ex4: self = loop_cond, target_op = linear.
        raises ValueError (linear not found in loop_cond block)
        """
        visible_vars = set(self._internal_vars)
        visible_vars.update(self.inputs)
        idx = -1
        # find the location of target_op
        for i, op in enumerate(self.operations):
            if op == target_op:
                if inclusive:
                    visible_vars.update(op.outputs)
                return i, visible_vars
            visible_vars.update(op.outputs)
        if target_op is not None:
            msg = "Op {} not found in {}: {}"
            raise ValueError(msg.format(target_op.name, self.name, self))
        return idx, visible_vars

    def _visible_vars_from_enclosing_block(self):
        """
        Returns:

        visible_vars: Vars from lexical scopes visible at the beginning of the
        block, up to but not including outputs from before_op. Given program:

        #    main(%a: (1, 2, fp32),
        #         %b: (1, 2, fp32),
        #         %c: (1, 2, fp32)) {
        #      block0() {
        #        %const1: (1, fp32) = const(...)
        #        %loop:0: (1, 2, fp32), %loop:1: (1, 2, fp32) = \
        #        while_loop(loop_vars=(%a, %b))
        #          loop_cond(%a.x, %b.x) {
        #            %blah: (bool) = some_op(x=%a.x, y=%b.x)
        #            %cond_var: (bool) = some_op2(x=%a.x, y=%blah)
        #          } -> (%cond_var)
        #          loop_body(%a.x, %b.x) {
        #            %add_0: (1, 2, fp32) = add(x=%a.x, y=%b.x)
        #          } -> (%add_0, %b.x)
        #        %const2: (1, fp32) = const(...)
        #      } -> (%loop:0, %loop:1)
        #    }

        Let V0 be the set of internal_vars of block0 block that supplies const
        vals (for const).

        Ex1: self = block0
            visible_vars = {%a, %b, %c} (function input)

        Ex2: self = loop_cond.
            visible_vars = {%a, %b, %c, %const1, V0} (Note that %const2 is not
            part of the set)
        """
        visible_vars = set()

        # function inputs are considered external to the block.
        if isinstance(self, Function):
            # block in function only has function_inputs as from enclosing
            # block (Ex1 above).
            visible_vars.update(self.function_inputs)
            return visible_vars

        if self.outer_op is not None:
            enclosing_block = self.outer_op.enclosing_block
            vars_at_start = enclosing_block._visible_vars_from_enclosing_block()
            visible_vars.update(vars_at_start)
            _, visible_vars_in_block = enclosing_block._visible_vars_in_block(
                self.outer_op, inclusive=False
            )
            visible_vars.update(visible_vars_in_block)

        return visible_vars

    def _insert_op_before(self, new_op, before_op=None):
        """
        A private API used by builder. Please use `builder.YOUR_OP(...,before_op)`.

        new_op's outputs are not used (not input to any other op) after
        this call. All inputs to new_op must be visible at or before
        the before_op (i.e., new_op must be added in topologically sorted
        order). Note that this is more restrictive than MIL, whose Block
        supports lexical scoping and thus an op can reference Var in enclosing
        scopes. new_op.name must be unique in the block.

        before_op=None to append new_op at the end of self.operations.

        Given:   %2 = op0(%1, %1)
                 %4 = op2(%1)
                 %6 = op3(%4, %4)

        Execute: insert_op_before(op1, before_op=op2),
                 where %3 = op1(%1, %2)

        Result:  %2 = op0(%1, %1)
                 %3 = op1(%1, %2)
                 %4 = op2(%1)
                 %6 = op3(%4, %4)

        Comment: We assume op1 has been constructed outside the block with
        %1, %2 as inputs. Typically it's builder's job to create an op and
        insert into the current block.

        Comment: insert_op_before(op1, before_op=op0) would error as %2 (an input to op1)
        is not visible before op0.
        """
        self.validate()
        visible_vars = self._visible_vars_from_enclosing_block()
        if before_op is not None:
            idx, visible_vars_in_block = self._visible_vars_in_block(
                before_op, inclusive=True
            )
            visible_vars.update(visible_vars_in_block)
        else:
            _, visible_vars_in_block = self._visible_vars_in_block()
            visible_vars.update(visible_vars_in_block)

        # check inputs are visible
        for k, v in new_op.inputs.items():
            if not isinstance(v, (Var, tuple)):
                continue
            if isinstance(v, Var):
                vs = [v]
            else:
                vs = v
            for s in vs:
                if s not in visible_vars:
                    before_op_name = before_op.name if before_op is not None else "None"
                    msg = "Op '{}' input {}={} is not in scope of {} before {}"
                    raise ValueError(
                        msg.format(new_op.name, k, s.name, self.name, before_op_name)
                    )

        # add new_op
        if before_op is None:
            self.operations.append(new_op)
        else:
            self.operations.insert(idx, new_op)

    def _replace_var(
        self,
        old_var,
        new_var,
        start=0,
        end_id=-1,
        no_check_var_visibility=False,
        no_check_var_types=False,
    ):
        """Helper function for replace_uses_of_var_after_op"""
        num_ops_affected = 0

        if end_id == -1:
            op_list = self.operations[start:]
        else:
            op_list = self.operations[start : end_id + 1]

        for op in op_list:
            new_inputs = {
                "no_check_var_visibility": no_check_var_visibility,
                "no_check_var_types": no_check_var_types,
            }
            affected = False
            for k, v in op.inputs.items():
                if isinstance(v, (list, tuple)) and old_var in v:
                    new_inputs[k] = tuple(new_var if vv == old_var else vv for vv in v)
                    affected = True
                elif v == old_var:
                    new_inputs[k] = new_var
                    affected = True
                else:
                    new_inputs[k] = v
            if affected:
                num_ops_affected += 1
                op.set_inputs(**new_inputs)

            # Replace recursively.
            for b in op.blocks:
                num_ops_affected += b._replace_var(old_var, new_var)

        if end_id != -1 and old_var.op not in op_list:
            return num_ops_affected

        # If old_var is block's output, replace as well.
        if old_var in self._outputs:
            idx = self._outputs.index(old_var)
            self._outputs[idx] = new_var
            new_var.consuming_blocks.append(self)

            # This block no longer uses `old_var` as its outputs
            old_var.consuming_blocks.remove(self)

            # if rename_new_var_if_fn_output:
            # Ensure output name is consistent
            if isinstance(self, Function):
                new_var.name = old_var.name
        return num_ops_affected

    def replace_uses_of_var_after_op(
        self,
        anchor_op,
        old_var,
        new_var,
        no_check_var_visibility=False,
        end_op=None,
        no_check_var_types=False,
    ):
        """
        Replace all uses of `old_var` with `new_var` after `anchor_op`,
        and before `end_op` (inclusive).

        That is all the ops that use `old_var` will now use `new_var`.
        The op that produces the `old_var` will continue to produce it, its output
        won't be replaced by `new_var`.

        If `anchor_op` is None, replace all input occurrences of `old_var` in the block.
        If `end_op` is None, all occurrences of `old_var` are replaced in the block starting from the op just
        after `anchor_op`

        no_check_var_visibility: True to disable the check ensuring new_var is visible
        (visibility requirement depends on anchor_op).

        no_check_var_types: An error will be raised if the type of new_var is not same as the old_var, unless
        `no_check_var_types` is set to True. Normally type inference is re-invoked for all the child ops of `old_var`
         after updating it to `new_var`. However, this is skipped if `no_check_var_types` is set to True.

        old_var, new_var must meet the following conditions:

        - old_var, new_var both existing within the block. This implies that
          the op generating new_var must be inserted prior to this
          replacement.

        - Affected ops (i.e., Operation after anchor_op that take old_var as
          input) must generate the same type inference results as before.

        - new_var must be visible at or before anchor_op in the order of
          self.operations.

        Given:   %2 = op0(%1, %1)
                 %3 = op1(%1, %2)
                 %4 = op2(%1)
                 %6 = op3(%4, %4)

        Execute: replace_uses_of_var_after_op(op2, %4, %3)

        Result:  %2 = op0(%1, %1)
                 %3 = op1(%1, %2)
                 %4 = op2(%1)
                 %6 = op3(%3, %3)     # type inference check against %6


        Comment: Execute: replace_uses_of_var_after_op(op1, %4, %3) would lead to
        identical results, as op2 does not take %4 as input.

        Comment: replace_uses_of_var_after_op(op0, %4, %3) would cause error as %3 is
        after op0

        Comment: To avoid clutter, we drop the names of arguments and return
        Var in the illustration above.


        Another example, usage of "end_op":

        Given:   %2 = op0(%1, %1)
                 %3 = op1()
                 %4 = op2(%1, %2)
                 %5 = op3(%2)

        if execute replace_uses_of_var_after_op(anchor_op=op0, old_var=%2, new_var=%3)

        Result:  %2 = op0(%1, %1)
                 %3 = op1()
                 %4 = op2(%1, %3)
                 %5 = op3(%3)

        if execute replace_uses_of_var_after_op(anchor_op=op0, old_var=%2, new_var=%3, end_op=op2)

        Result:  %2 = op0(%1, %1)
                 %3 = op1()
                 %4 = op2(%1, %3)           # %2 is replaced with %3 till here
                 %5 = op3(%2)               # will continue using %2

        """
        if not no_check_var_visibility:
            self.validate()
        # Get visible vars from enclosing block
        visible_vars = self._visible_vars_from_enclosing_block()

        if anchor_op is not None:
            # Get visible vars from the current block
            idx, block_vars = self._visible_vars_in_block(anchor_op, inclusive=True)
            visible_vars.update(block_vars)

            # start from the next op, excluding `anchor_op`
            start = idx + 1
        else:
            visible_vars.update(self._block_inputs)
            visible_vars.update(self._internal_vars)
            # Perform replacement from beginning
            start = 0

        if not no_check_var_visibility and new_var not in visible_vars:
            msg = (
                "new_var '{}' is not visible in block '{}' at or before "
                + "anchor_op '{}'"
            )
            anchor_op_name = "None" if anchor_op is None else anchor_op.name
            raise ValueError(msg.format(new_var.name, self.name, anchor_op_name))

        if end_op is not None:
            end_id, _ = self._visible_vars_in_block(end_op, inclusive=True)
        else:
            end_id = -1

        if end_id != -1 and end_id < start:
            msg = "end_op '{}' comes before the anchor_op '{}'"
            raise ValueError(msg.format(end_op.name, anchor_op.name))

        num_ops_affected = self._replace_var(
            old_var,
            new_var,
            start=start,
            end_id=end_id,
            no_check_var_visibility=no_check_var_visibility,
            no_check_var_types=no_check_var_types,
        )

        logging.debug("Num ops affected in replacing var: {}".format(num_ops_affected))

    def remove_ops(self, existing_ops):
        """
        Remove `existing_ops` (list[Operation]) that must be pre-existing in
        the block. Error if any other op in the block uses output Vars of
        `existing_ops`
        """
        self.validate()
        idxs = [-1] * len(existing_ops)
        existing_ops_set = set(existing_ops)
        for i, op in enumerate(self.operations):
            if op in existing_ops_set:
                idxs[existing_ops.index(op)] = i
        if -1 in idxs:
            not_found = []
            for i, op in zip(idxs, existing_ops):
                if i == -1:
                    not_found.append(op.name)
            raise ValueError(
                "Ops {} not found in block {}".format(not_found, self.name)
            )

        # Remove ops in reverse topological order
        pairs = list(zip(idxs, existing_ops))
        pairs.sort(key=lambda x: x[0], reverse=True)

        for idx, op in pairs:
            for i, v in enumerate(op.outputs):
                # Check that no ops depend on op's outputs
                if len(v.child_ops) > 0:
                    child_op_names = [s.name for s in v.child_ops]
                    msg = (
                        "Cannot delete op '{}' with active output at id {}: '{}' "
                        + "used by ops {}"
                    )
                    raise ValueError(msg.format(op.name, i, v.name, child_op_names))
                # Check that the output Var isn't block's output
                if v in self._outputs:
                    msg = (
                        "cannot delete op {} with output {}: {} "
                        + "that's block {}'s output"
                    )
                    raise ValueError(msg.format(op.name, i, v.name, self.name))

            for b in op.blocks:
                b.set_outputs([])
                b.remove_ops(b.operations)

            # remove the op (in reverse topological order)
            self.operations.pop(idx)
            op.enclosing_block = None

            for v in op.inputs.values():
                if isinstance(v, (tuple, list)):
                    for vv in v:
                        vv.remove_child_op(op)
                else:
                    v.remove_child_op(op)

    def operations_for_vars(self, end_vs):
        """
        Inputs:

        end_vs: list[Operation].

        Return:

        list[Operation] which are subset of self.operations that are ancestors
        of `end_vs`. Also do recursion into nested blocks.
        """
        used_vars = set(end_vs)
        used_ops = []
        for op in reversed(self.operations):
            # if none of op's output is used, delete op
            if not set(op.outputs).intersection(used_vars):
                continue

            used_ops.append(op)  # append in reverse topological order

            # recursively search for nested blocks
            ops_to_check = []
            for b in op.blocks:
                ops_to_check += b.operations_for_vars(b.outputs)
            ops_to_check.append(op)

            # mark used vars
            for op_to_check in ops_to_check:
                # mark all op's inputs to used
                for _, input_var in op_to_check.inputs.items():
                    if isinstance(input_var, (tuple, list)):
                        used_vars.update(list(input_var))
                    else:
                        used_vars.add(input_var)

        return used_ops[::-1]

    def indented_str(self, indent=None):
        if indent is None:
            indent = ""
        s = (
            indent
            + self.name
            + "("
            + ", ".join([str(var) for var in self._block_inputs])
        )
        s += ") {\n"
        for op in self.operations:
            s += op.indented_str(indent + SPACES * 1)
        s += indent + "} -> ("
        if self._outputs is not None:
            s += ", ".join(["%" + v.name for v in self._outputs])
        s += ")\n"
        return s

    def __repr__(self):
        return self.__str__()

    def __str__(self):
        return self.indented_str()

    def get_dot_string(
        self,
        function_name="main",
        prefix_id=0,
        highlight_debug_op_types=None,
        highlight_debug_op_names=None,
    ):
        """
        Return the dot string that can be used to show the block
        with dot. Const ops are not added to the dot string.

        * Input vars : yellow
        * output vars : goldenrod2
        * op names that user wants to highlight, provided in "highlight_debug_op_names": cyan
        * op types that user wants to highlight, provided in "highlight_debug_op_types": green

        Examples
        --------
        >>> import graphviz
        >>> graphviz.Source(block.get_dot_string()).view()
        >>> # OR
        >>> graphviz.Source(block.get_dot_string()).view(filename='graph.pdf')
        """
        if highlight_debug_op_types is None:
            highlight_debug_op_types = []
        if highlight_debug_op_names is None:
            highlight_debug_op_names = []

        dotstring = "digraph g {\n" + "\tcompound=true;\n"

        input_var_names = list(self.inputs.keys())
        output_var_names = [v.name for v in self.outputs]

        debug_op_types = []
        if len(highlight_debug_op_types) > 0:
            for op in self.operations:
                if op.op_type in highlight_debug_op_types:
                    debug_op_types.append(op.name)

        vis = DotVisitor()
        vis.highlight_nodes(input_var_names, "yellow").highlight_nodes(
            output_var_names, "goldenrod2"
        ).highlight_nodes(highlight_debug_op_names, "cyan").highlight_nodes(
            debug_op_types, "green"
        )

        vis.visit_all(self, nodename_prefix=str(prefix_id))
        res = vis.get_result("subgraph", "cluster_" + function_name.replace("/", "_"))
        dotstring += "\n".join("\t" + r for r in res.split("\n")) + "\n"
        dotstring += "}"
        return dotstring


class Function(Block):
    """
    """

    def __init__(self, inputs):
        """
        inputs: str -> placeholder
        """
        self.placeholder_inputs = inputs
        # str -> Var
        self._input_dict = OrderedDict()
        for k, v in self.placeholder_inputs.items():
            v.set_name(k)  # set to user input name
            self._input_dict[k] = v.outputs[0]
        self.function_inputs = tuple(self._input_dict.values())

        global k_used_symbols
        global k_num_internal_syms
        for inp in self.function_inputs:
            if types.is_tensor(inp.dtype):
                shapes = inp.dtype.get_shape()
                for s in shapes:
                    if is_symbolic(s):
                        k_used_symbols.add(s)
        super(Function, self).__init__()

    # Override Block's input
    @property
    def inputs(self):
        return self._input_dict

    def __repr__(self):
        return self.__str__()

    def __str__(self):
        return self.to_str("function")

    def to_str(self, func_name="function"):
        if len(self._input_dict) == 0:
            s = func_name + "() {"
        else:
            inputs = [(in_name, ph) for in_name, ph in self._input_dict.items()]
            s = func_name + "(" + str(inputs[0][1])
            for in_name, ph in inputs[1:]:
                s += ",\n" + " " * (len(func_name) + 1) + str(ph)
            s += ") {\n"
            s += self.indented_str(SPACES)
            s += "}\n"
        return s
