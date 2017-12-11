# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
#cython: boundscheck=False, wraparound=False

from cy_flexible_type cimport flexible_type, flex_type_enum, UNDEFINED, flex_int
from cy_flexible_type cimport flexible_type_from_pyobject
from cy_flexible_type cimport process_common_typed_list
from cy_flexible_type cimport pyobject_from_flexible_type
from cy_callback cimport register_exception
from .._gl_pickle import GLUnpickler

from copy import deepcopy
from libcpp.string cimport string
from libcpp.vector cimport vector
import ctypes
import os
import traceback
import sys
cimport cython

from random import seed as set_random_seed
from cy_cpp_utils cimport str_to_cpp, cpp_to_str

from cpython.version cimport PY_MAJOR_VERSION

if PY_MAJOR_VERSION == 2:
   import cPickle as py_pickle
elif PY_MAJOR_VERSION >= 3:
   import pickle as py_pickle

cdef extern from "<util/cityhash_tc.hpp>" namespace "turi":
    size_t hash64(const string&)

cdef extern from "<sframe/sframe_rows.hpp>" namespace "turi":

    cdef struct row "sframe_rows::row":
        const flexible_type& at "operator[]"(size_t col)
        
    cdef struct sframe_rows:
        row at "operator[]"(size_t row)

        size_t num_columns()
        size_t num_rows()


cdef extern from "<lambda/pylambda.hpp>" namespace "turi::lambda":

    cdef struct lambda_call_data:
        flex_type_enum output_enum_type
        bint skip_undefined

        # It's the responsibility of the calling class to make sure
        # these are valid.
        const flexible_type* input_values
        flexible_type* output_values
        size_t n_inputs

    cdef struct lambda_call_by_dict_data:
        flex_type_enum output_enum_type

        const vector[string]* input_keys
        const vector[vector[flexible_type] ]* input_rows

        flexible_type* output_values

    cdef struct lambda_call_by_sframe_rows_data:
        flex_type_enum output_enum_type

        const vector[string]* input_keys
        const sframe_rows* input_rows

        flexible_type* output_values
        
    ################################################################################
    # Graph Pylambda stuff
    cdef struct lambda_graph_triple_apply_data:

        const vector[vector[flexible_type]]* all_edge_data
        vector[vector[flexible_type]]* out_edge_data

        vector[vector[flexible_type]]* source_partition
        vector[vector[flexible_type]]* target_partition
        
        const vector[string]* vertex_keys
        const vector[string]* edge_keys
        const vector[string]* mutated_edge_keys
        size_t srcid_column, dstid_column
    
    cdef struct pylambda_evaluation_functions:
        void (*set_random_seed)(size_t seed)
        size_t (*init_lambda)(const string&)
        void (*release_lambda)(size_t)
        void (*eval_lambda)(size_t, lambda_call_data*)
        void (*eval_lambda_by_dict)(size_t, lambda_call_by_dict_data*)
        void (*eval_lambda_by_sframe_rows)(size_t, lambda_call_by_sframe_rows_data*)
        void (*eval_graph_triple_apply)(size_t, lambda_graph_triple_apply_data*)

    # The function to call to set everything up.
    void set_pylambda_evaluation_functions(pylambda_evaluation_functions* eval_function_struct)

    
################################################################################
# Lambda evaluation class. 

cdef class lambda_evaluator(object):
    """
    Main class to handle the actual pylambda evaluations.  It wraps a
    single lambda function that is meant to be called by one of the
    eval_* interface methods below.
    """

    cdef object lambda_function
    cdef list output_buffer
    cdef list keys
    cdef dict arg_dict_base 
    cdef bytes lambda_string

    def __init__(self, bytes lambda_string):

        self.lambda_string = lambda_string
        self.keys = None
        self.arg_dict_base = None

        cdef bint is_directory

        try:
            is_directory = os.path.isdir(self.lambda_string)
        except Exception:
            is_directory = False

        if is_directory:
            unpickler = GLUnpickler(self.lambda_string.decode())
            self.lambda_function = unpickler.load()
        else:
            self.lambda_function = py_pickle.loads(self.lambda_string)

        self.output_buffer = []

    @cython.boundscheck(False)
    cdef _set_dict_keys(self, const vector[string]* input_keys):
        cdef long n_keys = input_keys[0].size()
        cdef long i

        if self.keys is None or len(self.keys) != n_keys:
            self.keys = [None] * n_keys

        for i in range(n_keys):
            self.keys[i] = cpp_to_str(input_keys[0][i])

        # Now, build the base arg_dict_base
        self.arg_dict_base = {k : None for k in self.keys}
                
    @cython.boundscheck(False)
    cdef eval_simple(self, lambda_call_data* lcd):
        
        cdef const flexible_type* v_in = lcd.input_values
        cdef long n = lcd.n_inputs
        
        if len(self.output_buffer) != n:
            self.output_buffer = [None]*n
            
        cdef long i
        cdef object x
        
        for i in range(0, n):
            if v_in[i].get_type() == UNDEFINED and lcd.skip_undefined:
                self.output_buffer[i] = None
                continue
            
            x = pyobject_from_flexible_type(v_in[i])
            self.output_buffer[i] = self.lambda_function(x)

        process_common_typed_list(lcd.output_values, self.output_buffer, lcd.output_enum_type)

    @cython.boundscheck(False)
    cdef eval_multiple_rows(self, lambda_call_by_dict_data* lcd):

        cdef dict arg_dict = {}
        cdef long i, j
        cdef long n = lcd.input_rows[0].size()
        cdef long n_keys = lcd.input_keys[0].size()

        self._set_dict_keys(lcd.input_keys)

        if len(self.output_buffer) != n:
            self.output_buffer = [None]*n

        for i in range(n):
            if lcd.input_keys[0][i].size() != n_keys:
                raise ValueError("Row %d does not have the correct number of rows (%d, should be %d)"
                                 % (i, lcd.input_keys[0][i].size(), n))

            arg_dict = self.arg_dict_base.copy()
            
            for j in range(n_keys):
                arg_dict[self.keys[j]] = pyobject_from_flexible_type(lcd.input_rows[0][i][j])

            self.output_buffer[i] = self.lambda_function(arg_dict)

        process_common_typed_list(lcd.output_values, self.output_buffer, lcd.output_enum_type)

    @cython.boundscheck(False)
    cdef eval_sframe_rows(self, lambda_call_by_sframe_rows_data* lcd):

        cdef dict arg_dict = {}
        cdef long i, j
        cdef long n = lcd.input_rows[0].num_rows()
        cdef long n_keys = lcd.input_keys[0].size()

        assert lcd.input_rows[0].num_columns() == n_keys

        self._set_dict_keys(lcd.input_keys)

        if len(self.output_buffer) != n:
            self.output_buffer = [None]*n

        for i in range(n):
            arg_dict = self.arg_dict_base.copy()

            for j in range(n_keys):
                arg_dict[self.keys[j]] = pyobject_from_flexible_type(lcd.input_rows[0].at(i).at(j))

            self.output_buffer[i] = self.lambda_function(arg_dict)

        process_common_typed_list(lcd.output_values, self.output_buffer, lcd.output_enum_type)

    @cython.boundscheck(False)
    cdef process_output_dict(self, flexible_type* output_values, list mut_keys, dict ref_dict, dict ret_dict):
        """
        Checks to make sure all keys in ret_dict are in ref_dict, and updates
        ret_dict with values from ref_dict as needed.  Puts the resulting values
        in ret_dict into the position of output_values corresponding to the
        location in mut_keys.  Furthermore, if any values are mutated in mut_keys,
        then they are checked for equality. 
        """

        cdef long j
        cdef long src_key_count = 0
        cdef object src_val
        cdef list bad_values

        for j, k in enumerate(mut_keys):

            try:
                src_val = ret_dict[k.decode()]
                src_key_count += 1
                
            except KeyError:                
                src_val = ref_dict[k.decode()]
                 
            output_values[j] = flexible_type_from_pyobject(src_val)

        if src_key_count != len(ret_dict):
            # Seems there are extra keys that we don't want
            bad_values = []

            for k in ret_dict.iterkeys():
                if k not in ref_dict:
                    bad_values.append(k)

            if bad_values:
                raise KeyError("Return dictionary has invalid key(s) '%s'; possible keys are: %s" %
                               ((", ".join("'%s'" % str(k2) for k2 in bad_values)),
                                (", ".join("'%s'" % str(k2) for k2 in ref_dict.iterkeys()) )))
            else:
                # This means there are things in ref_dict not in
                # mut_keys, which is not a bad thing.  If we were
                # really rigorous, then we should check to make sure
                # that the values have not been changed; for now,
                # however, assume it just works.
                pass


    @cython.boundscheck(False)
    cdef eval_graph_triple_apply(self, lambda_graph_triple_apply_data* lcg):

        cdef long i, j
        cdef long n_edges = lcg.all_edge_data[0].size()
        cdef long n_edge_keys = lcg.edge_keys[0].size()
        cdef long n_vertex_keys = lcg.vertex_keys[0].size()

        cdef list edge_keys = [lcg.edge_keys[0][j] for j in range(n_edge_keys)]
        cdef list vertex_keys = [lcg.vertex_keys[0][j] for j in range(n_vertex_keys)]

        cdef long n_mutated_edges = lcg.mutated_edge_keys[0].size()
        cdef list mutated_edge_keys = [lcg.mutated_edge_keys[0][j] for j in range(n_mutated_edges)]

        cdef dict edge_object = {}, source_object = {}, target_object = {}
        cdef dict edge_object_param, source_object_param, target_object_param
        
        cdef flex_int srcid, dstid

        cdef dict ret_source_dict, ret_edge_dict, ret_target_dict

        cdef tuple ret
        
        for i in range(n_edges):
            
            srcid = lcg.all_edge_data[0][i][lcg.srcid_column].get_int()
            dstid = lcg.all_edge_data[0][i][lcg.dstid_column].get_int()

            # Set the edge update object. 
            for j in range(n_edge_keys):
                edge_object[edge_keys[j].decode()] = pyobject_from_flexible_type(lcg.all_edge_data[0][i][j]);
                
            edge_object_param = edge_object.copy()
                
            # Set the vertex data
            for j in range(n_vertex_keys):
                source_object[vertex_keys[j].decode()] = pyobject_from_flexible_type(lcg.source_partition[0][srcid][j])
                target_object[vertex_keys[j].decode()] = pyobject_from_flexible_type(lcg.target_partition[0][dstid][j])

            source_object_param = source_object.copy()
            target_object_param = target_object.copy()
                
            _ret = self.lambda_function(source_object_param, edge_object_param, target_object_param)

            if _ret is None or type(_ret) is not tuple or len(<tuple>_ret) != 3:
                raise TypeError("Lambda function must return a tuple of 3 dictionaries of the form "
                                "(source_data, edge_data, target_data).")

            ret = <tuple>_ret

            if ret[0] is not None and type(ret[0]) is not dict:
                raise TypeError("First return argument of lambda function not a dictionary.")

            if ret[1] is not None and type(ret[1]) is not dict:
                raise TypeError("Second return argument of lambda function not a dictionary.")

            if ret[2] is not None and type(ret[2]) is not dict:
                raise TypeError("Third return argument of lambda function not a dictionary.")

            if ret[0] is not None:
                ret_source_dict = ret[0]
                self.process_output_dict(lcg.source_partition[0][srcid].data(), vertex_keys,
                                         source_object, ret_source_dict)

            if n_mutated_edges != 0:
                lcg.out_edge_data[0][i].resize(n_mutated_edges)
                ret_edge_dict = <dict>(ret[1]) if ret[1] is not None else edge_object
                self.process_output_dict(lcg.out_edge_data[0][i].data(), mutated_edge_keys,
                                         edge_object, ret_edge_dict)

            if ret[2] is not None:
                ret_target_dict = <dict>(ret[2])
                self.process_output_dict(lcg.target_partition[0][dstid].data(), vertex_keys,
                                         target_object, ret_target_dict)
            
        # And we're done!
    

################################################################################
# Wrapping functions

cdef dict _lambda_id_to_evaluator = {}
cdef dict _lambda_function_string_to_id = {}

cdef lambda_evaluator _get_lambda_class(size_t lmfunc_id):
    try:
        return <lambda_evaluator>_lambda_id_to_evaluator[lmfunc_id]
    except KeyError:
        raise ValueError("Invalid id for lambda function (%ld)." % lmfunc_id)


cdef pylambda_evaluation_functions eval_functions

#########################################
# Random seed

cdef void _set_random_seed(size_t seed):
    set_random_seed(seed)

eval_functions.set_random_seed = _set_random_seed

########################################
# Initialization function.

cdef size_t _init_lambda(const string& _lambda_string):
    global _lambda_id_to_evaluator
    global _lambda_function_string_to_id

    cdef bytes lambda_string = _lambda_string
    cdef object lmfunc_id

    try:
        lmfunc_id = _lambda_function_string_to_id.get(lambda_string, None)

        # If it's been cleared out, then erase it and restart.
        if lmfunc_id is not None and lmfunc_id not in _lambda_id_to_evaluator:
            del _lambda_function_string_to_id[lambda_string]
            lmfunc_id = None

        if lmfunc_id is None:
            lmfunc_id = hash64(_lambda_string)
            _lambda_id_to_evaluator[lmfunc_id] = lambda_evaluator(lambda_string)
            _lambda_function_string_to_id[lambda_string] = lmfunc_id

        return lmfunc_id

    except Exception, e:
        register_exception(e)
        return 0

eval_functions.init_lambda = _init_lambda

########################################
# Release

cdef void _release_lambda(size_t lmfunc_id):

    try:
        try:
            del _lambda_id_to_evaluator[lmfunc_id]
        except KeyError, ke:
            raise ValueError("Attempted to clear lambda id that does not exist (%d)" % lmfunc_id)

    except Exception, e:
        register_exception(e)
        return

eval_functions.release_lambda = _release_lambda

########################################
# Single column evaluation

cdef void _eval_lambda(size_t lmfunc_id, lambda_call_data* lcd):
    try:
        _get_lambda_class(lmfunc_id).eval_simple(lcd)
    except Exception, e:
        register_exception(e)
        return


eval_functions.eval_lambda = _eval_lambda

########################################
# Multiple column evaluation

cdef void _eval_lambda_by_dict(size_t lmfunc_id, lambda_call_by_dict_data* lcd):
    try:
        _get_lambda_class(lmfunc_id).eval_multiple_rows(lcd)
    except Exception, e:
        register_exception(e)

eval_functions.eval_lambda_by_dict = _eval_lambda_by_dict

########################################
# Multiple column evaluation

cdef void _eval_lambda_by_sframe_rows(size_t lmfunc_id, lambda_call_by_sframe_rows_data* lcd):
    try:
        _get_lambda_class(lmfunc_id).eval_sframe_rows(lcd)
    except Exception, e:
        register_exception(e)

eval_functions.eval_lambda_by_sframe_rows = _eval_lambda_by_sframe_rows

########################################
# Triple Apply stuff

cdef void _eval_graph_triple_apply(size_t lmfunc_id, lambda_graph_triple_apply_data* lcd):
    try:
        _get_lambda_class(lmfunc_id).eval_graph_triple_apply(lcd)
    except Exception, e:
        register_exception(e)

eval_functions.eval_graph_triple_apply = _eval_graph_triple_apply

# Finally, set pylambda evaluation functions in the 
set_pylambda_evaluation_functions(&eval_functions)

################################################################################
# Stuff like this.

cdef extern from "<lambda/pylambda_worker.hpp>" namespace "turi::lambda":
    int pylambda_worker_main(const string& root_path, const string& server_address, int loglevel)


def run_pylambda_worker(str root_path, str server_address, int loglevel):
    return pylambda_worker_main(str_to_cpp(root_path), str_to_cpp(server_address),loglevel)
