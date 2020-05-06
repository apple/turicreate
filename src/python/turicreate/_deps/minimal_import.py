# Copyright © 2020 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _  # noqa
from __future__ import division as _  # noqa
from __future__ import absolute_import as _  # noqa


# value can be set to True by build pipeline
USE_MINIMAL = False


def is_minimal_pkg():
    return USE_MINIMAL
