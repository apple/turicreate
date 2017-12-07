# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
import mxnet as _mx
import numpy as _np

class ManualScheduler(_mx.lr_scheduler.LRScheduler):
    def __init__(self, step, factor):
        super(ManualScheduler, self).__init__()
        assert isinstance(step, (list, tuple)) and len(step) >= 1
        assert isinstance(factor, (list, tuple)) and len(factor) == len(step)
        self.step = _np.array(step)
        self.cur_step_ind = 0
        self.factor = _np.array(factor)
        self.count = 0

    def __call__(self, num_update):
        w = _np.where(self.step > num_update)[0]
        if w.size == 0:
            f = self.base_lr
        else:
            f = self.base_lr * self.factor[w[0]]
        return f

