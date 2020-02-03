# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause


# Standard lib functions would be great here, but the formatting options of
# timedelta are not great
def _seconds_as_string(seconds):
    """
    Returns seconds as a human-friendly string, e.g. '1d 4h 47m 41s'
    """
    TIME_UNITS = [("s", 60), ("m", 60), ("h", 24), ("d", None)]
    unit_strings = []
    cur = max(int(seconds), 1)
    for suffix, size in TIME_UNITS:
        if size is not None:
            cur, rest = divmod(cur, size)
        else:
            rest = cur
        if rest > 0:
            unit_strings.insert(0, "%d%s" % (rest, suffix))
    return " ".join(unit_strings)
