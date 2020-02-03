# -*- coding: utf-8 -*-
# Copyright © 2018 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Python API for MPS neural network backend
"""
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import os as _os
import ctypes as _ctypes
import numpy as _np
import six as _six
from copy import deepcopy as _deepcopy
from turicreate import config as _tc_config
from ._internal_utils import _mac_ver


class MpsGraphNetworkType(object):
    kSingleReLUGraphNet = 0
    kSingleConvGraphNet = 1
    kSingleBNGraphNet = 2
    kSingleMPGraphNet = 3
    kODGraphNet = 4
    kSTGraphNet = 5


class MpsGraphMode(object):
    Train = 0
    TrainReturnGrad = 1
    Inference = 2


class MpsLowLevelNetworkType(object):
    kSingleReLUNet = 0
    kSingleConvNet = 1
    kSingleBNNet = 2
    kSingleMPNet = 3
    kSingle1DConvNet = 4
    kODNet = 5
    kSingleDropOut = 6
    kSingleFcNet = 7
    kSingleSoftMaxNet = 8
    kActivityClassifierNet = 9
    kSingleLstmNet = 10


class MpsLowLevelMode(object):
    kLowLevelModeTrain = 0
    kLowLevelModeInference = 1
    kLowLevelModeTest = 2


def _decode_bytes_to_native_string(s):
    if _six.PY3:
        return s.decode()
    else:
        return s


def _prepare_network_parameters(arg_dict):
    items = []
    for name, arr in arg_dict.items():
        arr = _np.asarray(arr, dtype=_np.float32)
        items.append((name, MpsFloatArray(arr)))

    name = (_ctypes.c_char_p * len(items))()
    arr = (_ctypes.c_void_p * len(items))()
    for i in range(len(items)):
        name[i] = _ctypes.c_char_p(items[i][0].encode())
        arr[i] = items[i][1].handle
    return items, name, arr


_g_TCMPS_LIB = None


def _load_tcmps_lib():
    """
    Load global singleton of tcmps lib handler.

    This function is used not used at the top level, so
    that the shared library is loaded lazily only when needed.
    """
    global _g_TCMPS_LIB
    if _g_TCMPS_LIB is None:
        # This library requires macOS 10.14 or above
        if _mac_ver() < (10, 14):
            return None

        # The symbols defined in libtcmps are now exposed directly by
        # libunity_shared. Eventually the object_detector and
        # activity_classifier toolkits will use the same Python/C++ bridge as
        # the other toolkits, and this usage of ctypes will go away.
        file_dir = _os.path.dirname(__file__)
        lib_path = _os.path.abspath(
            _os.path.join(file_dir, _os.pardir, "libunity_shared.dylib")
        )
        try:
            _g_TCMPS_LIB = _ctypes.CDLL(lib_path, _ctypes.RTLD_LOCAL)
        except OSError:
            pass
    return _g_TCMPS_LIB


def has_fast_mps_support():
    """
    Returns True if the environment has MPS backend support
    and a high-power (fast) device is available.
    """
    lib = _load_tcmps_lib()
    if lib is None:
        return False

    c_bool = _ctypes.c_bool()
    ret = lib.TCMPSHasHighPowerMetalDevice(_ctypes.byref(c_bool))
    return ret == 0 and c_bool.value


def use_mps():
    """
    Returns True if MPS can and should be used.
    """
    return _tc_config.get_num_gpus() != 0 and has_fast_mps_support()


def mps_device_name():
    """
    Returns name of MPS device that will be used, else None.
    """
    lib = _load_tcmps_lib()
    if lib is None:
        return None

    n = 256
    c_name = (_ctypes.c_char * n)()
    ret = lib.TCMPSMetalDeviceName(_ctypes.byref(c_name), _ctypes.c_int32(n))
    if ret == 0:
        return _decode_bytes_to_native_string(c_name.value)
    else:
        return None


def mps_device_memory_limit():
    """
    Returns the memory size in bytes that can be effectively allocated on the
    MPS device that will be used, or None if no suitable device is available.
    """
    lib = _load_tcmps_lib()
    if lib is None:
        return None

    c_size = _ctypes.c_uint64()
    ret = lib.TCMPSMetalDeviceMemoryLimit(_ctypes.byref(c_size))
    return c_size.value if ret == 0 else None


def _xavier_init(weight):
    shape = weight.shape
    dim = len(shape)
    if dim < 2:
        raise ValueError("Xavier init expects at least 2 dimensions")

    scale = 1
    n_in = shape[0]
    n_out = shape[-1]

    if dim > 2:
        scale = _np.prod(shape[1:-1])

    c = _np.sqrt(3.0 / (0.5 * (n_in * scale + n_out * scale)))
    return _np.random.uniform(-c, c, shape).astype(_np.float32)


def _shape_tuple_from_ctypes(shape_ptr, dim):
    # size_t* shape_ptr
    assert isinstance(shape_ptr, _ctypes.POINTER(_ctypes.c_size_t))

    # size_t dim
    assert isinstance(dim, _ctypes.c_size_t)

    # Wrap size_t* as size_t[dim]
    shape_buf = (_ctypes.c_size_t * dim.value).from_address(
        _ctypes.addressof(shape_ptr.contents)
    )

    # Convert size_t[dim] to tuple
    return tuple(shape_buf)


def _numpy_array_from_ctypes(data_ptr, shape_ptr, dim):
    # float* data_ptr
    assert isinstance(data_ptr, _ctypes.POINTER(_ctypes.c_float))

    shape = _shape_tuple_from_ctypes(shape_ptr, dim)

    # Wrap float* to float[size]
    size = _np.prod(shape)
    data_buf = (_ctypes.c_float * size).from_address(
        _ctypes.addressof(data_ptr.contents)
    )

    # Convert float[size] to numpy
    return _np.fromiter(data_buf, _np.float32, size).reshape(shape)


class MpsFloatArray(object):
    """
    A Python wrapper owning a C++ float_array created by the TCMPS backend.

    This class exists to simplify conversions from numpy to the TCMPS format and
    to simplify memory management. Instances usually just serve as arguments to
    the methods on MpsGraphAPI and MpsLowLevelAPI, below.
    """

    def __init__(self, x):
        """Wrap an existing TCMPSFloatArrayRef or a numpy array"""

        # Load TCMPS backend library.
        self._LIB = _load_tcmps_lib()
        assert self._LIB is not None, "Cannot use MpsFloatArray without libtcmps.dylib"

        # If `x` is a void*, assume it's the right type and just wrap it.
        if isinstance(x, _ctypes.c_void_p):
            self.handle = x
            return

        assert isinstance(x, _np.ndarray)

        # Convert the input if necessary to contain a contiguous float array.
        self.data = x
        if self.data.dtype != _np.float32:
            self.data = self.data.astype(_np.float32)
        if not self.data.flags.c_contiguous:
            self.data = self.data.copy()
        assert self.data.flags.c_contiguous, "Data must be row-major"

        # Obtain a pointer to the internal float array (and obtain size).
        data_ptr = self.data.ctypes.data_as(_ctypes.POINTER(_ctypes.c_void_p))
        sz = _ctypes.c_size_t(self.data.size)

        # Copy the shape so that it contains a size_t array.
        self.shape = _np.array(self.data.shape).astype(_np.uintp)
        shape_ptr = self.shape.ctypes.data_as(_ctypes.POINTER(_ctypes.c_size_t))
        dim = _ctypes.c_size_t(self.data.ndim)

        # Call into TCMPS to create a wrapper around self.data and self.shape.
        # Those two properties must outlive the resulting self.handle.
        self.handle = _ctypes.c_void_p()
        status_code = self._LIB.TCMPSCreateFloatArray(
            _ctypes.byref(self.handle), data_ptr, sz, shape_ptr, dim
        )
        assert status_code == 0, "Error calling TCMPSCreateFloatArray"

    def __del__(self):
        status_code = self._LIB.TCMPSDeleteFloatArray(self.handle)
        assert status_code == 0, "Error calling TCMPSDeleteFloatArray"

    def shape(self):
        """Copy the shape from TCMPS as a new numpy ndarray."""

        # Create C variables that will serve as out parameters for TCMPS.
        shape_ptr = _ctypes.POINTER(_ctypes.c_size_t)()  # size_t* shape_ptr
        dim = _ctypes.c_size_t()  # size_t dim

        # Obtain pointer into memory owned by the C++ object self.handle.
        status_code = self._LIB.TCMPSGetFloatArrayShape(
            self.handle, _ctypes.byref(shape_ptr), _ctypes.byref(dim)
        )
        assert status_code == 0, "Error calling TCMPSGetFloatArrayShape"

        return _shape_tuple_from_ctypes(shape_ptr, dim)

    def asnumpy(self):
        """Copy the data from TCMPS into a new numpy ndarray"""

        # Create C variables that will serve as out parameters for TCMPS.
        data_ptr = _ctypes.POINTER(_ctypes.c_float)()  # float* data_ptr
        shape_ptr = _ctypes.POINTER(_ctypes.c_size_t)()  # size_t* shape_ptr
        dim = _ctypes.c_size_t()  # size_t dim

        # Obtain pointers into memory owned by the C++ object self.handle.
        # Note that this may trigger synchronization with another thread
        # producing the data.
        status_code = self._LIB.TCMPSReadFloatArray(
            self.handle,
            _ctypes.byref(data_ptr),
            _ctypes.byref(shape_ptr),
            _ctypes.byref(dim),
        )
        assert status_code == 0, "Error calling TCMPSReadFloatArray"

        return _numpy_array_from_ctypes(data_ptr, shape_ptr, dim)


class MpsFloatArrayIterator(object):
    """
    A Python wrapper owning a sequence of name/float_array pairs output from the
    TCMPS backend.

    This class exists to simplify conversions from the output of TCMPS export
    functions. It implements the iterator protocol, so that a Python dict
    (mapping parameter names for numpy arrays) can be initialized directly from
    an instance of this class.
    """

    def __init__(self, handle):
        """Wrap the output of a TCMPSExport* function."""
        self._LIB = _load_tcmps_lib()
        assert (
            self._LIB is not None
        ), "Cannot use MpsFloatArrayIterator without libtcmps.dylib"

        self.handle = handle

    def __del__(self):
        status_code = self._LIB.TCMPSDeleteFloatArrayMapIterator(self.handle)
        assert status_code == 0, "Error calling TCMPSDeleteFloatArrayMapIterator"

    def __iter__(self):
        return self

    def __next__(self):
        # Create C variables that will serve as out parameters for TCMPS.
        name_ptr = _ctypes.c_char_p()  # char* name_ptr
        data_ptr = _ctypes.POINTER(_ctypes.c_float)()  # float* data_ptr
        shape_ptr = _ctypes.POINTER(_ctypes.c_size_t)()  # size_t* shape_ptr
        dim = _ctypes.c_size_t()  # size_t dim

        # Obtain pointers into memory owned by the C++ object self.handle.
        status_code = self._LIB.TCMPSNextFloatArray(
            self.handle,
            _ctypes.byref(name_ptr),
            _ctypes.byref(data_ptr),
            _ctypes.byref(shape_ptr),
            _ctypes.byref(dim),
        )

        if status_code != 0:
            raise StopIteration

        # Convert char* to Python string
        name = _decode_bytes_to_native_string(name_ptr.value)

        # Convert data to numpy
        array = _numpy_array_from_ctypes(data_ptr, shape_ptr, dim)

        return (name, array)

    def next(self):
        return self.__next__()


# ----------------------------------------------------------
#
#  MPS Graph level API, currently used by Object detector
#
# ----------------------------------------------------------


class MpsGraphAPI(object):
    def __init__(self, network_id):
        self.handle = _ctypes.c_void_p()
        self._LIB = _load_tcmps_lib()
        assert self._LIB is not None, "Cannot use MpsGraphAPI without libtcmps.dylib"
        self._buf_out_fp16 = None
        self._buf_loss = None
        self._ishape = None
        self._oshape = None
        self.network_id = network_id
        # current state, for reloading weights
        self._cur_config = {}
        self._cur_learning_rate = None

    def __del__(self):
        self._LIB.TCMPSDeleteGraphModule(self.handle)

    def init(self, n, c_in, h_in, w_in, c_out, h_out, w_out, config=None, weights=None):
        if weights is None:
            weights = {}
        if config is None:
            config = {
                "learning_rate": 1e-3,
                "gradient_clipping": 0.025,
                "weight_decay": 0.00005,
                "momentum": 0.9,
            }

        self._mode = int(config.get("mode", MpsGraphMode.TrainReturnGrad))
        self._is_train = self._mode in {
            MpsGraphMode.TrainReturnGrad,
            MpsGraphMode.Train,
        }

        config_items, config_name, config_arr = _prepare_network_parameters(config)
        weights_items, weights_name, weights_arr = _prepare_network_parameters(weights)
        self._LIB.TCMPSCreateGraphModule(
            _ctypes.byref(self.handle),
            self.network_id,
            _ctypes.c_int32(n),
            _ctypes.c_int32(c_in),
            _ctypes.c_int32(h_in),
            _ctypes.c_int32(w_in),
            _ctypes.c_int32(c_out),
            _ctypes.c_int32(h_out),
            _ctypes.c_int32(w_out),
            config_name,
            config_arr,
            _ctypes.c_int32(len(config_items)),
            weights_name,
            weights_arr,
            _ctypes.c_int32(len(weights_items)),
        )
        self._cur_config = _deepcopy(config)
        if self._mode == MpsGraphMode.TrainReturnGrad:
            sz = n * c_in * h_in * w_in
        else:
            sz = n * c_out * h_out * w_out
        self._buf_out_fp16 = (_ctypes.c_float * (sz // 2))()
        self._buf_loss = (_ctypes.c_float * n)()
        self._ishape = (n, h_in, w_in, c_in)
        self._oshape = (n, h_out, w_out, c_out)

    def train(self, input, label):
        """
        Submits an input batch to the model. Returns a MpsFloatArray
        representing the batch loss. Calling asnumpy() on this value will wait
        for the batch to finish and yield the loss as a numpy array.
        """

        assert self._mode == MpsGraphMode.Train
        assert input.shape == self._ishape
        assert label.shape == self._oshape

        input_array = MpsFloatArray(input)
        label_array = MpsFloatArray(label)
        result_handle = _ctypes.c_void_p()
        status_code = self._LIB.TCMPSTrainGraph(
            self.handle,
            input_array.handle,
            label_array.handle,
            _ctypes.byref(result_handle),
        )

        assert status_code == 0, "Error calling TCMPSTrainGraph"
        assert result_handle, "TCMPSTrainGraph unexpectedly returned NULL pointer"

        result = MpsFloatArray(result_handle)

        # Output from training should be a one-dimensional array of loss values,
        # one per example in the batch.
        assert result.shape() == (self._oshape[0],)

        return result

    def predict(self, input):
        """
        Submits an input batch to the model. Returns a MpsFloatArray
        representing the model predictions. Calling asnumpy() on this value will
        wait for the batch to finish and yield the predictions as a numpy array.
        """

        assert self._mode == MpsGraphMode.Inference
        assert input.shape == self._ishape

        input_array = MpsFloatArray(input)
        result_handle = _ctypes.c_void_p()
        status_code = self._LIB.TCMPSPredictGraph(
            self.handle, input_array.handle, _ctypes.byref(result_handle)
        )

        assert status_code == 0, "Error calling TCMPSPredictGraph"
        assert result_handle, "TCMPSPredictGraph unexpectedly returned NULL pointer"

        result = MpsFloatArray(result_handle)
        assert result.shape() == self._oshape

        return result

    def train_return_grad(self, input, grad):
        """
        Performs a forward pass from the input batch, followed by a backward
        pass using the provided gradient (in place of a loss function). Returns
        a MpsFloatArray representing the output (final gradient) of the backward
        pass. Calling asnumpy() on this value will wait for the batch to finish
        and yield the output as a numpy array.
        """

        assert self._mode == MpsGraphMode.TrainReturnGrad
        assert input.shape == self._ishape
        assert grad.shape == self._oshape

        input_array = MpsFloatArray(input)
        grad_array = MpsFloatArray(grad)
        result_handle = _ctypes.c_void_p()
        status_code = self._LIB.TCMPSTrainGraph(
            self.handle,
            input_array.handle,
            grad_array.handle,
            _ctypes.byref(result_handle),
        )

        assert status_code == 0, "Error calling TCMPSTrainReturnGradGraph"
        assert (
            result_handle
        ), "TCMPSTrainReturnGradGraph unexpectedly returned NULL pointer"

        result = MpsFloatArray(result_handle)
        assert result.shape() == self._ishape

        return result

    def set_learning_rate(self, new_lr):
        self._cur_learning_rate = new_lr
        self._LIB.TCMPSSetLearningRateGraph(self.handle, _ctypes.c_float(new_lr))

    def load(self, weights):
        self._LIB.TCMPSDeleteGraphModule(self.handle)
        self.handle = _ctypes.c_void_p()
        self._LIB.TCMPSCreateGraphModule(
            _ctypes.byref(self.handle), _ctypes.c_int(self._mode)
        )
        n, h_in, w_in, c_in = self._ishape
        _, h_out, w_out, c_out = self._oshape
        self.init(
            n,
            c_in,
            h_in,
            w_in,
            c_out,
            h_out,
            w_out,
            config=self._cur_config,
            weights=weights,
        )
        # Reload state
        if self._cur_learning_rate:
            self.set_learning_rate(self._cur_learning_rate)

    def export(self):
        iter_handle = _ctypes.c_void_p()
        status_code = self._LIB.TCMPSExportGraph(
            self.handle, _ctypes.byref(iter_handle)
        )
        assert status_code == 0
        return dict(MpsFloatArrayIterator(iter_handle))


# ----------------------------------------------------------
#
#  MPS Graph level API, currently used by Activity Classifier
#
# ----------------------------------------------------------


class MpsLowLevelAPI(object):
    def __init__(self, network_id=MpsLowLevelNetworkType.kActivityClassifierNet):
        self.handle = _ctypes.c_void_p()
        self._LIB = _load_tcmps_lib()
        assert self._LIB is not None, "Cannot use MpsLowLevelAPI without libtcmps.dylib"
        self._LIB.TCMPSCreateCNNModule(_ctypes.byref(self.handle))
        self._buf = None
        self._buf_g = None
        self._ishape = None
        self._oshape = None
        self.network_id = network_id

    def __del__(self):
        self._LIB.TCMPSDeleteCNNModule(self.handle)

    def init(self, n, c_in, h_in, w_in, c_out, h_out, w_out, updater=1, config={}):
        config_items, config_name, config_arr = _prepare_network_parameters(config)
        self._LIB.TCMPSInit(
            self.handle,
            self.network_id,
            _ctypes.c_int32(n),
            _ctypes.c_int32(c_in),
            _ctypes.c_int32(h_in),
            _ctypes.c_int32(w_in),
            _ctypes.c_int32(c_out),
            _ctypes.c_int32(h_out),
            _ctypes.c_int32(w_out),
            _ctypes.c_int32(updater),
            config_name,
            config_arr,
            _ctypes.c_int32(len(config_items)),
        )
        sz = n * c_out * h_out * w_out
        self._buf = (_ctypes.c_float * sz)()
        sz = n * c_in * h_in * w_in
        self._buf_g = (_ctypes.c_float * sz)()

        self._ishape = (n, h_in, w_in, c_in)
        self._oshape = (n, h_out, w_out, c_out)

    def load(self, weights):
        weights_items, weights_name, weights_arr = _prepare_network_parameters(weights)
        self._LIB.TCMPSLoad(
            self.handle, weights_name, weights_arr, _ctypes.c_int32(len(weights_items))
        )

    def export(self):
        iter_handle = _ctypes.c_void_p()
        status_code = self._LIB.TCMPSExport(self.handle, _ctypes.byref(iter_handle))
        assert status_code == 0
        return dict(MpsFloatArrayIterator(iter_handle))

    def initalize_weights(self):
        args = self.export()
        for key, val in args.items():
            if key.endswith("weight"):
                args[key] = _xavier_init(val)
        self.load(args)

    def _loss_or_iteration_call(self, lib_method, input, labels, weights):
        expected_label_shape = self._oshape[:-1] + (1,)
        assert input.shape == self._ishape
        assert labels.shape == expected_label_shape
        assert weights.shape == expected_label_shape

        input_array = MpsFloatArray(input)
        labels_array = MpsFloatArray(labels)
        weights_array = MpsFloatArray(weights)
        output_handle = _ctypes.c_void_p()
        loss_handle = _ctypes.c_void_p()
        status_code = lib_method(
            self.handle,
            input_array.handle,
            labels_array.handle,
            weights_array.handle,
            _ctypes.byref(output_handle),
            _ctypes.byref(loss_handle),
        )

        assert status_code == 0, "Error calling TCMPS"
        assert output_handle, "TCMPS unexpectedly returned NULL pointer"
        assert loss_handle, "TCMPS unexpectedly returned NULL pointer"

        output = MpsFloatArray(output_handle)
        loss = MpsFloatArray(loss_handle)

        assert output.shape() == self._oshape
        assert loss.shape() == (self._oshape[0], 1, 1, 1)

        return (output, loss)

    def train(self, input, labels, weights):
        return self._loss_or_iteration_call(
            self._LIB.TCMPSTrain, input, labels, weights
        )

    def predict_with_loss(self, input, labels, weights):
        return self._loss_or_iteration_call(
            self._LIB.TCMPSPredict, input, labels, weights
        )

    def predict(self, input):
        assert input.shape == self._ishape

        input_array = MpsFloatArray(input)
        output_handle = _ctypes.c_void_p()
        status_code = self._LIB.TCMPSPredict(
            self.handle,
            input_array.handle,
            None,
            None,
            _ctypes.byref(output_handle),
            None,
        )

        assert status_code == 0, "Error calling TCMPSPredict"
        assert output_handle, "TCMPSPredict unexpectedly returned NULL pointer"

        output = MpsFloatArray(output_handle)
        assert output.shape() == self._oshape

        return output


class MpsStyleGraphAPI(object):
    def __init__(
        self, n, c_in, h_in, w_in, c_out, h_out, w_out, config=None, weights=None
    ):
        self.handle = _ctypes.c_void_p()
        self._LIB = _load_tcmps_lib()
        assert self._LIB is not None, "Cannot use MpsGraphAPI without libtcmps.dylib"

        self.network_id = MpsGraphNetworkType.kSTGraphNet
        self._cur_config = {}
        if weights is None:
            weights = {}

        if config is None:
            config = {
                "learning_rate": 1e-3,
                "gradient_clipping": 0.025,
                "weight_decay": 0.00005,
                "momentum": 0.9,
            }

        config_items, config_name, config_arr = _prepare_network_parameters(config)
        weights_items, weights_name, weights_arr = _prepare_network_parameters(weights)

        self._LIB.TCMPSCreateGraphModule(
            _ctypes.byref(self.handle),
            self.network_id,
            _ctypes.c_int32(n),
            _ctypes.c_int32(c_in),
            _ctypes.c_int32(h_in),
            _ctypes.c_int32(w_in),
            _ctypes.c_int32(c_out),
            _ctypes.c_int32(h_out),
            _ctypes.c_int32(w_out),
            config_name,
            config_arr,
            _ctypes.c_int32(len(config_items)),
            weights_name,
            weights_arr,
            _ctypes.c_int32(len(weights_items)),
        )
        self._cur_config = _deepcopy(config)

    def __del__(self):
        self._LIB.TCMPSDeleteGraphModule(self.handle)

    def train(self, input, label, index):
        input_array = MpsFloatArray(input)
        label_array = MpsFloatArray(label)

        result_handle = _ctypes.c_void_p()

        status_code = self._LIB.TCMPSTrainStyleTransferGraph(
            self.handle,
            _ctypes.c_int32(index),
            input_array.handle,
            label_array.handle,
            _ctypes.byref(result_handle),
        )

        assert status_code == 0, "Error calling TCMPSTrainStyleTransferGraph"
        assert (
            result_handle
        ), "TCMPSTrainStyleTransferGraph unexpectedly returned NULL pointer"

        result = MpsFloatArray(result_handle)

        return result

    def predict(self, input):
        input_array = MpsFloatArray(input)
        result_handle = _ctypes.c_void_p()

        status_code = self._LIB.TCMPSPredictGraph(
            self.handle, input_array.handle, _ctypes.byref(result_handle)
        )

        assert status_code == 0, "Error calling TCMPSPredictGraph"
        assert result_handle, "TCMPSPredictGraph unexpectedly returned NULL pointer"

        result = MpsFloatArray(result_handle)

        return result

    def train_return_grad(self, input, grad):
        pass

    def set_learning_rate(self, new_lr):
        pass

    def load(self, weights):
        pass

    def export(self):
        iter_handle = _ctypes.c_void_p()
        status_code = self._LIB.TCMPSExportGraph(
            self.handle, _ctypes.byref(iter_handle)
        )
        assert status_code == 0
        return dict(MpsFloatArrayIterator(iter_handle))
