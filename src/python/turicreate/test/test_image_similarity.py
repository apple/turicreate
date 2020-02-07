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
import turicreate as tc
from turicreate.toolkits._internal_utils import _mac_ver
import tempfile
from . import util as test_util
import coremltools
import numpy as np
from turicreate.toolkits._main import ToolkitError as _ToolkitError


def get_test_data():
    """
    Create 5 all white images and 5 all black images. Then add some noise to
    each image.
    """
    from PIL import Image

    DIM = 224

    # Five all white images
    data = []
    for _ in range(5):
        data.append(np.full((DIM, DIM, 3), 255, dtype=np.uint8))

    # Five all black images
    for _ in range(5):
        data.append(np.full((DIM, DIM, 3), 0, dtype=np.uint8))

    # Add some random noise to each image
    random = np.random.RandomState(100)
    for cur_image in data:
        for _ in range(1000):
            x, y = random.randint(DIM), random.randint(DIM)
            rand_pixel_value = (
                random.randint(255),
                random.randint(255),
                random.randint(255),
            )
            cur_image[x][y] = rand_pixel_value

    # Convert to an array of tc.Images
    images = []
    for cur_data in data:
        pil_image = Image.fromarray(cur_data)
        image_data = bytearray([z for l in pil_image.getdata() for z in l])
        image_data_size = len(image_data)
        tc_image = tc.Image(
            _image_data=image_data,
            _width=DIM,
            _height=DIM,
            _channels=3,
            _format_enum=2,
            _image_data_size=image_data_size,
        )
        images.append(tc_image)

    return tc.SFrame({"awesome_image": images})


data = get_test_data()


class ImageSimilarityTest(unittest.TestCase):
    @classmethod
    def setUpClass(self, input_image_shape=(3, 224, 224), model="resnet-50"):
        """
        The setup class method for the basic test case with all default values.
        """
        self.feature = "awesome_image"
        self.label = None
        self.input_image_shape = input_image_shape
        self.pre_trained_model = model

        # Create the model
        self.def_opts = {
            "model": "resnet-50",
            "verbose": True,
        }

        # Model
        self.model = tc.image_similarity.create(
            data, feature=self.feature, label=None, model=self.pre_trained_model
        )
        self.nn_model = self.model.feature_extractor
        self.lm_model = self.model.similarity_model
        self.opts = self.def_opts.copy()

        # Answers
        self.get_ans = {
            "similarity_model": lambda x: type(x)
            == tc.nearest_neighbors.NearestNeighborsModel,
            "feature": lambda x: x == self.feature,
            "training_time": lambda x: x > 0,
            "input_image_shape": lambda x: x == self.input_image_shape,
            "label": lambda x: x == self.label,
            "feature_extractor": lambda x: callable(x.extract_features),
            "num_features": lambda x: x == self.lm_model.num_features,
            "num_examples": lambda x: x == self.lm_model.num_examples,
            "model": lambda x: (
                x == self.pre_trained_model
                or (
                    self.pre_trained_model == "VisionFeaturePrint_Screen"
                    and x == "VisionFeaturePrint_Scene"
                )
            ),
        }
        self.fields_ans = self.get_ans.keys()

    def assertListAlmostEquals(self, list1, list2, tol):
        self.assertEqual(len(list1), len(list2))
        for a, b in zip(list1, list2):
            self.assertAlmostEqual(a, b, delta=tol)

    def test_create_with_missing_feature(self):
        with self.assertRaises(_ToolkitError):
            tc.image_similarity.create(data, feature="wrong_feature", label=self.label)

    def test_create_with_missing_label(self):
        with self.assertRaises(_ToolkitError):
            tc.image_similarity.create(data, feature=self.feature, label="wrong_label")

    def test_create_with_empty_dataset(self):
        with self.assertRaises(_ToolkitError):
            tc.image_similarity.create(data[:0])

    def test_query(self):
        model = self.model
        preds = model.query(data)
        self.assertEqual(len(preds), len(data) * 5)

        # Make sure all the white images (first five images) are only similar to the other white images
        white_sims = preds.filter_by([0, 1, 2, 3, 4], "query_label")["reference_label"]
        self.assertEqual(sorted(white_sims.unique()), [0, 1, 2, 3, 4])

        # Make sure all the black images (last five images) are only similar to the other black images
        white_sims = preds.filter_by([5, 6, 7, 8, 9], "query_label")["reference_label"]
        self.assertEqual(sorted(white_sims.unique()), [5, 6, 7, 8, 9])

    def test_similarity_graph(self):
        model = self.model
        preds = model.similarity_graph()
        self.assertEqual(len(preds.edges), len(data) * 5)

        preds = model.similarity_graph(output_type="SFrame")
        self.assertEqual(len(preds), len(data) * 5)

    def test_list_fields(self):
        model = self.model
        fields = model._list_fields()
        self.assertEqual(set(fields), set(self.fields_ans))

    def test_get(self):
        """
        Check the get function. Compare with the answer supplied as a lambda
        function for each field.
        """
        model = self.model
        for field in self.fields_ans:
            ans = model._get(field)
            self.assertTrue(
                self.get_ans[field](ans),
                "Get failed in field {}. Output was {}.".format(field, ans),
            )

    def test_query_input(self):
        model = self.model

        single_image = data[self.feature][0]
        sims = model.query(single_image)
        self.assertIsNotNone(sims)

        sarray = data[self.feature]
        sims = model.query(sarray)
        self.assertIsNotNone(sims)

        with self.assertRaises(TypeError):
            model.query("this is a junk value")

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

    def test_export_coreml(self):
        """
        Check the export_coreml() function.
        """

        def get_psnr(x, y):
            # See: https://en.wikipedia.org/wiki/Peak_signal-to-noise_ratio
            # The higher the number the better.
            return 20 * np.log10(max(x.max(), y.max())) - 10 * np.log10(
                np.square(x - y).mean()
            )

        # Save the model as a CoreML model file
        filename = tempfile.mkstemp("ImageSimilarity.mlmodel")[1]
        self.model.export_coreml(filename)

        # Load the model back from the CoreML model file
        coreml_model = coremltools.models.MLModel(filename)
        import platform

        self.assertDictEqual(
            {
                "com.github.apple.turicreate.version": tc.__version__,
                "com.github.apple.os.platform": platform.platform(),
                "type": "ImageSimilarityModel",
                "coremltoolsVersion": coremltools.__version__,
                "version": "1",
            },
            dict(coreml_model.user_defined_metadata),
        )
        expected_result = (
            "Image similarity (%s) created by Turi Create (version %s)"
            % (self.model.model, tc.__version__)
        )

        # Get model distances for comparison
        img = data[0:1][self.feature][0]
        img_fixed = tc.image_analysis.resize(img, *reversed(self.input_image_shape))
        tc_ret = self.model.query(img_fixed, k=data.num_rows())

        if _mac_ver() >= (10, 13):
            from PIL import Image as _PIL_Image

            pil_img = _PIL_Image.fromarray(img_fixed.pixel_data)
            coreml_ret = coreml_model.predict({"awesome_image": pil_img})

            # Compare distances
            coreml_distances = np.array(coreml_ret["distance"])
            tc_distances = tc_ret.sort("reference_label")["distance"].to_numpy()
            psnr_value = get_psnr(coreml_distances, tc_distances)
            self.assertTrue(psnr_value > 50)

    def test_save_and_load(self):
        with test_util.TempDirectory() as filename:

            self.model.save(filename)
            self.model = tc.load_model(filename)

            self.test_query()
            print("Query passed")
            self.test_similarity_graph()
            print("Similarity graph passed")
            self.test_get()
            print("Get passed")
            self.test_summary()
            print("Summary passed")
            self.test_list_fields()
            print("List fields passed")
            self.test_export_coreml()
            print("Export coreml passed")


class ImageSimilaritySqueezeNetTest(ImageSimilarityTest):
    @classmethod
    def setUpClass(self):
        super(ImageSimilaritySqueezeNetTest, self).setUpClass(
            model="squeezenet_v1.1", input_image_shape=(3, 227, 227)
        )


@unittest.skipIf(
    _mac_ver() < (10, 14), "VisionFeaturePrint_Scene only supported on macOS 10.14+"
)
class ImageSimilarityVisionFeaturePrintSceneTest(ImageSimilarityTest):
    @classmethod
    def setUpClass(self):
        super(ImageSimilarityVisionFeaturePrintSceneTest, self).setUpClass(
            model="VisionFeaturePrint_Scene", input_image_shape=(3, 299, 299)
        )

# A test to gaurantee that old code using the incorrect name still works.
@unittest.skipIf(
    _mac_ver() < (10, 14), "VisionFeaturePrint_Scene only supported on macOS 10.14+"
)
class ImageSimilarityVisionFeaturePrintSceneTest_bad_name(ImageSimilarityTest):
    @classmethod
    def setUpClass(self):
        super(ImageSimilarityVisionFeaturePrintSceneTest_bad_name, self).setUpClass(
            model="VisionFeaturePrint_Screen", input_image_shape=(3, 299, 299)
        )
