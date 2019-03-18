import time as _time

from coremltools.models import MLModel
import mxnet as mx
from mxnet.gluon import nn, utils

from .._internal_utils import _mac_ver
from .. import _mxnet_utils
from .._pre_trained_models import VGGish


VGGish_instance = None
def _get_feature_extractor(model_name):
    global VGGish_instance
    assert model_name == 'VGGish'
    if VGGish_instance is None:
        VGGish_instance = VGGishFeatureExtractor()
    return VGGish_instance

class Flatten_channel_last(nn.HybridBlock):
    def __init__(self, **kwargs):
        super(Flatten_channel_last, self).__init__(**kwargs)

    def hybrid_forward(self, F, x):
        x = mx.ndarray.swapaxes(x, 1, 2)
        x = mx.ndarray.swapaxes(x, 2, 3)
        return x.flatten()

    def __repr__(self):
        return self.__class__.__name__


class VGGishFeatureExtractor(object):
    output_length = 12288

    @staticmethod
    def preprocess_data(audio_data, labels, verbose=True):
        '''
        Preprocess each example, breaking it up into frames.

        Returns two numpy arrays: preprocessed frame and their labels.
        '''
        from .vggish_input import waveform_to_examples
        import numpy as np

        last_progress_update = _time.time()

        # Can't run as a ".apply(...)" due to numba.jit decorator issue:
        # https://github.com/apple/turicreate/issues/1216
        preprocessed_data, output_labels = [], []
        for i, audio_dict in enumerate(audio_data):
            scaled_data = audio_dict['data'] / 32768.0
            data = waveform_to_examples(scaled_data, audio_dict['sample_rate'])

            for j in data:
                preprocessed_data.append([j])
                output_labels.append(labels[i])

            # If `verbose` is set, print an progress update about every 20s
            if verbose and _time.time() - last_progress_update >= 20:
                print("Preprocessed {} of {} examples".format(i, len(audio_data)))
                last_progress_update = _time.time()

        return np.asarray(preprocessed_data), np.asarray(output_labels)

    @staticmethod
    def _build_net():
        net = nn.HybridSequential()
        net.add(nn.Conv2D(channels=64, kernel_size=(3, 3), in_channels=1, padding=(1, 1), prefix='vggish_conv0_'))
        net.add(nn.Activation('relu'))
        net.add(nn.MaxPool2D())
        net.add(nn.Conv2D(channels=128, kernel_size=(3, 3), in_channels=64, padding=(1, 1), prefix='vggish_conv1_'))
        net.add(nn.Activation('relu'))
        net.add(nn.MaxPool2D())
        net.add(nn.Conv2D(channels=256, kernel_size=(3, 3), in_channels=128, padding=(1, 1), prefix='vggish_conv2_'))
        net.add(nn.Activation('relu'))
        net.add(nn.Conv2D(channels=256, kernel_size=(3, 3), in_channels=256, padding=(1, 1), prefix='vggish_conv3_'))
        net.add(nn.Activation('relu'))
        net.add(nn.MaxPool2D())
        net.add(nn.Conv2D(channels=512, kernel_size=(3, 3), in_channels=256, padding=(1, 1), prefix='vggish_conv4_'))
        net.add(nn.Activation('relu'))
        net.add(nn.Conv2D(channels=512, kernel_size=(3, 3), in_channels=512, padding=(1, 1), prefix='vggish_conv5_'))
        net.add(nn.Activation('relu'))
        net.add(nn.MaxPool2D())
        net.add(Flatten_channel_last())
        return net

    def __init__(self):
        vggish_model_file = VGGish()

        if _mac_ver() < (10, 14):
            # Use MXNet
            model_path = vggish_model_file.get_model_path(format='mxnet')
            self.vggish_model = VGGishFeatureExtractor._build_net()
            net_params = self.vggish_model.collect_params()
            self.ctx = _mxnet_utils.get_mxnet_context()
            net_params.load(model_path, ctx=self.ctx)
        else:
            # Use Core ML
            model_path = vggish_model_file.get_model_path(format='coreml')
            self.vggish_model = MLModel(model_path)

    def extract_features(self, preprocessed_data, verbose=True):
        """
        Parameters
        ----------
        preprocessed_data : SArray

        Returns
        -------
        numpy array containing the deep features
        """
        import numpy as np

        last_progress_update = _time.time()

        if _mac_ver() < (10, 14):
            # Use MXNet
            preprocessed_data = mx.nd.array(preprocessed_data)

            ctx_list = self.ctx
            if len(preprocessed_data) < len(ctx_list):
                ctx_list = ctx_list[:len(preprocessed_data)]
            batches = utils.split_and_load(preprocessed_data, ctx_list=ctx_list, even_split=False)

            deep_features = []
            for i, cur_batch in enumerate(batches):
                y = self.vggish_model.forward(cur_batch).asnumpy()
                for j in y:
                    deep_features.append(j)

                # If `verbose` is set, print an progress update about every 20s
                if verbose and _time.time() - last_progress_update >= 20:
                    print("Extracted {} of {} batches".format(i, len(batches)))
                    last_progress_update = _time.time()

        else:
            # Use Core ML
            deep_features = []
            for i, cur_example in enumerate(preprocessed_data):
                for cur_frame in cur_example:
                    x = {'input1': [cur_frame]}
                    y = self.vggish_model.predict(x)
                    deep_features.append(y['output1'])

                # If `verbose` is set, print an progress update about every 20s
                if verbose and _time.time() - last_progress_update >= 20:
                    print("Extracted {} of {}".format(i, len(preprocessed_data)))
                    last_progress_update = _time.time()

        return np.asarray(deep_features)

    def get_spec(self):
        """
        Return the Core ML spec
        """
        if _mac_ver() >= (10, 14):
            return self.vggish_model.get_spec()
        else:
            vggish_model_file = VGGish()
            coreml_model_path = vggish_model_file.get_model_path(format='coreml')
            return MLModel(coreml_model_path).get_spec()
