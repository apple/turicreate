# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import absolute_import as _
from __future__ import print_function as _
from __future__ import division as _

_mxnet_module = None

class _MXNetWrapper(object):

    def __getattribute__(self, attr):
        global _mxnet_module

        if _mxnet_module is None:
            import mxnet

            version_tuple = tuple(int(x) for x in mxnet.__version__.split('.') if x.isdigit())
            lowest_version = (0, 11, 0)
            not_yet_supported_version = (1, 2, 0)
            recommended_version_str = '1.1.0'
            if not (lowest_version <= version_tuple < not_yet_supported_version):
                print('WARNING: You are using MXNet', _mx.__version__, 'which may result in breaking behavior.')
                print('         To fix this, please install the currently recommended version:')
                print()
                print('             pip uninstall -y mxnet && pip install mxnet==%s' % recommended_version_str)
                print()
                print("         If you want to use a CUDA GPU, then change 'mxnet' to 'mxnet-cu90' (adjust 'cu90' depending on your CUDA version):")
                print()

            _mxnet_module = mxnet

        return getattr(_mxnet_module, attr)

mxnet = _MXNetWrapper()
