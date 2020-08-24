# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from . import _SVC as _SVC

from ..._deps import _HAS_SKLEARN

if _HAS_SKLEARN:
    from ._sklearn_util import check_fitted
    from . import _sklearn_util
    from sklearn.svm import NuSVC as _NuSVC

    sklearn_class = _NuSVC

model_type = "classifier"


def convert(model, feature_names, target):
    """Convert a Nu-Support Vector Classification (NuSVC) model to the protobuf spec.
    Parameters
    ----------
    model: NuSVC
        A trained NuSVC encoder model.

    feature_names: [str], optional (default=None)
        Name of the input columns.

    target: str, optional (default=None)
        Name of the output column.

    Returns
    -------
    model_spec: An object of type Model_pb.
        Protobuf representation of the model
    """

    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )

    _sklearn_util.check_expected_type(model, _NuSVC)
    return _SVC.convert(model, feature_names, target)


def supports_output_scores(model):
    return _SVC.supports_output_scores(model)


def get_output_classes(model):
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )
    check_fitted(model, lambda m: hasattr(m, "support_vectors_"))
    return _SVC.get_output_classes(model)


def get_input_dimension(model):
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )

    check_fitted(model, lambda m: hasattr(m, "support_vectors_"))
    return _SVC.get_input_dimension(model)
