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
from turicreate.toolkits._internal_utils import (_numeric_param_check_range,
                                                 _mac_ver)


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


def get_gpu_ids_in_use(max_devices=None):
    from turicreate.util import _CUDA_GPUS
    gpus = _CUDA_GPUS
    num_gpus = get_num_gpus_in_use(max_devices=max_devices)
    if num_gpus >= 0:
        gpus = gpus[:num_gpus]
    if max_devices is not None:
        gpus = gpus[:max_devices]
    return [gpu_info['index'] for gpu_info in gpus]


def get_mxnet_context(max_devices=None):
    from turicreate.util import _CUDA_GPUS
    import mxnet as _mx
    assert_valid_num_gpus()
    num_gpus = _tc_config.get_num_gpus()
    if num_gpus == 0 or not _CUDA_GPUS:
        return [_mx.cpu()]
    else:
        gpu_indices = [gpu['index'] for gpu in _CUDA_GPUS]
        if num_gpus > 0:
            gpu_indices = gpu_indices[:num_gpus]

        ctx = [_mx.gpu(index) for index in gpu_indices]

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


def get_gpus_in_use(max_devices=None):
    """
    Like get_num_gpus_in_use, but returns a list of dictionaries with just
    queried GPU information.
    """
    from turicreate.util import _get_cuda_gpus
    gpu_indices = get_gpu_ids_in_use(max_devices=max_devices)
    gpus = _get_cuda_gpus()
    return [gpus[index] for index in gpu_indices]


def assert_valid_num_gpus():
    from turicreate.util import _CUDA_GPUS
    num_gpus = _tc_config.get_num_gpus()
    if not _CUDA_GPUS and _sys.platform == 'darwin':
        # GPU acceleration requires macOS 10.14+
        if num_gpus == 1 and _mac_ver() < (10, 14):
            raise _ToolkitError('GPU acceleration requires at least macOS 10.14')
        elif num_gpus >= 2:
            raise _ToolkitError('Using more than one GPU is currently not supported on Mac')
    _numeric_param_check_range('num_gpus', num_gpus, -1, _six.MAXSIZE)


def _warn_if_less_than_cuda_free_memory(mem_mb, max_devices=None, has_batch_size_setting=True):
    # Note, we cannot use _CUDA_GPUS, since we need an up-to-date snapshot of
    # the free memory
    import numpy as np
    from turicreate.util import _get_cuda_gpus
    import time
    import textwrap
    gpus = _get_cuda_gpus()
    num_gpus = get_num_gpus_in_use(max_devices=max_devices)
    gpus = gpus[:num_gpus]
    capable = all(gpu['memory_total'] > mem_mb for gpu in gpus)
    available = all(gpu['memory_free'] > mem_mb for gpu in gpus)
    warning = None
    if capable and not available:
        # Get GPU with the least amount of free memroy
        gpu_mems = [gpu['memory_free'] for gpu in gpus]
        gpu_idx = np.argmin(gpu_mems)

        warning = '''
        You may not have enough free GPU memory to create this model, which may
        result in a fatal crash or lower performance. We recommend that you
        have at least {mem_req} MiB available on each GPU, while currently you
        have only {mem_free:.0f} MiB free (out of {mem_total:.0f} MiB) on GPU
        #{index:d}. Please close all other processes that may be using this
        GPU.
        '''.format(mem_req=mem_mb,
                   mem_free=gpus[gpu_idx]['memory_free'],
                   mem_total=gpus[gpu_idx]['memory_total'],
                   index=gpus[gpu_idx]['index'])

    elif not capable:
        # Get GPU with the least amount of total memroy
        gpu_mems = [gpu['memory_total'] for gpu in gpus]
        gpu_idx = np.argmin(gpu_mems)

        warning = '''
        You may not have a capable enough GPU to create this model with the
        current settings, which may result in a fatal crash or lower
        performance. We recommend that you have at least {mem_req} MiB on each
        requested GPU, while currently GPU #{index} has only {mem_total:.0f}
        MiB of total memory.
        '''.format(mem_req=mem_mb,
                   mem_total=gpus[gpu_idx]['memory_total'],
                   index=gpus[gpu_idx]['index'])

        if has_batch_size_setting:
            warning += 'Please try lowering the batch size by passing the parameter batch_size to create().'

    if warning:
        print('WARNING:')
        print()
        # Re-wrap text
        lines = textwrap.wrap(textwrap.dedent(warning).lstrip(), width=75)
        for line in lines:
            print('   ', line)
        print()
        time.sleep(3)


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
        net_params[k]._load_init(net_params_dict[k], ctx)
    return net_params
