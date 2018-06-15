# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
#
# Turi Create Visualization allows you to create, view, and save Plot objects.
#
# class Plot
#
# 	An immutable object representation of a visualization.
#
#   Notes
#   -----
#   - A plot object is returned via the SFrame's or SArray's .plot() method
#
#   Examples
#   --------
#   Suppose 'plt' is an Plot Object
#
#   We can view it using:
#   >>> plt.show()
#
#   We can save it using:
#   >>> plt.save('vega_spec.json')
#
#   We can also save the vega representation of the plot without data:
#   >>> plt.save('vega_spec.json', False)
#
#   We can save the plot as a PNG/SVG using:
#   >>> plt.save('test.png')
#   >>> plt.save('test.svg')
#
# show
# 
# 	A method for displaying the Plot object. The plot will render either inline
# 	in a Jupyter Notebook, or in a native GUI window, depending on the value 
# 	provided in `turicreate.visualization.set_target` (defaults to 'auto').
# 
# save
#
# 	A method for saving the Plot object in a vega representation
#
#   Parameters
#	----------
#   filepath: string
#   The destination filepath where the plot object must be saved as.
#   The extension of this filepath determines what format the plot will
#   be saved as. Currently supported formats are JSON, PNG, and SVG.
# 
# Moreover, Turi Create Visualization also allows you to call `show` 
# on data sources like SFrames and SArrays, and provides functionality to 
# create independent plots like scatter plots, categorical heatmaps, box plots, 
# heatmaps, columnwise summaries, and item frequency plots.
#
#
# show
# 
# 	Plots the data in `x` on the X axis and the data in `y` on the Y axis
# 	in a 2d visualization, and shows the resulting visualization.
#	Uses the following heuristic to choose the visualization:

# 	* If `x` and `y` are both numeric (SArray of int or float), and they contain
#     fewer than or equal to 5,000 values, show a scatter plot.
# 	* If `x` and `y` are both numeric (SArray of int or float), and they contain
#     more than 5,000 values, show a heat map.
# 	* If `x` is numeric and `y` is an SArray of string, show a box and whisker
#     plot for the distribution of numeric values for each categorical (string)
#     value.
# 	* If `x` and `y` are both SArrays of string, show a categorical heat map.

# 	This show method supports SArrays of dtypes: int, float, str.

# 	Notes
# 	-----
# 	- The plot will render either inline in a Jupyter Notebook, or in a
#     native GUI window, depending on the value provided in
#     `turicreate.visualization.set_target` (defaults to 'auto').

# 	Parameters
# 	----------
# 	x : SArray
#  		The data to plot on the X axis of a 2d visualization.
# 	y : SArray
#   	The data to plot on the Y axis of a 2d visualization. Must be the same
#   	length as `x`.
# 	xlabel : str (optional)
#   	The text label for the X axis. Defaults to "X".
# 	ylabel : str (optional)
#   	The text label for the Y axis. Defaults to "Y".
# 	title : str (optional)
#   	The title of the plot. Defaults to None. If the value is None, the title
#   	will be "<xlabel> vs. <ylabel>". Otherwise, the string passed in as the
#   	title will be used as the plot title.

# 	Examples
# 	--------
# 	Show a categorical heat map of pets and their feelings.

# 	>>> x = turicreate.SArray(['dog', 'cat', 'dog', 'dog', 'cat'])
# 	>>> y = turicreate.SArray(['happy', 'grumpy', 'grumpy', 'happy', 'grumpy'])
# 	>>> turicreate.show(x, y)
# 	Show a scatter plot of the function y = 2x, for x from 0 through 9, labeling
# 	the axes and plot title with custom strings.

# 	>>> x = turicreate.SArray(range(10))
# 	>>> y = x * 2
# 	>>> turicreate.show(x, y,
# 	...                 xlabel="Custom X label",
# 	...                 ylabel="Custom Y label",
# 	...                 title="Custom title")

from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from .show import show
from ._plot import Plot, set_target
from .show import _get_client_app_path
from .show import scatter
from .show import categorical_heatmap
from .show import heatmap
from .show import box_plot
from .show import columnwise_summary
from .show import histogram
from .show import item_frequency
