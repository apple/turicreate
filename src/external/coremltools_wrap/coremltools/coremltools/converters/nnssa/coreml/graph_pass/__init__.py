# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from .op_removals import remove_no_ops_and_shift_control_dependencies
from .op_removals import constant_weight_link_removal
from .op_removals import remove_single_isolated_node
from .op_removals import remove_identity, remove_oneway_split

from .op_fusions import fuse_bias_add, transform_nhwc_to_nchw, \
    onehot_matmul_to_embedding, fuse_layer_norm, fuse_gelu, \
    fuse_conv_mul_add_into_batchnorm, fuse_pad_into_conv, \
    spatial_reduce_to_global_pool

from .mlmodel_passes import remove_disconnected_constants
