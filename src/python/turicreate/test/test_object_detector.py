# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import unittest
import pytest
from . import util as test_util
import turicreate as tc
import tempfile
import numpy as np
import platform
import pytest
import sys
import os
from turicreate.toolkits._main import ToolkitError as _ToolkitError
from turicreate.toolkits._internal_utils import (
    _raise_error_if_not_sarray,
    _mac_ver,
    _read_env_var_cpp,
)
from six import StringIO
import coremltools

_CLASSES = ["person", "cat", "dog", "chair"]


def _get_data(feature, annotations):
    from PIL import Image as _PIL_Image

    rs = np.random.RandomState(1234)

    def from_pil_image(pil_img, image_format="png"):
        # The above didn't work, so as a temporary fix write to temp files
        if image_format == "raw":
            image = np.array(pil_img)
            FORMAT_RAW = 2
            return tc.Image(
                _image_data=image.tobytes(),
                _width=image.shape[1],
                _height=image.shape[0],
                _channels=image.shape[2],
                _format_enum=FORMAT_RAW,
                _image_data_size=image.size,
            )
        else:
            with tempfile.NamedTemporaryFile(
                mode="w+b", suffix="." + image_format
            ) as f:
                pil_img.save(f, format=image_format)
                return tc.Image(f.name)

    num_examples = 100
    max_num_boxes_per_image = 10
    classes = _CLASSES
    images = []
    anns = []
    FORMATS = ["png", "jpeg", "raw"]
    for i in range(num_examples):
        # Randomly determine image size (should handle large and small)
        img_shape = tuple(rs.randint(100, 1000, size=2)) + (3,)
        img = rs.randint(255, size=img_shape)

        pil_img = _PIL_Image.fromarray(img, mode="RGB")
        # Randomly select image format
        image_format = FORMATS[rs.randint(len(FORMATS))]
        images.append(from_pil_image(pil_img, image_format=image_format))

        ann = []
        for j in range(rs.randint(max_num_boxes_per_image)):
            left, right = np.sort(rs.randint(0, img_shape[1], size=2))
            top, bottom = np.sort(rs.randint(0, img_shape[0], size=2))

            x = (left + right) / 2
            y = (top + bottom) / 2

            width = max(right - left, 1)
            height = max(bottom - top, 1)

            label = {
                "coordinates": {"x": x, "y": y, "width": width, "height": height,},
                "label": classes[rs.randint(len(classes))],
                "type": "rectangle",
            }
            ann.append(label)
        anns.append(ann)

    data = tc.SFrame({feature: tc.SArray(images), annotations: tc.SArray(anns),})
    return data


class ObjectDetectorTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        """
        The setup class method for the basic test case with all default values.
        """
        self.feature = "myimage"
        self.annotations = "myannotations"
        self.pre_trained_model = "darknet-yolo"
        ## Create the model
        self.def_opts = {
            "model": "darknet-yolo",
            "max_iterations": 0,
        }

        # Model
        self.sf = _get_data(feature=self.feature, annotations=self.annotations)
        self.model = tc.object_detector.create(
            self.sf,
            feature=self.feature,
            annotations=self.annotations,
            batch_size=2,
            max_iterations=1,
            model=self.pre_trained_model,
        )

        ## Answers
        self.opts = self.def_opts.copy()
        self.opts["max_iterations"] = 1

        self.get_ans = {
            "_model": lambda x: True,
            "_class_to_index": lambda x: isinstance(x, dict),
            "_training_time_as_string": lambda x: isinstance(x, str),
            "_grid_shape": lambda x: tuple(x) == (13, 13),
            "model": lambda x: x == self.pre_trained_model,
            "anchors": lambda x: (
                isinstance(x, (list, tuple, np.ndarray))
                and len(x) > 0
                and len(x[0]) == 2
            ),
            "input_image_shape": lambda x: tuple(x) == (3, 416, 416),
            "batch_size": lambda x: x == 2,
            "classes": lambda x: x == sorted(_CLASSES),
            "feature": lambda x: x == self.feature,
            "max_iterations": lambda x: x >= 0,
            "non_maximum_suppression_threshold": lambda x: 0 <= x <= 1,
            "training_time": lambda x: x > 0,
            "training_iterations": lambda x: x > 0,
            "training_epochs": lambda x: x >= 0,
            "num_bounding_boxes": lambda x: x > 0,
            "num_examples": lambda x: x > 0,
            "training_loss": lambda x: x > 0,
            "annotations": lambda x: x == self.annotations,
            "num_classes": lambda x: x == len(_CLASSES),
        }

        self.get_ans["annotation_position"] = lambda x: isinstance(x, str)
        self.get_ans["annotation_scale"] = lambda x: isinstance(x, str)
        self.get_ans["annotation_origin"] = lambda x: isinstance(x, str)
        self.get_ans["grid_height"] = lambda x: x > 0
        self.get_ans["grid_width"] = lambda x: x > 0
        self.get_ans["random_seed"] = lambda x: True
        self.get_ans["verbose"] = lambda x: True
        del self.get_ans["_model"]
        del self.get_ans["_class_to_index"]
        del self.get_ans["_grid_shape"]
        del self.get_ans["anchors"]
        del self.get_ans["non_maximum_suppression_threshold"]

        self.fields_ans = self.get_ans.keys()

    def test_create_with_missing_value(self):
        sf = self.sf.append(
            tc.SFrame(
                {
                    self.feature: tc.SArray([None], dtype=tc.Image),
                    self.annotations: [self.sf[self.annotations][0]],
                }
            )
        )
        with self.assertRaises(_ToolkitError):
            tc.object_detector.create(
                sf, feature=self.feature, annotations=self.annotations
            )

    def test_create_with_missing_feature(self):
        with self.assertRaises(_ToolkitError):
            tc.object_detector.create(
                self.sf, feature="wrong_feature", annotations=self.annotations
            )

    def test_create_with_missing_annotations(self):
        with self.assertRaises(_ToolkitError):
            tc.object_detector.create(
                self.sf, feature=self.feature, annotations="wrong_annotations"
            )

    def test_create_with_invalid_annotations_list_coord(self):
        with self.assertRaises(_ToolkitError):
            sf = self.sf.head()
            sf[self.annotations] = sf[self.annotations].apply(
                lambda x: [{"label": _CLASSES[0], "coordinates": [100, 50, 20, 40]}]
            )

            tc.object_detector.create(sf)

    def test_create_with_invalid_annotations_coordinate(self):
        with self.assertRaises(_ToolkitError):
            sf = self.sf.head()
            sf[self.annotations] = sf[self.annotations].apply(
                lambda x: [
                    {
                        "label": _CLASSES[0],
                        "coordinates": {"x": None, "y": 1, "width": 1, "height": 1},
                    }
                ]
            )
            tc.object_detector.create(sf)

        with self.assertRaises(_ToolkitError):
            sf = self.sf.head()
            sf[self.annotations] = sf[self.annotations].apply(
                lambda x: [
                    {
                        "label": _CLASSES[0],
                        "coordinates": {"x": 1, "y": [], "width": 1, "height": 1},
                    }
                ]
            )
            tc.object_detector.create(sf)

        with self.assertRaises(_ToolkitError):
            sf = self.sf.head()
            sf[self.annotations] = sf[self.annotations].apply(
                lambda x: [
                    {
                        "label": _CLASSES[0],
                        "coordinates": {"x": 1, "y": 1, "width": {}, "height": 1},
                    }
                ]
            )
            tc.object_detector.create(sf)

        with self.assertRaises(_ToolkitError):
            sf = self.sf.head()
            sf[self.annotations] = sf[self.annotations].apply(
                lambda x: [
                    {
                        "label": _CLASSES[0],
                        "coordinates": {"x": 1, "y": 1, "width": 1, "height": "1"},
                    }
                ]
            )
            tc.object_detector.create(sf)

    def test_create_with_missing_annotations_label(self):
        def create_missing_annotations_label(x):
            for y in x:
                y["label"] = None
            return x

        with self.assertRaises(_ToolkitError):
            sf = self.sf.head()
            sf[self.annotations] = sf[self.annotations].apply(
                lambda x: create_missing_annotations_label(x)
            )
            tc.object_detector.create(sf)

    def test_create_with_invalid_annotations_not_dict(self):
        with self.assertRaises(_ToolkitError):
            sf = self.sf.head()
            sf[self.annotations] = sf[self.annotations].apply(lambda x: [1])

            tc.object_detector.create(sf)

    def test_create_with_invalid_user_define_classes(self):
        sf = self.sf.head()
        old_stdout = sys.stdout
        result_out = StringIO()
        sys.stdout = result_out
        model = tc.object_detector.create(
            sf,
            feature=self.feature,
            annotations=self.annotations,
            classes=["invalid"],
            max_iterations=1,
        )
        sys.stdout = old_stdout
        self.assertTrue("Warning" in result_out.getvalue())

    def test_create_with_empty_dataset(self):
        with self.assertRaises(_ToolkitError):
            tc.object_detector.create(self.sf[:0])

    def test_create_with_verbose_False(self):
        args = [self.sf, self.annotations, self.feature]
        kwargs = {"max_iterations": 1, "model": self.pre_trained_model}
        test_util.assert_longer_verbose_logs(tc.object_detector.create, args, kwargs)

    def test_dict_annotations(self):
        sf_copy = self.sf[:]
        sf_copy[self.annotations] = sf_copy[self.annotations].apply(
            lambda x: x[0] if len(x) > 0 else None
        )
        dict_model = tc.object_detector.create(
            sf_copy,
            feature=self.feature,
            annotations=self.annotations,
            max_iterations=1,
            model=self.pre_trained_model,
        )

        pred = dict_model.predict(sf_copy)
        metrics = dict_model.evaluate(sf_copy)
        annotated_img = tc.object_detector.util.draw_bounding_boxes(
            sf_copy[self.feature], sf_copy[self.annotations]
        )

    def test_extra_classes(self):
        # Create while the data has extra classes
        model = tc.object_detector.create(
            self.sf, classes=_CLASSES[:2], max_iterations=1
        )
        self.assertEqual(len(model.classes), 2)

        # Evaluate while the data has extra classes
        ret = model.evaluate(self.sf.head())
        self.assertEqual(len(ret["average_precision_50"]), 2)

    def test_different_grip_shape(self):
        # Should able to give different input grip shape
        shapes = [[1, 1], [5, 5], [13, 13], [26, 26]]
        for shape in shapes:
            model = tc.object_detector.create(
                self.sf, max_iterations=1, grid_shape=shape
            )
            pred = model.predict(self.sf)

    def test_predict(self):
        sf = self.sf.head()
        # Make sure this does not need the annotations column to work
        del sf[self.annotations]

        pred = self.model.predict(sf.head())

        # Check the structure of the output
        _raise_error_if_not_sarray(pred)
        self.assertEqual(len(pred), len(sf))

        # Make sure SFrame was not altered
        self.assertEqual([col for col in sf.column_names() if col.startswith("_")], [])

        # Predict should work on no input (and produce no predictions)
        pred0 = self.model.predict(sf[:0])
        self.assertEqual(len(pred0), 0)

    def test_predict_with_invalid_annotation(self):
        # predict function shouldn't throw exception when annotations column is invalid
        sf = self.sf.head()
        sf[self.annotations] = sf[self.annotations].apply(lambda x: "invalid")
        pred = self.model.predict(sf)

    def test_single_image(self):
        # Predict should work on a single image and product a list of dictionaries
        # (we set confidene threshold to 0 to ensure predictions are returned)
        pred = self.model.predict(self.sf[self.feature][0], confidence_threshold=0)
        self.assertTrue(isinstance(pred, list))
        self.assertTrue(isinstance(pred[0], dict))

    def test_sarray(self):
        sarray = self.sf.head()[self.feature]
        pred = self.model.predict(sarray, confidence_threshold=0)
        self.assertEqual(len(pred), len(sarray))

    def test_confidence_threshold(self):
        sf = self.sf.head()
        pred = self.model.predict(sf.head(), confidence_threshold=1.0)
        stacked = tc.object_detector.util.stack_annotations(pred)
        # This should return no predictions, especially with an unconverged model
        self.assertEqual(len(stacked), 0)

        pred = self.model.predict(sf.head(), confidence_threshold=0.0)
        stacked = tc.object_detector.util.stack_annotations(pred)
        # This should return predictions
        self.assertTrue(len(stacked) > 0)

    def test_evaluate(self):
        ret = self.model.evaluate(self.sf.head(), metric="average_precision")

        self.assertTrue(set(ret), {"average_precision"})
        self.assertEqual(set(ret["average_precision"].keys()), set(_CLASSES))

        ret = self.model.evaluate(self.sf.head())
        self.assertEqual(
            set(ret), {"mean_average_precision_50", "average_precision_50"}
        )
        self.assertTrue(isinstance(ret["mean_average_precision_50"], float))
        self.assertEqual(set(ret["average_precision_50"].keys()), set(_CLASSES))

        # Empty dataset should not fail with error (although it should to 0
        # metrics)
        ret = self.model.evaluate(self.sf[:0])
        self.assertEqual(ret["mean_average_precision_50"], 0.0)

    def test_predict_invalid_threshold(self):
        with self.assertRaises(_ToolkitError):
            self.model.predict(self.sf.head(), confidence_threshold=-1)
        with self.assertRaises(_ToolkitError):
            self.model.predict(self.sf.head(), iou_threshold=-1)

    def test_evaluate_invalid_threshold(self):
        with self.assertRaises(_ToolkitError):
            self.model.evaluate(self.sf.head(), confidence_threshold=-1)
        with self.assertRaises(_ToolkitError):
            self.model.evaluate(self.sf.head(), iou_threshold=-1)

    def test_evaluate_sframe_format(self):
        metrics = ["mean_average_precision_50", "mean_average_precision"]
        for metric in metrics:
            pred = self.model.evaluate(
                self.sf.head(), metric=metric, output_type="sframe"
            )
            self.assertEqual(pred.column_names(), ["label"])

    def test_evaluate_invalid_metric(self):
        with self.assertRaises(_ToolkitError):
            self.model.evaluate(self.sf.head(), metric="not-supported-metric")

    def test_evaluate_invalid_format(self):
        with self.assertRaises(_ToolkitError):
            self.model.evaluate(self.sf.head(), output_type="not-supported-format")

    def test_evaluate_missing_annotations(self):
        with self.assertRaises(_ToolkitError):
            sf = self.sf.copy()
            del sf[self.annotations]
            self.model.evaluate(sf.head())

    def test_evaluate_with_missing_annotations_label(self):
        def create_missing_annotations_label(x):
            for y in x:
                y["label"] = None
            return x

        with self.assertRaises(_ToolkitError):
            sf = self.sf.head()
            sf[self.annotations] = sf[self.annotations].apply(
                lambda x: create_missing_annotations_label(x)
            )
            self.model.evaluate(sf)

    def test_export_coreml(self):
        from PIL import Image
        import coremltools
        import platform

        filename = tempfile.mkstemp("bingo.mlmodel")[1]
        self.model.export_coreml(filename, include_non_maximum_suppression=False)

        coreml_model = coremltools.models.MLModel(filename)
        self.assertDictEqual(
            {
                "com.github.apple.turicreate.version": tc.__version__,
                "com.github.apple.os.platform": platform.platform(),
                "annotations": self.annotations,
                "type": "object_detector",
                "classes": ",".join(sorted(_CLASSES)),
                "feature": self.feature,
                "include_non_maximum_suppression": "False",
                "max_iterations": "1",
                "model": "darknet-yolo",
                "training_iterations": "1",
                "version": "1",
            },
            dict(coreml_model.user_defined_metadata),
        )
        expected_result = "Object detector created by Turi Create (version %s)" % (
            tc.__version__
        )
        self.assertEquals(expected_result, coreml_model.short_description)

        img = self.sf[0:1][self.feature][0]
        img_fixed = tc.image_analysis.resize(img, 416, 416, 3)
        pil_img = Image.fromarray(img_fixed.pixel_data)
        if _mac_ver() >= (10, 13):
            ret = coreml_model.predict({self.feature: pil_img}, usesCPUOnly=True)
            self.assertEqual(ret["coordinates"].shape[1], 4)
            self.assertEqual(ret["confidence"].shape[1], len(_CLASSES))
            self.assertEqual(ret["coordinates"].shape[0], ret["confidence"].shape[0])
            # A numeric comparison of the resulting of top bounding boxes is
            # not that meaningful unless the model has converged

        # Also check if we can train a second model and export it.
        filename2 = tempfile.mkstemp("bingo2.mlmodel")[1]
        # We also test at the same time if we can export a model with a single
        # class
        sf = tc.SFrame(
            {
                "image": [self.sf[self.feature][0]],
                "ann": [self.sf[self.annotations][0][:1]],
            }
        )
        model2 = tc.object_detector.create(sf, max_iterations=1)
        model2.export_coreml(filename2, include_non_maximum_suppression=False)

    @unittest.skipIf(
        _mac_ver() < (10, 14),
        "Non-maximum suppression is only supported on MacOS 10.14+.",
    )
    def test_export_coreml_with_non_maximum_suppression(self):
        from PIL import Image

        filename = tempfile.mkstemp("bingo.mlmodel")[1]
        self.model.export_coreml(filename, include_non_maximum_suppression=True)

        coreml_model = coremltools.models.MLModel(filename)
        img = self.sf[0:1][self.feature][0]
        img_fixed = tc.image_analysis.resize(img, 416, 416, 3)
        pil_img = Image.fromarray(img_fixed.pixel_data)
        if _mac_ver() >= (10, 13):
            ret = coreml_model.predict({self.feature: pil_img}, usesCPUOnly=True)
            self.assertEqual(ret["coordinates"].shape[1], 4)
            self.assertEqual(ret["confidence"].shape[1], len(_CLASSES))
            self.assertEqual(ret["coordinates"].shape[0], ret["confidence"].shape[0])
            # A numeric comparison of the resulting of top bounding boxes is
            # not that meaningful unless the model has converged

        # Also check if we can train a second model and export it.
        filename2 = tempfile.mkstemp("bingo2.mlmodel")[1]
        # We also test at the same time if we can export a model with a single
        # class
        sf = tc.SFrame(
            {
                "image": [self.sf[self.feature][0]],
                "ann": [self.sf[self.annotations][0][:1]],
            }
        )
        model2 = tc.object_detector.create(sf, max_iterations=1)
        model2.export_coreml(filename2, include_non_maximum_suppression=True)

    @pytest.mark.xfail
    @unittest.skipIf(
        sys.platform != "darwin" or _mac_ver() >= (10, 14),
        "GPU selection should fail on macOS 10.13 or below",
    )
    def test_no_gpu_support_on_unsupported_macos(self):
        num_gpus = tc.config.get_num_gpus()
        tc.config.set_num_gpus(1)
        with self.assertRaises(_ToolkitError):
            tc.object_detector.create(self.sf, max_iterations=1)
        tc.config.set_num_gpus(num_gpus)

    def test__list_fields(self):
        model = self.model
        fields = model._list_fields()
        self.assertEqual(set(fields), set(self.fields_ans))

    def test_get(self):
        model = self.model
        for field in self.fields_ans:
            ans = model._get(field)
            self.assertTrue(
                self.get_ans[field](ans),
                """Get failed in field {}. Output was {}.""".format(field, ans),
            )

    def test_summary(self):
        model = self.model
        model.summary()

    def test_summary_str(self):
        model = self.model
        self.assertTrue(isinstance(model.summary("str"), str))

    def test_summary_dict(self):
        model = self.model
        self.assertTrue(isinstance(model.summary("dict"), dict))

    def test_summary_invalid_input(self):
        model = self.model
        with self.assertRaises(_ToolkitError):
            model.summary(model.summary("invalid"))

        with self.assertRaises(_ToolkitError):
            model.summary(model.summary(0))

        with self.assertRaises(_ToolkitError):
            model.summary(model.summary({}))

    def test_repr(self):
        # Repr after fit
        model = self.model
        self.assertEqual(type(str(model)), str)
        self.assertEqual(type(model.__repr__()), str)

    def test_save_and_load(self):
        with test_util.TempDirectory() as filename:
            self.model.save(filename)
            self.model = tc.load_model(filename)

            self.test_predict()
            print("Predict passed")
            self.test_get()
            print("Get passed")
            self.test_summary()
            print("Summary passed")
            self.test__list_fields()
            print("List fields passed")


@unittest.skipIf(tc.util._num_available_gpus() == 0, "Requires GPU")
@pytest.mark.gpu
class ObjectDetectorGPUTest(unittest.TestCase):
    @classmethod
    def setUpClass(self):
        self.feature = "myimage"
        self.annotations = "myannotations"
        self.sf = _get_data(feature=self.feature, annotations=self.annotations)

    def test_gpu_save_load_export(self):
        old_num_gpus = tc.config.get_num_gpus()
        gpu_options = set([old_num_gpus, 0, 1])
        for in_gpus in gpu_options:
            tc.config.set_num_gpus(in_gpus)
            model = tc.object_detector.create(self.sf, max_iterations=1)
            for out_gpus in gpu_options:
                with test_util.TempDirectory() as path:
                    model.save(path)
                    tc.config.set_num_gpus(out_gpus)
                    model = tc.load_model(path)
                    model.export_coreml(os.path.join(path, "model.mlmodel"))

        tc.config.set_num_gpus(old_num_gpus)
