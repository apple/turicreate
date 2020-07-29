import coremltools as ct
import pytest


def _get_visible_items(d):
    return [x for x in dir(d) if not x.startswith("_")]


def _check_visible_modules(actual, expected):
    assert set(actual) == set(expected), "API mis-matched. Got %s, expected %s" % (
        actual,
        expected,
    )


class TestApiVisibilities:
    """Test public coremltools API visibilities."""

    def test_top_level(self):
        expected = [
            "ClassifierConfig",
            "EnumeratedShapes",
            "ImageType",
            "RangeDim",
            "SPECIFICATION_VERSION",
            "Shape",
            "TensorType",
            "convert",
            "converters",
            "libcoremlpython",
            "models",
            "proto",
            "target",
            "utils",
            "version",
            "test",
        ]
        if not ct.utils._is_macos():
             expected.remove("libcoremlpython")
        _check_visible_modules(_get_visible_items(ct), expected)

    def test_utils(self):
        expected = [
            "convert_double_to_float_multiarray_type",
            "convert_neural_network_spec_weights_to_fp16",
            "convert_neural_network_weights_to_fp16",
            "evaluate_classifier",
            "evaluate_classifier_with_probabilities",
            "evaluate_regressor",
            "evaluate_transformer",
            "load_spec",
            "rename_feature",
            "save_spec",
        ]
        _check_visible_modules(_get_visible_items(ct.utils), expected)

    def test_models(self):
        expected = [
            "MLModel",
            "datatypes",
            "model",
            "neural_network",
            "pipeline",
            "tree_ensemble",
            "utils",
            "nearest_neighbors",
            "feature_vectorizer",
        ]
        _check_visible_modules(_get_visible_items(ct.models), expected)

    def test_models_mlmodel(self):
        expected = [
            "author",
            "get_spec",
            "input_description",
            "license",
            "output_description",
            "predict",
            "save",
            "short_description",
            "user_defined_metadata",
            "version",
        ]
        _check_visible_modules(_get_visible_items(ct.models.MLModel), expected)

    def test_models_neural_network(self):
        expected = [
            "AdamParams",
            "NeuralNetworkBuilder",
            "SgdParams",
            "builder",
            "datatypes",
            "flexible_shape_utils",
            "optimization_utils",
            "printer",
            "quantization_utils",
            "set_training_features",
            "set_transform_interface_params",
            "spec_inspection_utils",
            "update_optimizer_utils",
            "utils",
        ]
        _check_visible_modules(_get_visible_items(ct.models.neural_network), expected)

    def test_models_neural_network_utils(self):
        expected = ["NeuralNetworkBuilder", "make_image_input", "make_nn_classifier"]
        _check_visible_modules(
            _get_visible_items(ct.models.neural_network.utils), expected
        )

    def test_models_tree_ensemble(self):
        expected = [
            "TreeEnsembleBase",
            "TreeEnsembleClassifier",
            "TreeEnsembleRegressor",
            "set_classifier_interface_params",
            "set_regressor_interface_params",
        ]
        _check_visible_modules(_get_visible_items(ct.models.tree_ensemble), expected)

    def test_models_pipeline(self):
        expected = [
            "Pipeline",
            "PipelineClassifier",
            "PipelineRegressor",
            "set_classifier_interface_params",
            "set_regressor_interface_params",
            "set_training_features",
            "set_transform_interface_params",
        ]
        _check_visible_modules(_get_visible_items(ct.models.pipeline), expected)

    def test_converters(self):
        expected = [
            "ClassifierConfig",
            "EnumeratedShapes",
            "ImageType",
            "RangeDim",
            "Shape",
            "TensorType",
            "caffe",
            "convert",
            "keras",
            "libsvm",
            "mil",
            "onnx",
            "sklearn",
            "xgboost",
        ]
        _check_visible_modules(_get_visible_items(ct.converters), expected)

    def test_converters_caffe(self):
        _check_visible_modules(_get_visible_items(ct.converters.caffe), ["convert"])

    @pytest.mark.skipif(
        ct.utils._python_version() >= (3, 8, 0),
        reason="Keras isn't compatible with Python 3.8+.",
    )
    @pytest.mark.xfail(
         condition=not ct.utils._is_macos(),
         reason="rdar://65138103 (Keras converter not exposed on Linux)",
         run=False,
     )
    def test_converters_keras(self):
        _check_visible_modules(_get_visible_items(ct.converters.keras), ["convert"])

    def test_converters_libsvm(self):
        _check_visible_modules(_get_visible_items(ct.converters.libsvm), ["convert"])

    @pytest.mark.skipif(
        ct.utils._python_version() >= (3, 8, 0),
        reason="ONNX isn't compatible with Python 3.8+.",
    )
    def test_converters_onnx(self):
        _check_visible_modules(_get_visible_items(ct.converters.onnx), ["convert"])

    def test_converters_sklearn(self):
        _check_visible_modules(_get_visible_items(ct.converters.sklearn), ["convert"])

    def test_converters_xgboost(self):
        _check_visible_modules(_get_visible_items(ct.converters.xgboost), ["convert"])

    def test_converters_mil(self):
        pass  # TODO: [Create API visibility tests for MIL](rdar://64413959)

    def test_models_neural_network_quantization_utils(self):
        expected = [
            "AdvancedQuantizedLayerSelector",
            "MatrixMultiplyLayerSelector",
            "ModelMetrics",
            "NoiseMetrics",
            "OutputMetric",
            "QuantizedLayerSelector",
            "TopKMetrics",
            "activate_int8_int8_matrix_multiplications",
            "compare_models",
            "quantize_weights",
        ]
        _check_visible_modules(
            _get_visible_items(ct.models.neural_network.quantization_utils), expected
        )

    def test_models_neural_network_flexible_shape_utils(self):
        expected = [
            "NeuralNetworkImageSize",
            "NeuralNetworkImageSizeRange",
            "NeuralNetworkMultiArrayShape",
            "NeuralNetworkMultiArrayShapeRange",
            "Shape",
            "ShapeRange",
            "Size",
            "add_enumerated_image_sizes",
            "add_enumerated_multiarray_shapes",
            "add_multiarray_ndshape_enumeration",
            "set_multiarray_ndshape_range",
            "update_image_size_range",
            "update_multiarray_shape_range",
        ]
        _check_visible_modules(
            _get_visible_items(ct.models.neural_network.flexible_shape_utils), expected
        )

    def test_models_neural_network_update_optimizer_utils(self):
        expected = ["AdamParams", "Batch", "RangeParam", "SgdParams"]
        _check_visible_modules(
            _get_visible_items(ct.models.neural_network.update_optimizer_utils),
            expected,
        )

    def test_models_neural_network_optimization_utils(self):
        _check_visible_modules(
            _get_visible_items(ct.models.neural_network.optimization_utils), [],
        )
