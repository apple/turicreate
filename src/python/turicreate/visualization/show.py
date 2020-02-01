# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from ._plot import Plot, LABEL_DEFAULT
import turicreate as tc


def _get_title(title):
    if title == "":
        title = " "
    if title is None:
        title = ""

    return title


def plot(x, y, xlabel=LABEL_DEFAULT, ylabel=LABEL_DEFAULT, title=LABEL_DEFAULT):
    """
    Plots the data in `x` on the X axis and the data in `y` on the Y axis
    in a 2d visualization, and shows the resulting visualization.
    Uses the following heuristic to choose the visualization:

    * If `x` and `y` are both numeric (SArray of int or float), and they contain
      fewer than or equal to 5,000 values, show a scatter plot.
    * If `x` and `y` are both numeric (SArray of int or float), and they contain
      more than 5,000 values, show a heat map.
    * If `x` is numeric and `y` is an SArray of string, show a box and whisker
      plot for the distribution of numeric values for each categorical (string)
      value.
    * If `x` and `y` are both SArrays of string, show a categorical heat map.

    This show method supports SArrays of dtypes: int, float, str.

    Notes
    -----
    - The plot will be returned as a Plot object, which can then be shown,
      saved, etc. and will display automatically in a Jupyter Notebook.

    Parameters
    ----------
    x : SArray
      The data to plot on the X axis of a 2d visualization.
    y : SArray
      The data to plot on the Y axis of a 2d visualization. Must be the same
      length as `x`.
    xlabel : str (optional)
      The text label for the X axis. Defaults to "X".
    ylabel : str (optional)
      The text label for the Y axis. Defaults to "Y".
    title : str (optional)
      The title of the plot. Defaults to LABEL_DEFAULT. If the value is
      LABEL_DEFAULT, the title will be "<xlabel> vs. <ylabel>". If the value
      is None, the title will be omitted. Otherwise, the string passed in as the
      title will be used as the plot title.

    Examples
    --------
    Show a categorical heat map of pets and their feelings.

    >>> x = turicreate.SArray(['dog', 'cat', 'dog', 'dog', 'cat'])
    >>> y = turicreate.SArray(['happy', 'grumpy', 'grumpy', 'happy', 'grumpy'])
    >>> turicreate.show(x, y)


    Show a scatter plot of the function y = 2x, for x from 0 through 9, labeling
    the axes and plot title with custom strings.

    >>> x = turicreate.SArray(range(10))
    >>> y = x * 2
    >>> turicreate.show(x, y,
    ...                 xlabel="Custom X label",
    ...                 ylabel="Custom Y label",
    ...                 title="Custom title")

    """
    title = _get_title(title)
    if not (isinstance(x, tc.SArray)):
        raise ValueError("The X axis data should be an SArray.")
    if not (isinstance(y, tc.SArray)):
        raise ValueError("The Y axis data should be an SArray.")
    plt_ref = tc.extensions.plot(x, y, xlabel, ylabel, title)
    return Plot(_proxy=plt_ref)


def show(x, y, xlabel=LABEL_DEFAULT, ylabel=LABEL_DEFAULT, title=LABEL_DEFAULT):
    """
    Plots the data in `x` on the X axis and the data in `y` on the Y axis
    in a 2d visualization, and shows the resulting visualization.
    Uses the following heuristic to choose the visualization:

    * If `x` and `y` are both numeric (SArray of int or float), and they contain
      fewer than or equal to 5,000 values, show a scatter plot.
    * If `x` and `y` are both numeric (SArray of int or float), and they contain
      more than 5,000 values, show a heat map.
    * If `x` is numeric and `y` is an SArray of string, show a box and whisker
      plot for the distribution of numeric values for each categorical (string)
      value.
    * If `x` and `y` are both SArrays of string, show a categorical heat map.

    This show method supports SArrays of dtypes: int, float, str.

    Notes
    -----
    - The plot will render either inline in a Jupyter Notebook, in a web
      browser, or in a native GUI window, depending on the value provided in
      `turicreate.visualization.set_target` (defaults to 'auto').

    Parameters
    ----------
    x : SArray
      The data to plot on the X axis of a 2d visualization.
    y : SArray
      The data to plot on the Y axis of a 2d visualization. Must be the same
      length as `x`.
    xlabel : str (optional)
      The text label for the X axis. Defaults to "X".
    ylabel : str (optional)
      The text label for the Y axis. Defaults to "Y".
    title : str (optional)
      The title of the plot. Defaults to LABEL_DEFAULT. If the value is
      LABEL_DEFAULT, the title will be "<xlabel> vs. <ylabel>". If the value
      is None, the title will be omitted. Otherwise, the string passed in as the
      title will be used as the plot title.

    Examples
    --------
    Show a categorical heat map of pets and their feelings.

    >>> x = turicreate.SArray(['dog', 'cat', 'dog', 'dog', 'cat'])
    >>> y = turicreate.SArray(['happy', 'grumpy', 'grumpy', 'happy', 'grumpy'])
    >>> turicreate.show(x, y)


    Show a scatter plot of the function y = 2x, for x from 0 through 9, labeling
    the axes and plot title with custom strings.

    >>> x = turicreate.SArray(range(10))
    >>> y = x * 2
    >>> turicreate.show(x, y,
    ...                 xlabel="Custom X label",
    ...                 ylabel="Custom Y label",
    ...                 title="Custom title")

    """
    plot(x, y, xlabel, ylabel, title).show()


def scatter(x, y, xlabel=LABEL_DEFAULT, ylabel=LABEL_DEFAULT, title=LABEL_DEFAULT):
    """
    Plots the data in `x` on the X axis and the data in `y` on the Y axis
    in a 2d scatter plot, and returns the resulting Plot object.

    The function supports SArrays of dtypes: int, float.

    Parameters
    ----------
    x : SArray
      The data to plot on the X axis of the scatter plot.
      Must be numeric (int/float).
    y : SArray
      The data to plot on the Y axis of the scatter plot. Must be the same
      length as `x`.
      Must be numeric (int/float).
    xlabel : str (optional)
      The text label for the X axis. Defaults to "X".
    ylabel : str (optional)
      The text label for the Y axis. Defaults to "Y".
    title : str (optional)
      The title of the plot. Defaults to LABEL_DEFAULT. If the value is
      LABEL_DEFAULT, the title will be "<xlabel> vs. <ylabel>". If the value
      is None, the title will be omitted. Otherwise, the string passed in as the
      title will be used as the plot title.

    Returns
    -------
    out : Plot
      A :class: Plot object that is the scatter plot.

    Examples
    --------
    Make a scatter plot.

    >>> x = turicreate.SArray([1,2,3,4,5])
    >>> y = x * 2
    >>> scplt = turicreate.visualization.scatter(x, y)
    """
    if (
        not isinstance(x, tc.data_structures.sarray.SArray)
        or not isinstance(y, tc.data_structures.sarray.SArray)
        or x.dtype not in [int, float]
        or y.dtype not in [int, float]
    ):
        raise ValueError(
            "turicreate.visualization.scatter supports "
            + "SArrays of dtypes: int, float"
        )
    # legit input
    title = _get_title(title)
    plt_ref = tc.extensions.plot_scatter(x, y, xlabel, ylabel, title)
    return Plot(_proxy=plt_ref)


def categorical_heatmap(
    x, y, xlabel=LABEL_DEFAULT, ylabel=LABEL_DEFAULT, title=LABEL_DEFAULT
):
    """
    Plots the data in `x` on the X axis and the data in `y` on the Y axis
    in a 2d categorical heatmap, and returns the resulting Plot object.

    The function supports SArrays of dtypes str.

    Parameters
    ----------
    x : SArray
      The data to plot on the X axis of the categorical heatmap.
      Must be string SArray
    y : SArray
      The data to plot on the Y axis of the categorical heatmap.
      Must be string SArray and must be the same length as `x`.
    xlabel : str (optional)
      The text label for the X axis. Defaults to "X".
    ylabel : str (optional)
      The text label for the Y axis. Defaults to "Y".
    title : str (optional)
      The title of the plot. Defaults to LABEL_DEFAULT. If the value is
      LABEL_DEFAULT, the title will be "<xlabel> vs. <ylabel>". If the value
      is None, the title will be omitted. Otherwise, the string passed in as the
      title will be used as the plot title.

    Returns
    -------
    out : Plot
      A :class: Plot object that is the categorical heatmap.

    Examples
    --------
    Make a categorical heatmap.

    >>> x = turicreate.SArray(['1','2','3','4','5'])
    >>> y = turicreate.SArray(['a','b','c','d','e'])
    >>> catheat = turicreate.visualization.categorical_heatmap(x, y)
    """
    if (
        not isinstance(x, tc.data_structures.sarray.SArray)
        or not isinstance(y, tc.data_structures.sarray.SArray)
        or x.dtype != str
        or y.dtype != str
    ):
        raise ValueError(
            "turicreate.visualization.categorical_heatmap supports "
            + "SArrays of dtype: str"
        )
    # legit input
    title = _get_title(title)
    plt_ref = tc.extensions.plot_categorical_heatmap(x, y, xlabel, ylabel, title)
    return Plot(_proxy=plt_ref)


def heatmap(x, y, xlabel=LABEL_DEFAULT, ylabel=LABEL_DEFAULT, title=LABEL_DEFAULT):
    """
    Plots the data in `x` on the X axis and the data in `y` on the Y axis
    in a 2d heatmap, and returns the resulting Plot object.

    The function supports SArrays of dtypes int, float.

    Parameters
    ----------
    x : SArray
      The data to plot on the X axis of the heatmap.
      Must be numeric (int/float).
    y : SArray
      The data to plot on the Y axis of the heatmap.
      Must be numeric (int/float) and must be the same length as `x`.
    xlabel : str (optional)
      The text label for the X axis. Defaults to "X".
    ylabel : str (optional)
      The text label for the Y axis. Defaults to "Y".
    title : str (optional)
      The title of the plot. Defaults to LABEL_DEFAULT. If the value is
      LABEL_DEFAULT, the title will be "<xlabel> vs. <ylabel>". If the value
      is None, the title will be omitted. Otherwise, the string passed in as the
      title will be used as the plot title.

    Returns
    -------
    out : Plot
      A :class: Plot object that is the heatmap.

    Examples
    --------
    Make a heatmap.

    >>> x = turicreate.SArray([1,2,3,4,5])
    >>> y = x * 2
    >>> heat = turicreate.visualization.heatmap(x, y)
    """
    if (
        not isinstance(x, tc.data_structures.sarray.SArray)
        or not isinstance(y, tc.data_structures.sarray.SArray)
        or x.dtype not in [int, float]
        or y.dtype not in [int, float]
    ):
        raise ValueError(
            "turicreate.visualization.heatmap supports "
            + "SArrays of dtype: int, float"
        )
    title = _get_title(title)
    plt_ref = tc.extensions.plot_heatmap(x, y, xlabel, ylabel, title)
    return Plot(_proxy=plt_ref)


def box_plot(x, y, xlabel=LABEL_DEFAULT, ylabel=LABEL_DEFAULT, title=LABEL_DEFAULT):
    """
    Plots the data in `x` on the X axis and the data in `y` on the Y axis
    in a 2d box and whiskers plot, and returns the resulting Plot object.

    The function x as SArray of dtype str and y as SArray of dtype: int, float.

    Parameters
    ----------
    x : SArray
      The data to plot on the X axis of the box and whiskers plot.
      Must be an SArray with dtype string.
    y : SArray
      The data to plot on the Y axis of the box and whiskers plot.
      Must be numeric (int/float) and must be the same length as `x`.
    xlabel : str (optional)
      The text label for the X axis. Defaults to "X".
    ylabel : str (optional)
      The text label for the Y axis. Defaults to "Y".
    title : str (optional)
      The title of the plot. Defaults to LABEL_DEFAULT. If the value is
      LABEL_DEFAULT, the title will be "<xlabel> vs. <ylabel>". If the value
      is None, the title will be omitted. Otherwise, the string passed in as the
      title will be used as the plot title.

    Returns
    -------
    out : Plot
      A :class: Plot object that is the box and whiskers plot.

    Examples
    --------
    Make a box and whiskers plot.

    >>> bp = turicreate.visualization.box_plot(tc.SArray(['a','b','c','a','a']),tc.SArray([4.0,3.25,2.1,2.0,1.0]))
    """
    if (
        not isinstance(x, tc.data_structures.sarray.SArray)
        or not isinstance(y, tc.data_structures.sarray.SArray)
        or x.dtype != str
        or y.dtype not in [int, float]
    ):
        raise ValueError(
            "turicreate.visualization.box_plot supports "
            + "x as SArray of dtype str and y as SArray of dtype: int, float."
            + "\nExample: turicreate.visualization.box_plot(tc.SArray(['a','b','c','a','a']),tc.SArray([4.0,3.25,2.1,2.0,1.0]))"
        )
    title = _get_title(title)
    plt_ref = tc.extensions.plot_boxes_and_whiskers(x, y, xlabel, ylabel, title)
    return Plot(_proxy=plt_ref)


def columnwise_summary(sf):
    """
    Plots a columnwise summary of the sframe provided as input,
    and returns the resulting Plot object.

    The function supports SFrames.

    Parameters
    ----------
    sf : SFrame
      The data to get a columnwise summary for.

    Returns
    -------
    out : Plot
      A :class: Plot object that is the columnwise summary plot.

    Examples
    --------
    Make a columnwise summary of an SFrame.

    >>> x = turicreate.SArray([1,2,3,4,5])
    >>> s = turicreate.SArray(['a','b','c','a','a'])
    >>> sf_test = turicreate.SFrame([x,x,x,x,s,s,s,x,s,x,s,s,s,x,x])
    >>> colsum = turicreate.visualization.columnwise_summary(sf_test)
    """
    if not isinstance(sf, tc.data_structures.sframe.SFrame):
        raise ValueError(
            "turicreate.visualization.columnwise_summary " + "supports SFrame"
        )
    plt_ref = tc.extensions.plot_columnwise_summary(sf)
    return Plot(_proxy=plt_ref)


def histogram(sa, xlabel=LABEL_DEFAULT, ylabel=LABEL_DEFAULT, title=LABEL_DEFAULT):
    """
    Plots a histogram of the sarray provided as input, and returns the
    resulting Plot object.

    The function supports numeric SArrays with dtypes int or float.

    Parameters
    ----------
    sa : SArray
      The data to get a histogram for. Must be numeric (int/float).
    xlabel : str (optional)
      The text label for the X axis. Defaults to "Values".
    ylabel : str (optional)
      The text label for the Y axis. Defaults to "Count".
    title : str (optional)
      The title of the plot. Defaults to LABEL_DEFAULT. If the value is
      LABEL_DEFAULT, the title will be "<xlabel> vs. <ylabel>". If the value
      is None, the title will be omitted. Otherwise, the string passed in as the
      title will be used as the plot title.

    Returns
    -------
    out : Plot
      A :class: Plot object that is the histogram.

    Examples
    --------
    Make a histogram of an SArray.

    >>> x = turicreate.SArray([1,2,3,4,5,1,1,1,1,2,2,3,2,3,1,1,1,4])
    >>> hist = turicreate.visualization.histogram(x)
    """
    if not isinstance(sa, tc.data_structures.sarray.SArray) or sa.dtype not in [
        int,
        float,
    ]:
        raise ValueError(
            "turicreate.visualization.histogram supports "
            + "SArrays of dtypes: int, float"
        )
    title = _get_title(title)
    plt_ref = tc.extensions.plot_histogram(sa, xlabel, ylabel, title)
    return Plot(_proxy=plt_ref)


def item_frequency(sa, xlabel=LABEL_DEFAULT, ylabel=LABEL_DEFAULT, title=LABEL_DEFAULT):
    """
    Plots an item frequency of the sarray provided as input, and returns the
    resulting Plot object.

    The function supports SArrays with dtype str.

    Parameters
    ----------
    sa : SArray
      The data to get an item frequency for. Must have dtype str
    xlabel : str (optional)
      The text label for the X axis. Defaults to "Values".
    ylabel : str (optional)
      The text label for the Y axis. Defaults to "Count".
    title : str (optional)
      The title of the plot. Defaults to LABEL_DEFAULT. If the value is
      LABEL_DEFAULT, the title will be "<xlabel> vs. <ylabel>". If the value
      is None, the title will be omitted. Otherwise, the string passed in as the
      title will be used as the plot title.

    Returns
    -------
    out : Plot
      A :class: Plot object that is the item frequency plot.

    Examples
    --------
    Make an item frequency of an SArray.

    >>> x = turicreate.SArray(['a','ab','acd','ab','a','a','a','ab','cd'])
    >>> ifplt = turicreate.visualization.item_frequency(x)
    """
    if not isinstance(sa, tc.data_structures.sarray.SArray) or sa.dtype != str:
        raise ValueError(
            "turicreate.visualization.item_frequency supports " + "SArrays of dtype str"
        )
    title = _get_title(title)
    plt_ref = tc.extensions.plot_item_frequency(sa, xlabel, ylabel, title)
    return Plot(_proxy=plt_ref)
