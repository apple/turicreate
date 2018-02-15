# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import sys as _sys
import six as _six
from turicreate import config as _tc_config
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._internal_utils import _numeric_param_check_range

def ndarray_from_array(shape, buffer, ctx = None):
    import mxnet as _mx
    import numpy
    shape = [int(x) for x in shape]
    return _mx.ndarray.array(numpy.ndarray(shape, buffer = buffer))

def params_from_dict(params_dict, ctx = None):
    shapes = params_dict['shapes']
    data = params_dict['data']
    import numpy
    return {k: ndarray_from_array(shapes[k], v, ctx) for k,v in data.items()}

def params_to_dict(params):
    shapes = {k: v.shape for k,v in params.items()}
    data = {k: v.asnumpy().flatten() for k,v in params.items()}
    return {'data': data, 'shapes': shapes}

def get_mxnet_state(model):
    sym  = model.symbol
    (arg_params, aux_params) = model.get_params()
    state = {}
    state['sym'] = sym.tojson()
    state['arg_params'] = params_to_dict(arg_params)
    state['aux_params'] = params_to_dict(aux_params)
    return state

def get_mxnet_context(max_devices=None):
    from turicreate.util import _CUDA_GPU_IDS
    import mxnet as _mx
    assert_valid_num_gpus()
    num_gpus = _tc_config.get_num_gpus()
    if num_gpus == 0 or not _CUDA_GPU_IDS:
        return [_mx.cpu()]
    else:
        if num_gpus == -1:
            ctx = [_mx.gpu(i) for i in _CUDA_GPU_IDS]
        elif num_gpus > 0:
            ctx = [_mx.gpu(i) for i in range(num_gpus)]

        ctx = ctx[:max_devices]

        # Do a quick check if we can use the GPU
        try:
            for ctx_i in ctx:
                _mx.nd.array([1], ctx=ctx_i)

        except _mx.base.MXNetError:
            # This is following a cluttered mxnet error, so we have to draw
            # extra attention to this message
            print()
            print('ERROR: Incomplete installation for leveraging GPUs for computations.')
            print('Please make sure you have CUDA installed and run the following line in')
            print('your terminal and try again:')
            print()
            print('    pip uninstall -y mxnet && pip install mxnet-cu90=={}'.format(_mx.__version__))
            print()
            print("Adjust 'cu90' depending on your CUDA version ('cu75' and 'cu80' are also available).")
            print('You can also disable GPU usage altogether by invoking turicreate.config.set_num_gpus(0)')
            _sys.exit(1)
        return ctx

def get_num_gpus_in_use(max_devices=None):
    # Unlike turicreate.config.get_num_gpus() which returns an option that may
    # be set to -1, this function returns GPUs actually in use.
    gpu_ctx = [ctx for ctx in get_mxnet_context(max_devices=max_devices) if ctx.device_type == 'gpu']
    return len(gpu_ctx)

def assert_valid_num_gpus():
    from turicreate.util import _CUDA_GPU_IDS
    num_gpus = _tc_config.get_num_gpus()
    if not _CUDA_GPU_IDS and _sys.platform == 'darwin' and num_gpus > 0:
        raise _ToolkitError('Using GPUs is currently not supported on Mac')
    _numeric_param_check_range('num_gpus', num_gpus, -1, _six.MAXSIZE)

def load_mxnet_model_from_state(state, data, labels=None, existing_module=None, ctx = None):
    import mxnet as _mx
    sym = _mx.symbol.load_json(state['sym'])
    arg_params = params_from_dict(state['arg_params'], ctx)
    aux_params = params_from_dict(state['aux_params'], ctx)

    label_keys = None if labels is None else [l[0] for l in labels]
    mod = _mx.mod.Module(symbol=sym, label_names=label_keys)
    mod.bind(for_training=False, data_shapes=data.items(),
             label_shapes=labels, shared_module=existing_module)
    mod.set_params(arg_params, aux_params)
    return mod

def get_gluon_net_params_state(net_params):
    shapes = {k: net_params[k].data(net_params[k].list_ctx()[0]).shape for k, v in net_params.items()}
    data = {k: net_params[k].data(net_params[k].list_ctx()[0]).asnumpy().flatten() for k, v in net_params.items()}
    return {'data': data, 'shapes': shapes}

def load_net_params_from_state(net_params, state, ctx = None):
    net_params_dict = params_from_dict(state, ctx)
    for k in net_params_dict:
        #net_params[k].initialize(ctx)
        #import pdb; pdb.set_trace()
        #net_params[k].set_data(net_params_dict[k])
        net_params[k]._load_init(net_params_dict[k], ctx)
    return net_params
