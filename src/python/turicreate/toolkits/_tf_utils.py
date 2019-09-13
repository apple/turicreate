# -*- coding: utf-8 -*-
# Copyright © 2019 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import numpy as np
import tensorflow as tf

def convert_conv1d_coreml_to_tf(conv_weights, tf_session):
	conv_weights = np.transpose(conv_weights, (3, 1, 0, 2))
