from __future__ import print_function as _
import unittest
import numpy as np
import coremltools.models.datatypes as datatypes
from coremltools.models import neural_network as neural_network
from coremltools.models import MLModel
from coremltools.models.utils import _is_macos, _macos_version
from coremltools._deps import _HAS_TF, MSG_TF1_NOT_FOUND

if _HAS_TF:
    import tensorflow as tf
import itertools

np.random.seed(10)
np.set_printoptions(precision=4, suppress=True)


@unittest.skipIf(not _HAS_TF, MSG_TF1_NOT_FOUND)
class CorrectnessTest(unittest.TestCase):
    def _compare_shapes(self, ref_preds, coreml_preds):
        if np.squeeze(ref_preds).shape != np.squeeze(coreml_preds).shape:
            return False
        else:
            return True

    def _compare_predictions_numerical(
        self, ref_preds, coreml_preds, snr_thresh=15, psnr_thresh=30
    ):
        ref_preds = ref_preds.flatten()
        coreml_preds = coreml_preds.flatten()
        noise = coreml_preds - ref_preds
        noise_var = np.mean(noise ** 2)
        signal_energy = np.mean(ref_preds ** 2)
        max_signal_energy = np.amax(ref_preds ** 2)

        if noise_var > 1e-6 and signal_energy > 1e-6:
            SNR = 10 * np.log10(signal_energy / noise_var)
            PSNR = 10 * np.log10(max_signal_energy / noise_var)

            # print('ref: ', ref_preds)
            # print('coreml: ', coreml_preds)
            # print('noise: ', noise)

            print("SNR: {}, PSNR: {}".format(SNR, PSNR))
            print("noise var: ", np.mean(noise ** 2))
            print("max signal energy: ", np.amax(ref_preds ** 2))
            print("signal energy: ", np.mean(ref_preds ** 2))

            self.assertGreaterEqual(PSNR, psnr_thresh)
            self.assertGreaterEqual(SNR, snr_thresh)

    def _test_model(
        self,
        input_dict,
        ref_output_dict,
        coreml_model,
        snr_thresh=15,
        psnr_thresh=30,
        cpu_only=False,
    ):
        coreml_out_dict = coreml_model.predict(input_dict, useCPUOnly=cpu_only)
        for out_ in list(ref_output_dict.keys()):
            ref_out = ref_output_dict[out_].flatten()
            coreml_out = coreml_out_dict[out_].flatten()
            self.assertEquals(len(coreml_out), len(ref_out))
            self._compare_predictions_numerical(
                ref_out, coreml_out, snr_thresh=snr_thresh, psnr_thresh=psnr_thresh
            )


@unittest.skipUnless(_is_macos(), "Only supported for MacOS platform.")
class StressTest(CorrectnessTest):
    def runTest(self):
        pass

    def test_data_reorganize(self, cpu_only=False):
        def get_coreml_model_reorganize(X, params):
            eval = True
            mlmodel = None
            try:
                input_dim = X.shape[2:]
                input_features = [("data", datatypes.Array(*input_dim))]
                output_features = [("output", None)]
                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features
                )
                builder.add_reorganize_data(
                    "reorg",
                    "data",
                    "output",
                    mode=params["mode"],
                    block_size=params["block_size"],
                )
                mlmodel = MLModel(builder.spec)
            except RuntimeError as e:
                print(e)
                eval = False

            return mlmodel, eval

        def get_tf_predictions_reorganize(X, params):
            Hin = params["H"]
            Win = params["W"]
            Cin = params["C"]
            with tf.Graph().as_default(), tf.Session() as sess:
                x = tf.placeholder(tf.float32, shape=(1, Hin, Win, Cin))
                if params["mode"] == "SPACE_TO_DEPTH":
                    y = tf.space_to_depth(x, params["block_size"])
                else:
                    y = tf.depth_to_space(x, params["block_size"])
                return sess.run(y, feed_dict={x: X})

        """
        Define Params
        """
        params_dict = dict(
            C=[1, 2, 8, 16, 15, 27],
            H=[2, 4, 6, 8, 10, 15, 21, 16],
            W=[2, 4, 6, 8, 10, 15, 21, 16],
            block_size=[2, 3, 4, 5],
            mode=["SPACE_TO_DEPTH", "DEPTH_TO_SPACE"],
        )
        params = [x for x in list(itertools.product(*params_dict.values()))]
        all_candidates = [dict(zip(params_dict.keys(), x)) for x in params]
        valid_params = []
        for pr in all_candidates:
            if pr["mode"] == "SPACE_TO_DEPTH":
                if pr["H"] % pr["block_size"] == 0 and pr["W"] % pr["block_size"] == 0:
                    valid_params.append(pr)
            else:
                if pr["C"] % (pr["block_size"] ** 2) == 0:
                    valid_params.append(pr)
        print(
            "Total params to be tested: ",
            len(valid_params),
            "out of canditates: ",
            len(all_candidates),
        )
        """
        Test
        """
        failed_tests_compile = []
        for i in range(len(valid_params)):
            params = valid_params[i]
            # print("=========: ", params)
            # if i % 10 == 0: print("======== Testing {}/{}".format(str(i), str(len(valid_params))))
            X = np.random.rand(1, params["C"], params["H"], params["W"])
            tf_preds = get_tf_predictions_reorganize(
                np.transpose(X, [0, 2, 3, 1]), params
            )
            tf_preds = np.transpose(tf_preds, [0, 3, 1, 2])
            coreml_model, eval = get_coreml_model_reorganize(
                np.expand_dims(X, axis=0), params
            )
            if eval is False:
                failed_tests_compile.append(params)
            else:
                input_dict = {"data": np.expand_dims(X, axis=0)}
                ref_output_dict = {"output": tf_preds[0, :, :, :]}
                self._test_model(
                    input_dict, ref_output_dict, coreml_model, cpu_only=cpu_only
                )

        self.assertEqual(failed_tests_compile, [])

    def test_data_reorganize_cpu_only(self):
        self.test_data_reorganize(cpu_only=True)

    def test_depthwise_conv(self, cpu_only=False):
        def get_coreml_model_depthwise(X, params, w):
            eval = True
            mlmodel = None
            try:
                input_dim = X.shape[2:]
                input_features = [("data", datatypes.Array(*input_dim))]
                output_features = [("output", None)]
                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features
                )
                # tranlate weights : (Kh, Kw, kernel_channels, output_channels) == (Kh, Kw, Cin/g, Cout) == (Kh, Kw, 1, channel_multiplier * Cin)
                w_e = np.reshape(
                    w,
                    (
                        params["kernel_size"],
                        params["kernel_size"],
                        params["multiplier"] * params["C"],
                        1,
                    ),
                )
                w_e = np.transpose(w_e, [0, 1, 3, 2])
                if params["padding"] == "SAME":
                    pad_mode = "same"
                else:
                    pad_mode = "valid"
                builder.add_convolution(
                    "conv",
                    kernel_channels=1,
                    output_channels=params["multiplier"] * params["C"],
                    height=params["kernel_size"],
                    width=params["kernel_size"],
                    stride_height=params["stride"],
                    stride_width=params["stride"],
                    border_mode=pad_mode,
                    groups=params["C"],
                    W=w_e,
                    b=None,
                    has_bias=0,
                    is_deconv=0,
                    output_shape=None,
                    input_name="data",
                    output_name="output",
                )

                mlmodel = MLModel(builder.spec)
            except RuntimeError as e:
                print(e)
                eval = False
            return mlmodel, eval

        def get_tf_predictions_depthwise(X, params, w):
            Hin = Win = params["H"]
            Cin = params["C"]
            Kh = Kw = params["kernel_size"]
            channel_multiplier = params["multiplier"]
            with tf.Graph().as_default(), tf.Session() as sess:
                x = tf.placeholder(tf.float32, shape=(1, Hin, Win, Cin))
                W = tf.constant(
                    w, dtype=tf.float32, shape=[Kh, Kw, Cin, channel_multiplier]
                )
                y = tf.nn.depthwise_conv2d(
                    x,
                    W,
                    strides=[1, params["stride"], params["stride"], 1],
                    padding=params["padding"],
                )
                return sess.run(y, feed_dict={x: X})

        """
        Define Params
        """
        params_dict = dict(
            C=[1, 4, 7],
            H=[11, 16],
            stride=[1, 2, 3],
            kernel_size=[1, 2, 3, 5],
            multiplier=[1, 2, 3, 4],
            padding=["SAME", "VALID"],
        )
        params = [x for x in list(itertools.product(*params_dict.values()))]
        all_candidates = [dict(zip(params_dict.keys(), x)) for x in params]
        valid_params = []
        for pr in all_candidates:
            if pr["padding"] == "VALID":
                if np.floor((pr["H"] - pr["kernel_size"]) / pr["stride"]) + 1 <= 0:
                    continue
            valid_params.append(pr)
        print(
            "Total params to be tested: ",
            len(valid_params),
            "out of canditates: ",
            len(all_candidates),
        )
        """
        Test
        """
        failed_tests_compile = []
        for i in range(len(valid_params)):
            params = valid_params[i]
            # print("=========: ", params)
            # if i % 10 == 0: print "======== Testing {}/{}".format(str(i), str(len(valid_params)))
            X = np.random.rand(1, params["C"], params["H"], params["H"])
            w = np.random.rand(
                params["kernel_size"],
                params["kernel_size"],
                params["C"],
                params["multiplier"],
            )
            tf_preds = get_tf_predictions_depthwise(
                np.transpose(X, [0, 2, 3, 1]), params, w
            )
            tf_preds = np.transpose(tf_preds, [0, 3, 1, 2])
            coreml_model, eval = get_coreml_model_depthwise(
                np.expand_dims(X, axis=0), params, w
            )
            if eval is False:
                failed_tests_compile.append(params)
            else:
                input_dict = {"data": np.expand_dims(X, axis=0)}
                ref_output_dict = {"output": tf_preds[0, :, :, :]}
                self._test_model(
                    input_dict, ref_output_dict, coreml_model, cpu_only=cpu_only
                )

        self.assertEqual(failed_tests_compile, [])

    def test_depthwise_conv_cpu_only(self, cpu_only=False):
        self.test_depthwise_conv(cpu_only=True)

    @unittest.skipUnless(_macos_version() >= (10, 14), "Only supported on MacOS 10.14+")
    def test_resize_bilinear(self, cpu_only=False):
        def get_coreml_model_resize_bilinear(X, params):
            eval = True
            mlmodel = None
            try:
                input_dim = X.shape[2:]
                input_features = [("data", datatypes.Array(*input_dim))]
                output_features = [("output", None)]
                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features
                )
                if params["align_corners"]:
                    mode = "STRICT_ALIGN_ENDPOINTS_MODE"
                else:
                    mode = "UPSAMPLE_MODE"
                builder.add_resize_bilinear(
                    "resize",
                    "data",
                    "output",
                    target_height=params["Hnew"],
                    target_width=params["Wnew"],
                    mode=mode,
                )
                mlmodel = MLModel(builder.spec)
            except RuntimeError as e:
                print(e)
                eval = False

            return mlmodel, eval

        def get_tf_predictions_resize_bilinear(X, params):
            with tf.Graph().as_default(), tf.Session() as sess:
                x = tf.placeholder(
                    tf.float32,
                    shape=(params["batch"], params["H"], params["W"], params["ch"]),
                )
                y = tf.image.resize_bilinear(
                    x,
                    size=[params["Hnew"], params["Wnew"]],
                    align_corners=params["align_corners"],
                )
                return sess.run(y, feed_dict={x: X})

        """
        Define Params
        """
        params_dict = dict(
            H=[1, 3, 10],  # [1,2,3,10]
            W=[1, 3, 10],  # [1,2,3,10]
            Hnew=[1, 2, 6],  # [1,3,6,12,20]
            Wnew=[1, 2, 6],  # [1,3,6,12,20]
            align_corners=[False, True],  # [False, True]
            ch=[1, 5],  # [1,5]
            batch=[1, 3],  # [1, 3]
        )
        params = [x for x in list(itertools.product(*params_dict.values()))]
        valid_params = [dict(zip(params_dict.keys(), x)) for x in params]
        print("Total params to be tested: {}".format(len(valid_params)))
        """
        Test
        """
        failed_tests_compile = []
        for i in range(len(valid_params)):
            params = valid_params[i]
            # #print("=========: ", params)
            if i % 100 == 0:
                print(
                    "======================= Testing {}/{}".format(
                        str(i), str(len(valid_params))
                    )
                )
            X = np.round(
                255
                * np.random.rand(
                    params["batch"], params["ch"], params["H"], params["W"]
                )
            )
            tf_preds = get_tf_predictions_resize_bilinear(
                np.transpose(X, [0, 2, 3, 1]), params
            )
            tf_preds = np.transpose(tf_preds, [0, 3, 1, 2])
            coreml_model, eval = get_coreml_model_resize_bilinear(
                np.expand_dims(X, axis=0), params
            )
            if eval is False:
                failed_tests_compile.append(params)
            else:
                input_dict = {"data": np.expand_dims(X, axis=0)}
                ref_output_dict = {"output": np.expand_dims(tf_preds, axis=0)}
                self._test_model(
                    input_dict, ref_output_dict, coreml_model, cpu_only=cpu_only
                )

        self.assertEqual(failed_tests_compile, [])

    @unittest.skipUnless(_macos_version() >= (10, 14), "Only supported on MacOS 10.14+")
    def test_resize_bilinear_cpu_only(self):
        self.test_resize_bilinear(cpu_only=True)

    @unittest.skipUnless(_macos_version() >= (10, 14), "Only supported on MacOS 10.14+")
    def test_crop_resize(self, cpu_only=False):
        def get_coreml_model_crop_resize(params):
            eval = True
            mlmodel = None
            batch, ch, n_roi = params["b_c_n"]
            H = params["H"]
            W = params["W"]
            try:
                input_features = [("data", datatypes.Array(ch, H, W))]
                input_features.append(("roi", datatypes.Array(4, 1, 1)))
                if batch != 1:
                    input_features.append(("box_ind", datatypes.Array(1, 1, 1)))
                output_features = [("output", None)]
                builder = neural_network.NeuralNetworkBuilder(
                    input_features, output_features
                )

                if batch != 1:
                    builder.add_elementwise(
                        "concat", ["box_ind", "roi"], "roi_out", "CONCAT"
                    )
                    input_names = ["data", "roi_out"]
                else:
                    input_names = ["data", "roi"]

                builder.add_crop_resize(
                    "resize",
                    input_names,
                    "output",
                    target_height=params["Hnew"],
                    target_width=params["Wnew"],
                    mode="ALIGN_ENDPOINTS_MODE",
                    normalized_roi=True,
                    box_indices_mode="CORNERS_HEIGHT_FIRST",
                    spatial_scale=1.0,
                )
                mlmodel = MLModel(builder.spec)
            except RuntimeError as e:
                print(e)
                eval = False

            return mlmodel, eval

        def get_tf_predictions_crop_resize(X, boxes, box_ind, params):
            batch, ch, n_roi = params["b_c_n"]
            with tf.Graph().as_default(), tf.Session() as sess:
                x = tf.placeholder(
                    tf.float32, shape=(batch, params["H"], params["W"], ch)
                )
                y = tf.image.crop_and_resize(
                    x, boxes, box_ind, crop_size=[params["Hnew"], params["Wnew"]]
                )
                return sess.run(y, feed_dict={x: X})

        """
        Define Params
        """
        params_dict = dict(
            H=[1, 3, 10],  # [1,2,3,6,10]
            W=[1, 3, 10],  # [1,2,3,6,10]
            Hnew=[1, 2, 3, 6],  # [1,2,3,6,12,20]
            Wnew=[1, 2, 3, 6],  # [1,2,3,6,12,20]
            b_c_n=[
                (1, 1, 1),
                (1, 2, 3),
                (3, 2, 1),
                (3, 4, 3),
            ],  # [(1,1,1),(1,2,3),(3,2,1),(3,4,3)]
        )
        params = [x for x in list(itertools.product(*params_dict.values()))]
        valid_params = [dict(zip(params_dict.keys(), x)) for x in params]
        print("Total params to be tested: {}".format(len(valid_params)))
        """
        Test
        """
        failed_tests_compile = []
        for i in range(len(valid_params)):
            params = valid_params[i]
            # print("=========: ", params)
            # if i % 100 == 0:
            #     print("======================= Testing {}/{}".format(str(i), str(len(valid_params))))
            batch, ch, n_roi = params["b_c_n"]
            X = np.round(255 * np.random.rand(batch, ch, params["H"], params["W"]))
            roi = np.zeros((n_roi, 4), dtype=np.float32)
            box_ind = np.zeros((n_roi))
            if batch != 1:
                box_ind = np.random.randint(low=0, high=batch, size=(n_roi))
            for ii in range(n_roi):
                r = np.random.rand(4)
                w_start = r[0]
                h_start = r[1]
                w_end = r[2] * (1 - w_start) + w_start
                h_end = r[3] * (1 - h_start) + h_start
                roi[ii, :] = [h_start, w_start, h_end, w_end]
                roi[ii, :] = np.round(100 * roi[ii, :]) / 100
                assert roi[ii, 0] <= roi[ii, 2]
                assert roi[ii, 1] <= roi[ii, 3]

            tf_preds = get_tf_predictions_crop_resize(
                np.transpose(X, [0, 2, 3, 1]), roi, box_ind, params
            )
            tf_preds = np.transpose(tf_preds, [0, 3, 1, 2])
            coreml_model, eval = get_coreml_model_crop_resize(params)
            if eval is False:
                failed_tests_compile.append(params)
            else:
                input_dict = {"data": np.expand_dims(X, axis=0)}
                input_dict["roi"] = np.reshape(roi, (n_roi, 1, 4, 1, 1))
                if batch != 1:
                    input_dict["box_ind"] = np.reshape(
                        box_ind.astype(np.float32), (n_roi, 1, 1, 1, 1)
                    )
                ref_output_dict = {"output": np.expand_dims(tf_preds, axis=0)}
                self._test_model(
                    input_dict, ref_output_dict, coreml_model, cpu_only=cpu_only
                )

        self.assertEqual(failed_tests_compile, [])

    @unittest.skipUnless(_macos_version() >= (10, 14), "Only supported on MacOS 10.14+")
    def test_crop_resize_cpu_only(self):
        self.test_crop_resize(cpu_only=True)
