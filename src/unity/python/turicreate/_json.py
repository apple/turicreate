# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import array as _array
import datetime as _datetime
import json as _json

def to_serializable(obj):
    from . import extensions
    return extensions.json.to_serializable(obj)

def from_serializable(data, schema):
    from . import extensions
    return extensions.json.from_serializable(data, schema)

def dumps(obj):
    """
    Dumps a serializable object to JSON. This API maps to the Python built-in
    json dumps method, with a few differences:

    * The return value is always valid JSON according to RFC 7159.
    * The input can be any of the following types:
        - SFrame
        - SArray
        - SGraph
        - single flexible_type (Image, int, long, float, datetime.datetime)
        - recursive flexible_type (list, dict, array.array)
        - recursive variant_type (list or dict of all of the above)
    * Serialized result includes both data and schema. Deserialization requires
      valid schema information to disambiguate various other wrapped types
      (like Image) from dict.
    """
    from . import extensions
    (data, schema) = to_serializable(obj)
    return _json.dumps({'data': data, 'schema': schema})

def loads(json_string):
    """
    Loads a serializable object from JSON. This API maps to the Python built-in
    json loads method, with a few differences:

    * The input string must be valid JSON according to RFC 7159.
    * The input must represent a serialized result produced by the `dumps`
      method in this module, including both data and schema.
      If it does not the result will be unspecified and may raise exceptions.
    """
    from . import extensions
    result = _json.loads(json_string)
    return from_serializable(**result)
