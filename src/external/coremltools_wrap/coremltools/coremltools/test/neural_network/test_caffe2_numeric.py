from __future__ import division
from __future__ import print_function
from coremltools._deps import HAS_CAFFE2
import unittest
import numpy as np
import coremltools.models.datatypes as datatypes
from coremltools.models import neural_network as neural_network
from coremltools.models import MLModel
from coremltools.models.utils import macos_version
import itertools
import copy

if HAS_CAFFE2:
    from caffe2.python import workspace, model_helper

np.random.seed(10)
np.set_printoptions(precision=3, suppress=True)


class CorrectnessTest(unittest.TestCase):
    def _compare_shapes(self, ref_preds, coreml_preds):
        if np.squeeze(ref_preds).shape != np.squeeze(coreml_preds).shape:
            return False
        else:
            return True

    def _compare_predictions(self, ref_preds, coreml_preds, snr_thresh=35):
        ref_preds = ref_preds.flatten()
        coreml_preds = coreml_preds.flatten()
        noise = coreml_preds - ref_preds
        noise_var = np.sum(noise ** 2) / len(noise) + 1e-7
        signal_energy = np.sum(ref_preds ** 2) / len(ref_preds)
        max_signal_energy = np.amax(ref_preds ** 2)
        SNR = 10 * np.log10(signal_energy / noise_var)
        if SNR < snr_thresh:
            return False
        else:
            return True

    def _test_model(self, input_dict, ref_output_dict, coreml_model):
        if macos_version() >= (10, 13):
            coreml_out_dict = coreml_model.predict(input_dict, useCPUOnly=True)
            for out_ in list(ref_output_dict.keys()):
                ref_out = ref_output_dict[out_]
                coreml_out = coreml_out_dict[out_]
                if self._compare_shapes(ref_out, coreml_out):
                    return True, self._compare_predictions(ref_out, coreml_out)
                else:
                    return False, False
        return True, True


@unittest.skipIf(not HAS_CAFFE2, 'Missing Caffe2. Skipping tests.')
class StressTest(CorrectnessTest):

    def test_roi_align(self):

        def get_coreml_model_roi_align(params):
            eval = True
            mlmodel = None
            batch, ch, n_roi = params["b_c_n"]
            H = params["H"]
            W = params["W"]
            s_ratio = params["sampling_ratio"]
            try:
                input_features = [('data', datatypes.Array(ch,H,W))]
                if batch == 1:
                    input_features.append(('roi', datatypes.Array(4, 1, 1)))
                else:
                    input_features.append(('roi', datatypes.Array(5, 1, 1)))
                output_features = [('output', None)]
                builder = neural_network.NeuralNetworkBuilder(input_features, output_features)

                builder.add_crop_resize('resize', ['data', 'roi'], 'output_crop_resize',
                                        target_height=params["Hnew"]*s_ratio, target_width=params["Wnew"]*s_ratio,
                                        mode='ROI_ALIGN_MODE',
                                        normalized_roi=False,
                                        box_indices_mode='CORNERS_WIDTH_FIRST',
                                        spatial_scale=params["spatial_scale"])
                builder.add_pooling('pool', height=s_ratio, width=s_ratio,
                                    stride_height=s_ratio, stride_width=s_ratio,
                                    layer_type='AVERAGE',
                                    padding_type='VALID',
                                    input_name='output_crop_resize', output_name='output')
                mlmodel = MLModel(builder.spec)
            except RuntimeError as e:
                print(e)
                eval = False

            return mlmodel, eval

        def get_caffe2_predictions_roi_align(X, roi, params):
            workspace.ResetWorkspace()
            workspace.FeedBlob("data", X.astype(np.float32))
            workspace.FeedBlob("roi_in", roi.astype(np.float32))
            m = model_helper.ModelHelper(name="my net")
            roi_align = m.net.RoIAlign(["data", "roi_in"], "out",
                                       spatial_scale=params["spatial_scale"],
                                       pooled_h=params["Hnew"],
                                       pooled_w=params["Wnew"],
                                       sampling_ratio=params["sampling_ratio"])
            workspace.RunNetOnce(m.param_init_net)
            workspace.CreateNet(m.net)
            workspace.RunNetOnce(m.net)
            out = workspace.FetchBlob("out")
            return out


        '''
        Define Params
        '''
        params_dict = dict(H = [1,4,10,100],  #[1,4,10,100]
                            W = [1,4,10,100],  #[1,4,10,100]
                            Hnew = [1,2,6,7],  #[1,2,3,6,7]
                            Wnew = [1,2,6,7],  #[1,2,3,6,7]
                            b_c_n=[(1, 1, 1), (1, 2, 3), (3, 2, 1), (3, 4, 3)],  # [(1,1,1),(1,2,3),(3,2,1),(3,4,3)]
                            sampling_ratio = [1,2,3],  #[1,2,3]
                            spatial_scale = [1.0,0.5],  #[1.0, 0.5]
                           )
        params = [x for x in list(itertools.product(*params_dict.values()))]
        valid_params = [dict(zip(params_dict.keys(), x)) for x in params]
        print("Total params to be tested: {}".format(len(valid_params)))
        '''
        Test
        '''
        failed_tests_compile = []
        failed_tests_shape = []
        failed_tests_numerical = []
        for i in range(len(valid_params)):
            params = valid_params[i]
            # print("=========: ", params)
            # if i % 100 == 0:
            #     print("======================= Testing {}/{}".format(str(i), str(len(valid_params))))
            batch, ch, n_roi = params["b_c_n"]
            X = np.round(255 * np.random.rand(batch, ch, params["H"], params["W"]))
            H = params["H"]
            W = params["W"]
            spatial_scale = params["spatial_scale"]

            if batch == 1:
                roi = np.zeros((n_roi, 4), dtype=np.float32)
            else:
                roi = np.zeros((n_roi, 5), dtype=np.float32)
                roi[:, 0] = np.random.randint(low=0, high=batch, size=(1, n_roi))

            for ii in range(n_roi):
                r = np.random.rand(4)

                w_start = r[0] * (W - 1)
                h_start = r[1] * (H - 1)
                w_end = r[2] * (W - 1 - w_start) + w_start
                h_end = r[3] * (H - 1 - h_start) + h_start

                if batch == 1:
                    roi[ii, :] = [w_start, h_start, w_end, h_end]
                    roi[ii, :] /= spatial_scale
                    roi[ii, :] = np.round(10 * roi[ii, :]) / 10
                    assert roi[ii, 0] <= roi[ii, 2]
                    assert roi[ii, 1] <= roi[ii, 3]
                else:
                    roi[ii, 1:] = [w_start, h_start, w_end, h_end]
                    roi[ii, 1:] /= spatial_scale
                    roi[ii, 1:] = np.round(10 * roi[ii, 1:]) / 10
                    assert roi[ii, 1] <= roi[ii, 3]
                    assert roi[ii, 2] <= roi[ii, 4]

            caffe2_preds = get_caffe2_predictions_roi_align(copy.deepcopy(X), copy.deepcopy(roi), params)
            coreml_model, eval = get_coreml_model_roi_align(params)
            if eval is False:
                failed_tests_compile.append(params)
            else:
                input_dict = {'data': np.expand_dims(X, axis=0)}
                input_dict['roi'] = np.reshape(roi, (n_roi,1,-1,1,1))
                ref_output_dict = {'output': np.expand_dims(caffe2_preds, axis=0)}
                shape_match, numerical_match = self._test_model(input_dict, ref_output_dict, coreml_model)
                if not shape_match:
                    failed_tests_shape.append(params)
                if not numerical_match:
                    # print(params)
                    # print(np.squeeze(input_dict['roi']))
                    # print(np.squeeze(input_dict['data']))
                    # import sys
                    # sys.exit(1)
                    failed_tests_numerical.append(params)

        self.assertEqual(failed_tests_compile, [])
        self.assertEqual(failed_tests_shape, [])
        self.assertEqual(failed_tests_numerical, [])