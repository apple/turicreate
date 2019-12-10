# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
The primary file for converting Scikit-learn models.


"""

from ...models import _feature_management as _fm
from ...models import datatypes
from ...models.feature_vectorizer import create_feature_vectorizer
from ...models.pipeline import Pipeline, PipelineRegressor, PipelineClassifier

from ..._deps import HAS_SKLEARN

if HAS_SKLEARN:
    import sklearn as _sklearn
    from sklearn.pipeline import Pipeline as sk_Pipeline

from collections import namedtuple as _namedtuple
import numpy as _np
from six import string_types as _string_types

from . import _one_hot_encoder
from . import _dict_vectorizer
from . import _NuSVC
from . import _NuSVR
from . import _SVC
from . import _SVR
from . import _LinearSVC
from . import _LinearSVR
from . import _linear_regression
from . import _decision_tree_classifier
from . import _decision_tree_regressor
from . import _gradient_boosting_classifier
from . import _gradient_boosting_regressor
from . import _logistic_regression
from . import _normalizer
from . import _imputer
from . import _random_forest_classifier
from . import _random_forest_regressor
from . import _standard_scaler
from . import _k_neighbors_classifier

_PIPELINE_INTERNAL_FEATURE_NAME = "__feature_vector__"

_converter_module_list = [
        _dict_vectorizer,
        _one_hot_encoder,
        _normalizer,
        _standard_scaler,
        _imputer,
        _NuSVC,
        _NuSVR,
        _SVC,
        _SVR,
        _linear_regression,
        _LinearSVC,
        _LinearSVR,
        _logistic_regression,
        _random_forest_classifier,
        _random_forest_regressor,
        _decision_tree_classifier,
        _decision_tree_regressor,
        _gradient_boosting_classifier,
        _gradient_boosting_regressor,
        _k_neighbors_classifier]

def _test_module(m):
    assert m.model_type in ["transformer", "regressor", "classifier"], m.__name__
    if m.model_type == "transformer":
        assert hasattr(m, 'update_dimension'), m.__name__
    if m.model_type == "classifier":
        assert hasattr(m, "supports_output_scores"), m.__name__
        assert hasattr(m, "get_output_classes"), m.__name__
    assert hasattr(m, 'sklearn_class'), m.__name__
    assert hasattr(m, "get_input_dimension"), m.__name__

    return True

assert all(_test_module(m) for m in _converter_module_list)

_converter_lookup = dict( (md.sklearn_class, i) for i, md in enumerate(_converter_module_list))
_converter_functions = [md.convert for md in _converter_module_list]

def _get_converter_module(sk_obj):
    """
    Returns the module holding the conversion functions for a 
    particular model).
    """
    try:
        cv_idx = _converter_lookup[sk_obj.__class__]
    except KeyError:
        raise ValueError(
                "Transformer '%s' not supported; supported transformers are %s."
                % (repr(sk_obj),
                    ",".join(k.__name__ for k in _converter_module_list)))

    return _converter_module_list[cv_idx]

def _is_sklearn_model(sk_obj):
    if not(HAS_SKLEARN):
        raise RuntimeError('scikit-learn not found. scikit-learn conversion API is disabled.')
    from sklearn.pipeline import Pipeline as sk_Pipeline
    return (isinstance(sk_obj, sk_Pipeline) 
            or sk_obj.__class__ in _converter_lookup)

def _convert_sklearn_model(input_sk_obj, input_features = None,
                           output_feature_names = None, class_labels = None):
    """
    Converts a generic sklearn pipeline, transformer, classifier, or regressor 
    into an coreML specification.
    """
    if not(HAS_SKLEARN):
        raise RuntimeError('scikit-learn not found. scikit-learn conversion API is disabled.')
    from sklearn.pipeline import Pipeline as sk_Pipeline
    
    if input_features is None:
        input_features = "input"

    if isinstance(input_sk_obj, sk_Pipeline):
        sk_obj_list = input_sk_obj.steps
    else:
        sk_obj_list = [("SKObj", input_sk_obj)]

    if len(sk_obj_list) == 0:
        raise ValueError("No SKLearn transformers supplied.")

    # Put the transformers into a pipeline list to hold them so that they can 
    # later be added to a pipeline object.  (Hold off adding them to the 
    # pipeline now in case it's a single model at the end, in which case it
    # gets returned as is.)
    #
    # Each member of the pipeline list is a tuple of the proto spec for that 
    # model, the input features, and the output features.
    pipeline_list = []

    # These help us keep track of what's going on a bit easier. 
    Input = _namedtuple('InputTransformer', ['name', 'sk_obj', 'module'])
    Output = _namedtuple('CoreMLTransformer', ['spec', 'input_features', 'output_features'])


    # Get a more information rich representation of the list for convenience. 
    # obj_list is a list of tuples of (name, sk_obj, and the converter module for 
    # that step in the list.
    obj_list = [ Input(sk_obj_name, sk_obj, _get_converter_module(sk_obj))
                for sk_obj_name, sk_obj in sk_obj_list]


    # Various preprocessing steps.

    # If the first component of the object list is the sklearn dict vectorizer,
    # which is unique in that it accepts a list of dictionaries, then we can
    # get the feature type mapping from that.  This then may require the addition
    # of several OHE steps, so those need to be processed in the first stage.
    if isinstance(obj_list[0].sk_obj, _dict_vectorizer.sklearn_class):

        dv_obj = obj_list[0].sk_obj
        output_dim = len(_dict_vectorizer.get_input_feature_names(dv_obj))
 
        if not isinstance(input_features, _string_types):
            raise TypeError("If the first transformer in a pipeline is a "
                            "DictVectorizer, then the input feature must be the name "
                            "of the input dictionary.")

        input_features = [(input_features, datatypes.Dictionary(str))]
       
        if len(obj_list) > 1:
            output_feature_name = _PIPELINE_INTERNAL_FEATURE_NAME 

        else:
            if output_feature_names is None:
                output_feature_name = "transformed_features"

            elif isinstance(output_feature_names, _string_types):
                output_feature_name = output_feature_names
            
            else:
                raise TypeError(
                    "For a transformer pipeline, the "
                    "output_features needs to be None or a string "
                    "for the predicted value.")
 
        output_features = [(output_feature_name, datatypes.Array(output_dim))]

        spec = _dict_vectorizer.convert(dv_obj, input_features, output_features)._spec
        pipeline_list.append(Output(spec, input_features, output_features) )

        # Set up the environment for the rest of the pipeline
        current_input_features = output_features
        current_num_dimensions = output_dim
    
        # In the corner case that it's only the dict vectorizer here, just return
        # and exit with that at this point. 
        if len(obj_list) == 1:
            return spec
        else:
            del obj_list[0]

    else: 

        # First, we need to resolve the input feature types as the sklearn pipeline
        # expects just an array as input, but what we want to expose to the coreML
        # user is an interface with named variables.  This resolution has to handle 
        # a number of cases. 

        # Can we get the number of features from the model?  If so, pass that
        # information into the feature resolution function.  If we can't, then this 
        # function should return None.
        first_sk_obj = obj_list[0].sk_obj
        num_dimensions = _get_converter_module(first_sk_obj).get_input_dimension(first_sk_obj)
        # Resolve the input features.  
        features = _fm.process_or_validate_features(input_features, num_dimensions)
        current_num_dimensions = _fm.dimension_of_array_features(features)

        # Add in a feature vectorizer that consolodates all of the feature inputs
        # into the form expected by scipy's pipelines.  Essentially this is a
        # translation layer between the coreML form with named arguments and the
        # scikit learn variable form.
        if len(features) == 1 and isinstance(features[0][1], datatypes.Array):
            current_input_features = features
        else:
            spec, _output_dimension = create_feature_vectorizer(
                    features, _PIPELINE_INTERNAL_FEATURE_NAME)

            assert _output_dimension == current_num_dimensions
            ft_out_features = [(_PIPELINE_INTERNAL_FEATURE_NAME, 
                                datatypes.Array(current_num_dimensions))]
            pipeline_list.append( Output(spec, features, ft_out_features) )
            current_input_features = ft_out_features

    # Now, validate the sequence of transformers to make sure we have something
    # that can work with all of this.
    for i, (_, _, m) in enumerate(obj_list[:-1]):
        if m.model_type != "transformer":
            raise ValueError("Only a sequence of transformer classes followed by a "
                    "single transformer, regressor, or classifier is currently supported. "
                    "(object in position %d interpreted as %s)" % (i, m.model_type))

    overall_mode = obj_list[-1].module.model_type
    assert overall_mode in ('transformer', 'regressor', 'classifier')

    # Now, go through each transformer in the sequence of transformers and add
    # it to the pipeline.
    for _, sk_obj, sk_m in obj_list[: -1]:

        next_dimension = sk_m.update_dimension(sk_obj, current_num_dimensions)

        output_features = [(_PIPELINE_INTERNAL_FEATURE_NAME, 
                            datatypes.Array(next_dimension))]
        spec = sk_m.convert(sk_obj, current_input_features, output_features)._spec

        pipeline_list.append( Output(spec, current_input_features, output_features))

        current_input_features = output_features
        current_num_dimensions = next_dimension


    # Now, handle the final transformer.  This is where we need to have different
    # behavior depending on whether it's a classifier, transformer, or regressor.
    _, last_sk_obj, last_sk_m = obj_list[-1]

    if overall_mode == "classifier":
        supports_output_scores = last_sk_m.supports_output_scores(last_sk_obj)
        _internal_output_classes = list(last_sk_m.get_output_classes(last_sk_obj))

        if class_labels is None:
            class_labels = _internal_output_classes

        output_features = _fm.process_or_validate_classifier_output_features(
                output_feature_names, class_labels, supports_output_scores)

    elif overall_mode == "regressor":
        if output_feature_names is None:
            output_features = [("prediction", datatypes.Double())]
        elif isinstance(output_feature_names, _string_types):
            output_features = [(output_feature_names, datatypes.Double())]
        else:
            raise TypeError("For a regressor object or regressor pipeline, the "
                            "output_features needs to be None or a string for the predicted value.")

    else:   # transformer
        final_output_dimension = last_sk_m.update_dimension(last_sk_obj, current_num_dimensions)

        if output_feature_names is None:
            output_features = [("transformed_features",
                                datatypes.Array(final_output_dimension))]

        elif isinstance(output_feature_names, _string_types):
            output_features = [(output_feature_names, datatypes.Array(final_output_dimension))]

        else:
            raise TypeError("For a transformer object or transformer pipeline, the "
                            "output_features needs to be None or a string for the "
                            "name of the transformed value.")

    last_spec = last_sk_m.convert(last_sk_obj, current_input_features, output_features)._spec

    pipeline_list.append( Output(last_spec, current_input_features, output_features) )

    # Now, create the pipeline and return the spec for it.

    # If it's just one element, we can return it.
    if len(pipeline_list) == 1:
        return pipeline_list[0].spec

    original_input_features = pipeline_list[0].input_features

    if overall_mode == 'regressor':
        pipeline = PipelineRegressor(original_input_features, output_features)

    elif overall_mode == 'classifier':
        pipeline = PipelineClassifier(original_input_features,
                    class_labels, output_features)

    else:
        pipeline = Pipeline(original_input_features, output_features)

    # Okay, now we can build the pipeline spec.
    for spec, input_features, output_features in pipeline_list:
        pipeline.add_model(spec)

    return pipeline.spec

