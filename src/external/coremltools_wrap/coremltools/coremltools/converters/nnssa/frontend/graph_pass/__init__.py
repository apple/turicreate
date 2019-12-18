# -*- coding: utf-8 -*-
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from .type_inference import type_inference_pass
from .delete_constant import delete_unnecessary_constant_nodes
from .identity_outputs import add_identity_outputs
from .trace_constants import trace_constants
from .remove_identities import remove_identities
from .common_symbolic_value_elimination import common_symbolic_value_elimination
from .shift_global import shift_get_global_to_set_global

from .remove_unused_nodes import remove_unused_nodes
