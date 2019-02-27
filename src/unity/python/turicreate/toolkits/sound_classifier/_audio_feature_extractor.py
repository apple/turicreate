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
    def preprocess_data(audio_data, labels):
        '''
        Preprocess each example, breaking it up into frames.

        Returns two numpy arrays: preprocessed frame and their labels.
        '''
        from .vggish_input import waveform_to_examples
        import numpy as np

        # Can't run as a ".apply(...)" due to numba.jit decorator issue:
        # https://github.com/apple/turicreate/issues/1216
        preprocessed_data, output_labels = [], []
        for i, audio_dict in enumerate(audio_data):
            scaled_data = audio_dict['data'] / 32768.0
            data = waveform_to_examples(scaled_data, audio_dict['sample_rate'])

            for j in data:
                preprocessed_data.append([j])
                output_labels.append(labels[i])

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

        if _mac_ver() < (10, 13):
            # Use MXNet
            model_path = vggish_model_file.get_model_path(format='mxnet')
            self.vggish_model = VGGishFeatureExtractor._build_net()
            net_params = self.vggish_model.collect_params()
            self.ctx = _mxnet_utils.get_mxnet_context()
            net_params.initialize(mx.init.Xavier(), ctx=self.ctx)    # TODO: verify on Linux that we don't need this line.
            net_params.load(model_path, ctx=self.ctx)
        else:
            # Use Core ML
            from coremltools.models import MLModel
            model_path = vggish_model_file.get_model_path(format='coreml')
            self.vggish_model = MLModel(model_path)

    def extract_features(self, preprocessed_data):
        """
        Parameters
        ----------
        preprocessed_data : SArray

        Returns
        -------
        numpy array containing the deep features
        """
        import numpy as np

        if _mac_ver() < (10, 13):
            # Use MXNet
            preprocessed_data = mx.nd.array(preprocessed_data)
            batches = utils.split_and_load(preprocessed_data, ctx_list=self.ctx, even_split=False)

            deep_features = []
            for cur_batch in batches:
                y = self.vggish_model.forward(cur_batch).asnumpy()
                for i in y:
                    deep_features.append(i)

        else:
            # Use Core ML
            deep_features = []
            for i, cur_example in enumerate(preprocessed_data):
                for cur_frame in cur_example:
                    x = {'input1': [cur_frame]}
                    y = self.vggish_model.predict(x)
                    deep_features.append(y['output1'])

        return np.asarray(deep_features)

    def get_spec(self):
        """
        Return the Core ML spec
        """
        if _mac_ver() >= (10, 13):
            return self.vggish_model.get_spec()
        else:
            # TODO: make this work on Linux
            assert(False)
