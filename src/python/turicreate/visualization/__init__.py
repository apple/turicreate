# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
"""
Turi Create Visualization allows you to create, view, and save Plot objects.
The Plot class is used to create an immutable object representation of a
visualization and has the following methods:
- `show`: A method for displaying the Plot object.
- `save`: A method for saving the Plot object in a vega representation.

The `show` method, available on data structures in Turi Create as well as at
the top level in the namespace, streamlines the process of creating and
subsequently calling `show` on an automatically-selected Plot object.

Furthermore, Turi Create provides functionality to create independent plots
like scatter plots, categorical heatmaps, box plots, heatmaps,
columnwise summaries, and item frequency plots using the following API:

- scatter(x, y, xlabel, ylabel, title)
- categorical_heatmap(x, y, xlabel, ylabel, title)
- heatmap(x, y, xlabel, ylabel, title)
- box_plot(x, y, xlabel, ylabel, title)
- columnwise_summary(sf)
- histogram(sa, xlabel, ylabel, title)
- item_frequency(sa, xlabel, ylabel, title)

For more detailed information, please refer to the docstrings for each of
those classes/methods.

"""

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from .show import plot, show
from .show import scatter
from .show import categorical_heatmap
from .show import heatmap
from .show import box_plot
from .show import columnwise_summary
from .show import histogram
from .show import item_frequency
from ._plot import Plot, set_target, _get_client_app_path, LABEL_DEFAULT
