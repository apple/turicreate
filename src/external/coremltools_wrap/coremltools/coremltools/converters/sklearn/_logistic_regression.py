# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from collections import Iterable

from ..._deps import _HAS_SKLEARN
from ...models import MLModel as _MLModel

if _HAS_SKLEARN:
    from sklearn.linear_model import LogisticRegression
    from . import _sklearn_util

    sklearn_class = LogisticRegression

from ... import SPECIFICATION_VERSION
from ...models._interface_management import set_classifier_interface_params
from ...proto import Model_pb2 as _Model_pb2

model_type = "classifier"


def convert(model, feature_names, target):
    """Convert a Logistic Regression model to the protobuf spec.
    Parameters
    ----------
    model: LogisticRegression
        A trained LogisticRegression model.

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

    _sklearn_util.check_expected_type(model, LogisticRegression)
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "coef_"))

    return _MLModel(_convert(model, feature_names, target))


def _convert(model, feature_names, target):
    spec = _Model_pb2.Model()
    spec.specificationVersion = SPECIFICATION_VERSION

    set_classifier_interface_params(
        spec, feature_names, model.classes_, "glmClassifier", output_features=target
    )

    glmClassifier = spec.glmClassifier

    if model.multi_class == "ovr":
        glmClassifier.classEncoding = glmClassifier.OneVsRest
    else:
        print(
            '[ERROR] Currently "One Vs Rest" is the only supported multiclass option.'
        )
        return None

    glmClassifier.postEvaluationTransform = glmClassifier.Logit

    if isinstance(model.intercept_, Iterable):
        for val in model.intercept_:
            glmClassifier.offset.append(val)
    else:
        for _ in model.coef_:
            glmClassifier.offset.append(model.intercept_)

    for cur_in_row in model.coef_:
        cur_out_row = glmClassifier.weights.add()
        for val in cur_in_row:
            cur_out_row.value.append(val)

    return spec


def supports_output_scores(model):
    return True


def get_output_classes(model):
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "coef_"))
    return list(model.classes_)


def get_input_dimension(model):
    if not (_HAS_SKLEARN):
        raise RuntimeError(
            "scikit-learn not found. scikit-learn conversion API is disabled."
        )
    _sklearn_util.check_fitted(model, lambda m: hasattr(m, "coef_"))
    return len(model.coef_[0])
