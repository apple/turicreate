#  Copyright (c) 2020, Apple Inc. All rights reserved.
#
#  Use of this source code is governed by a BSD-3-clause license that can be
#  found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from coremltools.converters.mil import testing_reqs
from coremltools.converters.mil.testing_reqs import *
from coremltools.converters.mil.frontend.tensorflow.test.testing_utils import (
    make_tf_graph,
    run_compare_tf,
    layer_counts,
)
import math

backends = testing_reqs.backends

tf = pytest.importorskip("tensorflow")


class TestTF1Graphs:

    @pytest.mark.parametrize(
        "use_cpu_only, backend", itertools.product([True, False], backends)
    )
    def test_masked_input(self, use_cpu_only, backend):

        input_shape = [4, 10, 8]
        val = np.random.rand(*input_shape).astype(np.float32)

        @make_tf_graph([input_shape])
        def build_model(input):
            sliced_input = input[..., 4]
            mask = tf.where_v2(sliced_input > 0)
            masked_input = tf.gather_nd(input, mask)
            return masked_input

        model, inputs, outputs = build_model

        input_values = [val]
        input_dict = dict(zip(inputs, input_values))
        run_compare_tf(
            model,
            input_dict,
            outputs,
            use_cpu_only=use_cpu_only,
            frontend_only=False,
            backend=backend,
        )
