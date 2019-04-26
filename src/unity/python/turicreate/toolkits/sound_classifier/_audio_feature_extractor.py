import time as _time

from coremltools.models import MLModel
import mxnet as mx
import numpy as _np
import turicreate as _tc
from mxnet.gluon import nn

from .._internal_utils import _mac_ver
from .._mxnet import _mxnet_utils
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
    name = 'VGGish'
    output_length = 12288

    @staticmethod
    def _preprocess_data(audio_data, verbose=True):
        '''
        Preprocess each example, breaking it up into frames.

        Returns two numpy arrays: preprocessed frame and their indexes
        '''
        from .vggish_input import waveform_to_examples

        last_progress_update = _time.time()
        progress_header_printed = False

        # Can't run as a ".apply(...)" due to numba.jit decorator issue:
        # https://github.com/apple/turicreate/issues/1216
        preprocessed_data, audio_data_index = [], []
        for i, audio_dict in enumerate(audio_data):
            scaled_data = audio_dict['data'] / 32768.0
            data = waveform_to_examples(scaled_data, audio_dict['sample_rate'])

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
            print("Preprocessed {} of {} examples\n".format(len(audio_data), len(audio_data)))
        return _np.asarray(preprocessed_data), audio_data_index

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
        from mxnet.gluon import utils

        if _mac_ver() < (10, 14):
            # Use MXNet
            preprocessed_data = mx.nd.array(preprocessed_data)

            ctx_list = self.ctx
            if len(preprocessed_data) < len(ctx_list):
                ctx_list = ctx_list[:len(preprocessed_data)]
            batches = utils.split_and_load(preprocessed_data, ctx_list=ctx_list, even_split=False)

            for i, cur_batch in enumerate(batches):
                y = self.vggish_model.forward(cur_batch).asnumpy()
                for j in y:
                    deep_features.append(j)

                # If `verbose` is set, print an progress update about every 20s
                if verbose and _time.time() - last_progress_update >= 20:
                    if not progress_header_printed:
                        print("Extracting deep features -")
                        progress_header_printed = True
                    print("Extracted {} of {} batches".format(i, len(batches)))
                    last_progress_update = _time.time()
            if progress_header_printed:
                print("Extracted {} of {} batches\n".format(len(batches), len(batches)))

        else:
            # Use Core ML
            for i, cur_example in enumerate(preprocessed_data):
                for cur_frame in cur_example:
                    x = {'input1': [cur_frame]}
                    y = self.vggish_model.predict(x)
                    deep_features.append(y['output1'])

                # If `verbose` is set, print an progress update about every 20s
                if verbose and _time.time() - last_progress_update >= 20:
                    if not progress_header_printed:
                        print("Extracting deep features -")
                        progress_header_printed = True
                    print("Extracted {} of {}".format(i, len(preprocessed_data)))
                    last_progress_update = _time.time()
            if progress_header_printed:
                print("Extracted {} of {}\n".format(len(preprocessed_data), len(preprocessed_data)))

        return deep_features.close()

    def get_deep_features(self, audio_data, verbose):
        '''
        Performs both audio preprocessing and VGGish deep feature extraction.
        '''
        preprocessed_data, row_ids = self._preprocess_data(audio_data, verbose)
        deep_features = self._extract_features(preprocessed_data, verbose)

        output = _tc.SFrame({'deep features': deep_features, 'row id': row_ids})
        output = output.unstack('deep features')

        max_row_id = len(audio_data)
        missing_ids = set(range(max_row_id)) - set(output['row id'].unique())
        if len(missing_ids) != 0:
            empty_rows = _tc.SFrame({'List of deep features': [ [] for _ in range(len(missing_ids)) ],
                                     'row id': missing_ids})
            output = output.append(empty_rows)

        output = output.sort('row id')
        return output['List of deep features']
    
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
