# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ._tree_ensemble import convert_tree_ensemble as _convert_tree_ensemble
from ...models import MLModel as _MLModel
from coremltools import __version__ as ct_version
from coremltools.models import _METADATA_VERSION, _METADATA_SOURCE


def convert(
    model,
    feature_names=None,
    target="target",
    force_32bit_float=True,
    mode="regressor",
    class_labels=None,
    n_classes=None,
):
    """
    Convert a trained XGBoost model to Core ML format.

    Parameters
    ----------
    decision_tree : Booster
        A trained XGboost tree model.

    feature_names: [str] | str
        Names of input features that will be exposed in the Core ML model
        interface.

        Can be set to one of the following:

        - None for using the feature names from the model.
        - List of names of the input features that should be exposed in the
          interface to the Core ML model. These input features are in the same
          order as the XGboost model.

    target: str
        Name of the output feature name exposed to the Core ML model.

    force_32bit_float: bool
        If True, then the resulting CoreML model will use 32 bit floats internally.

    mode: str in ['regressor', 'classifier']
        Mode of the tree model.

    class_labels: list[int] or None
        List of classes. When set to None, the class labels are just the range from
        0 to n_classes - 1.

    n_classes: int or None
        Number of classes in classification. When set to None, the number of
        classes is expected from the model or class_labels should be provided.

    Returns
    -------
    model:MLModel
        Returns an MLModel instance representing a Core ML model.

    Examples
    --------
    .. sourcecode:: python

		# Convert it with default input and output names
   		>>> import coremltools
		>>> coreml_model = coremltools.converters.xgboost.convert(model)

		# Saving the Core ML model to a file.
		>>> coremltools.save('my_model.mlmodel')
    """
    model = _MLModel(
        _convert_tree_ensemble(
            model,
            feature_names,
            target,
            force_32bit_float=force_32bit_float,
            mode=mode,
            class_labels=class_labels,
            n_classes=n_classes,
        )
    )

    from xgboost import __version__ as xgboost_version

    model.user_defined_metadata[_METADATA_VERSION] = ct_version
    model.user_defined_metadata[_METADATA_SOURCE] = "xgboost=={0}".format(
        xgboost_version
    )

    return model
