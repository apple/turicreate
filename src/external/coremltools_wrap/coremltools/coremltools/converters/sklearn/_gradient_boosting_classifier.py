# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ._tree_ensemble import convert_tree_ensemble as _convert_tree_ensemble
from ._tree_ensemble import get_input_dimension

from ..._deps import HAS_SKLEARN as _HAS_SKLEARN
from ...models import MLModel as _MLModel

if _HAS_SKLEARN:
    import sklearn.ensemble as _ensemble
    from . import _sklearn_util
    sklearn_class = _ensemble.GradientBoostingClassifier

model_type = 'classifier'

def convert(model, feature_names, target):
    """Convert a boosted tree model to protobuf format.

    Parameters
    ----------
    decision_tree : GradientBoostingClassifier
        A trained scikit-learn tree model.

    feature_names: [str]
        Name of the input columns.

    target: str
        Name of the output column.

    Returns
    -------
    model_spec: An object of type Model_pb.
        Protobuf representation of the model
    """
    if not(_HAS_SKLEARN):
        raise RuntimeError('scikit-learn not found. scikit-learn conversion API is disabled.')
    
    _sklearn_util.check_expected_type(model, _ensemble.GradientBoostingClassifier)
    def is_gbr_model(m):
        if len(m.estimators_) == 0:
            return False
        if hasattr(m, 'estimators_') and m.estimators_ is not None:
            for t in m.estimators_.flatten():
                if not hasattr(t, 'tree_') or t.tree_ is None:
                    return False
            return True
        else:
            return False
    _sklearn_util.check_fitted(model, is_gbr_model)
    post_evaluation_transform = None
    if model.n_classes_ == 2:
        base_prediction = [model.init_.prior]
        post_evaluation_transform = 'Regression_Logistic'
    else:
        base_prediction = list(model.init_.priors)
        post_evaluation_transform = 'Classification_SoftMax'
    return _MLModel(_convert_tree_ensemble(model, feature_names, target, mode = 'classifier',
            base_prediction = base_prediction, class_labels = model.classes_,
            post_evaluation_transform = post_evaluation_transform))

def supports_output_scores(model):
    return True

def get_output_classes(model):
    return list(model.classes_)


