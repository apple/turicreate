import time as _time

from coremltools.models import MLModel
import numpy as _np
from tensorflow import keras as _keras

# Suppresses verbosity to only errors
import turicreate.toolkits._tf_utils as _utils

_utils.suppress_tensorflow_warnings()
import turicreate as _tc

from .vggish_params import SAMPLE_RATE
from .._internal_utils import _mac_ver
from .._pre_trained_models import VGGish


# We need to disable this here to match behavior in the rest of TuriCreate
from tensorflow.compat.v1 import disable_v2_behavior

disable_v2_behavior()


VGGish_instance = None


def _get_feature_extractor(model_name):
    global VGGish_instance
    assert model_name == "VGGish"
    if VGGish_instance is None:
        VGGish_instance = VGGishFeatureExtractor()
    return VGGish_instance


class VGGishFeatureExtractor(object):
    name = "VGGish"
    output_length = 12288
    input_sample_rate = SAMPLE_RATE

    @staticmethod
    def _preprocess_data(audio_data, verbose=True):
        """
        Preprocess each example, breaking it up into frames.

        Returns two numpy arrays: preprocessed frame and their indexes
        """
        from .vggish_input import waveform_to_examples

        last_progress_update = _time.time()
        progress_header_printed = False

        # Can't run as a ".apply(...)" due to numba.jit decorator issue:
        # https://github.com/apple/turicreate/issues/1216
        preprocessed_data, audio_data_index = [], []
        for i, audio_dict in enumerate(audio_data):
            scaled_data = audio_dict["data"] / 32768.0
            data = waveform_to_examples(scaled_data, audio_dict["sample_rate"])

            for j in data:
                preprocessed_data.append([j])
                audio_data_index.append(i)

            # If `verbose` is set, print an progress update about every 20s
            if verbose and _time.time() - last_progress_update >= 20:
                if not progress_header_printed:
                    print("Preprocessing audio data -")
                    progress_header_printed = True
                print("Preprocessed {} of {} examples".format(i, len(audio_data)))
                last_progress_update = _time.time()

        if progress_header_printed:
            print(
                "Preprocessed {} of {} examples\n".format(
                    len(audio_data), len(audio_data)
                )
            )
        return _np.asarray(preprocessed_data), audio_data_index

    def __init__(self):
        vggish_model_file = VGGish()
        self.mac_ver = _mac_ver()

        if self.mac_ver < (10, 14):
            # Use TensorFlow/Keras
            import turicreate.toolkits._tf_utils as _utils

            self.gpu_policy = _utils.TensorFlowGPUPolicy()
            self.gpu_policy.start()

            model_path = vggish_model_file.get_model_path(format="tensorflow")
            self.vggish_model = _keras.models.load_model(model_path)
        else:
            # Use Core ML
            model_path = vggish_model_file.get_model_path(format="coreml")
            self.vggish_model = MLModel(model_path)

    def __del__(self):
        if self.mac_ver < (10, 14):
            self.gpu_policy.stop()

    def _extract_features(self, preprocessed_data, verbose=True):
        """
        Parameters
        ----------
        preprocessed_data : SArray

        Returns
        -------
        numpy array containing the deep features
        """
        last_progress_update = _time.time()
        progress_header_printed = False

        deep_features = _tc.SArrayBuilder(_np.ndarray)

        if _mac_ver() < (10, 14):
            # Use TensorFlow/Keras

            # Transpose data from channel first to channel last
            preprocessed_data = _np.transpose(preprocessed_data, (0, 2, 3, 1))

            for i, cur_example in enumerate(preprocessed_data):
                y = self.vggish_model.predict([[cur_example]])
                deep_features.append(y[0])

                # If `verbose` is set, print an progress update about every 20s
                if verbose and _time.time() - last_progress_update >= 20:
                    if not progress_header_printed:
                        print("Extracting deep features -")
                        progress_header_printed = True
                    print("Extracted {} of {}".format(i, len(preprocessed_data)))
                    last_progress_update = _time.time()
            if progress_header_printed:
                print(
                    "Extracted {} of {}\n".format(
                        len(preprocessed_data), len(preprocessed_data)
                    )
                )

        else:
            # Use Core ML

            for i, cur_example in enumerate(preprocessed_data):
                for cur_frame in cur_example:
                    x = {"input1": _np.asarray([cur_frame])}
                    y = self.vggish_model.predict(x)
                    deep_features.append(y["output1"])

                # If `verbose` is set, print an progress update about every 20s
                if verbose and _time.time() - last_progress_update >= 20:
                    if not progress_header_printed:
                        print("Extracting deep features -")
                        progress_header_printed = True
                    print("Extracted {} of {}".format(i, len(preprocessed_data)))
                    last_progress_update = _time.time()
            if progress_header_printed:
                print(
                    "Extracted {} of {}\n".format(
                        len(preprocessed_data), len(preprocessed_data)
                    )
                )

        return deep_features.close()

    def get_deep_features(self, audio_data, verbose):
        """
        Performs both audio preprocessing and VGGish deep feature extraction.
        """
        preprocessed_data, row_ids = self._preprocess_data(audio_data, verbose)
        deep_features = self._extract_features(preprocessed_data, verbose)

        output = _tc.SFrame({"deep features": deep_features, "row id": row_ids})
        output = output.unstack("deep features")

        max_row_id = len(audio_data)
        missing_ids = set(range(max_row_id)) - set(output["row id"].unique())
        if len(missing_ids) != 0:
            empty_rows = _tc.SFrame(
                {
                    "List of deep features": [[] for _ in range(len(missing_ids))],
                    "row id": missing_ids,
                }
            )
            output = output.append(empty_rows)

        output = output.sort("row id")
        return output["List of deep features"]

    def get_spec(self):
        """
        Return the Core ML spec
        """
        if _mac_ver() >= (10, 14):
            return self.vggish_model.get_spec()
        else:
            vggish_model_file = VGGish()
            coreml_model_path = vggish_model_file.get_model_path(format="coreml")
            return MLModel(coreml_model_path).get_spec()
