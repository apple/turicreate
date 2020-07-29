# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ...proto import Model_pb2 as _Model_pb2
from ...models._interface_management import set_regressor_interface_params
from ... import SPECIFICATION_VERSION

from ..._deps import _HAS_SKLEARN
from ...models import MLModel as _MLModel

if _HAS_SKLEARN:
    from ._sklearn_util import check_fitted
    from sklearn.svm import SVR as _SVR
    from . import _sklearn_util

    sklearn_class = _SVR

model_type = "regressor"

from ._svm_common import _set_kernel


def _generate_base_svm_regression_spec(model):
    """
    Takes an SVM regression model  produces a starting spec using the parts.
    that are shared between all SVMs.
    """
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )

    spec = _Model_pb2.Model()
    spec.specificationVersion = SPECIFICATION_VERSION
    svm = spec.supportVectorRegressor

    _set_kernel(model, svm)

    svm.rho = -model.intercept_[0]
    for i in range(len(model._dual_coef_)):
        for cur_alpha in model._dual_coef_[i]:
            svm.coefficients.alpha.append(cur_alpha)

    for cur_src_vector in model.support_vectors_:
        cur_dest_vector = svm.denseSupportVectors.vectors.add()
        for i in cur_src_vector:
            cur_dest_vector.values.append(i)
    return spec


def convert(model, features, target):
    """Convert a Support Vector Regressor (SVR) model to the protobuf spec.
    Parameters
    ----------
    model: SVR
        A trained SVR encoder model.

    feature_names: [str]
        Name of the input columns.

    target: str
        Name of the output column.

    Returns
    -------
    model_spec: An object of type Model_pb.
        Protobuf representation of the model
    """
    spec = _generate_base_svm_regression_spec(model)
    spec = set_regressor_interface_params(spec, features, target)
    return _MLModel(spec)


def get_input_dimension(model):
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )
    check_fitted(model, lambda m: hasattr(m, "support_vectors_"))
    return len(model.support_vectors_[0])
