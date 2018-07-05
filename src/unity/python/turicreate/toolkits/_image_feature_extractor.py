# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

from . import _mxnet_utils


def _create_feature_extractor(model_name):
    import os
    from platform import system
    from ._internal_utils import _mac_ver
    from ._pre_trained_models import MODELS, _get_model_cache_dir
    from turicreate.config import get_runtime_config
    from turicreate import extensions

    # If we don't have Core ML, use an MxNet model.
    if system() != 'Darwin' or _mac_ver() < (10, 13):
        ptModel = MODELS[model_name]()
        return MXFeatureExtractor(ptModel)

    download_path = _get_model_cache_dir()

    if(model_name == 'resnet-50'):
        mlmodel_resnet_save_path = download_path + "/Resnet50.mlmodel"
        if not os.path.exists(mlmodel_resnet_save_path):
            from turicreate.toolkits import _pre_trained_models
            mxnetResNet = _pre_trained_models.ResNetImageClassifier()
            feature_extractor = MXFeatureExtractor(mxnetResNet)
            mlModel = feature_extractor.get_coreml_model()
            mlModel.save(mlmodel_resnet_save_path)

    result = extensions.__dict__["image_deep_feature_extractor"]()
    result.init_options({'model_name': model_name, 'download_path': download_path})
    return result


class ImageFeatureExtractor(object):

    def __init__(self, ptModel):
        """
        Parameters
        ----------
        ptModel: ImageClassifierPreTrainedModel
            An instance of a pre-trained model.
        """
        pass

    def extract_features(self, dataset):
        """
        Parameters
        ----------
        dataset: SFrame
            SFrame with data to extract features from
        """
        pass

    def get_coreml_model(self):
        """
        Returns
        -------
        model:
            Return the underlying model in Core ML format
        """
        pass

class MXFeatureExtractor(ImageFeatureExtractor):

    def __init__(self, ptModel):
        """
        Parameters
        ----------
        ptModel: ImageClassifierPreTrainedModel
            An instance of a pre-trained model.
        """
        self.ptModel = ptModel
        self.data_layer = ptModel.data_layer
        self.feature_layer = ptModel.feature_layer
        self.image_shape = ptModel.input_image_shape
        self.context = _mxnet_utils.get_mxnet_context()

    @staticmethod
    def _get_mx_module(mxmodel, data_layer, feature_layer, context,
            image_shape, batch_size = 1, label_layer = None):
        import mxnet as _mx
        sym, arg_params, aux_params = mxmodel
        all_layers = sym.get_internals()
        feature_layer_sym = all_layers[feature_layer]

        # Add a feature label layer (for exporting)
        if (label_layer is not None) and (label_layer not in arg_params):
            arg_params[label_layer] = _mx.nd.array([0])

        # Make sure we do not use a number of contexts that could leave empty workloads
        context = context[:batch_size]
        model = _mx.mod.Module(symbol=feature_layer_sym, label_names=None, context=context)
        model.bind(for_training=False, data_shapes=[(data_layer, (batch_size, ) + image_shape)])
        model.set_params(arg_params, aux_params)
        return model

    def extract_features(self, dataset, feature, batch_size=64, verbose=False):
        """
        Parameters
        ----------
        dataset: SFrame
            SFrame of images
        """
        from ..mx import SFrameImageIter as _SFrameImageIter
        from six.moves.queue import Queue as _Queue
        from threading import Thread as _Thread
        import turicreate as _tc
        import array

        if len(dataset) == 0:
            return _tc.SArray([], array.array)

        batch_size = min(len(dataset), batch_size)
        # Make a data iterator
        dataIter = _SFrameImageIter(sframe=dataset, data_field=[feature], batch_size=batch_size, image_shape=self.image_shape)

        # Setup the MXNet model
        model = MXFeatureExtractor._get_mx_module(self.ptModel.mxmodel,
                self.data_layer, self.feature_layer, self.context, self.image_shape, batch_size)

        out = _tc.SArrayBuilder(dtype = array.array)
        progress = { 'num_processed' : 0, 'total' : len(dataset) }
        if verbose:
            print("Performing feature extraction on resized images...")

        # Encapsulates the work done by the MXNet model for a single batch
        def handle_request(batch):
            model.forward(batch)
            mx_out = [array.array('d',m) for m in model.get_outputs()[0].asnumpy()]
            if batch.pad != 0:
                # If batch size is not evenly divisible by the length, it will loop back around.
                # We don't want that.
                mx_out = mx_out[:-batch.pad]
            return mx_out

        # Copies the output from MXNet into the SArrayBuilder and emits progress
        def consume_response(mx_out):
            out.append_multiple(mx_out)

            progress['num_processed'] += len(mx_out)
            if verbose:
                print('Completed {num_processed:{width}d}/{total:{width}d}'.format(
                    width = len(str(progress['total'])), **progress))

        # Create a dedicated thread for performing MXNet work, using two FIFO
        # queues for communication back and forth with this thread, with the
        # goal of keeping MXNet busy throughout.
        request_queue = _Queue()
        response_queue = _Queue()
        def mx_worker():
            while True:
                batch = request_queue.get()  # Consume request
                if batch is None:
                    # No more work remains. Allow the thread to finish.
                    return
                response_queue.put(handle_request(batch))  # Produce response
        mx_worker_thread = _Thread(target=mx_worker)
        mx_worker_thread.start()

        try:
            # Attempt to have two requests in progress at any one time (double
            # buffering), so that the iterator is creating one batch while MXNet
            # performs inference on the other.
            if dataIter.has_next:
                request_queue.put(next(dataIter))  # Produce request
                while dataIter.has_next:
                    request_queue.put(next(dataIter))  # Produce request
                    consume_response(response_queue.get())
                consume_response(response_queue.get())
        finally:
            # Tell the worker thread to shut down.
            request_queue.put(None)

        return out.close()

    def get_coreml_model(self, mode = 'classifier'):
        """
        Parameters
        ----------
        mode: str ('classifier', 'regressor' or None)
            Mode of the converted coreml model.
            When mode = 'classifier', a NeuralNetworkClassifier spec will be constructed.
            When mode = 'regressor', a NeuralNetworkRegressor spec will be constructed.

        Returns
        -------
        model: MLModel
            Return the underlying model.
        """
        from ._mxnet_to_coreml import _mxnet_converter
        import mxnet as _mx

        (sym, arg_params, aux_params) = self.ptModel.mxmodel
        fe_mxmodel = self.ptModel.mxmodel

        if self.ptModel.is_feature_layer_final:
            feature_layer_size = self.ptModel.feature_layer_size
            num_dummy_classes = 10
            feature_layer_sym = sym.get_children()[0]
            fc_symbol = _mx.symbol.FullyConnected(feature_layer_sym, num_hidden=num_dummy_classes)
            prob = _mx.symbol.SoftmaxOutput(fc_symbol, name = sym.name, attr=sym.attr_dict()[sym.name])
            arg_params['%s_weight' % fc_symbol.name] = _mx.ndarray.zeros((num_dummy_classes, feature_layer_size))
            arg_params['%s_bias' % fc_symbol.name] = _mx.ndarray.zeros((num_dummy_classes))
            fe_mxmodel = (prob, arg_params, aux_params)

        model = MXFeatureExtractor._get_mx_module(fe_mxmodel,
                self.data_layer, self.ptModel.output_layer, _mxnet_utils.get_mxnet_context(max_devices=1),
                self.image_shape, label_layer = self.ptModel.label_layer)

        preprocessor_args = {'image_input_names': [self.data_layer]}
        return _mxnet_converter.convert(model, mode = 'classifier',
                input_shape=[(self.data_layer, (1, ) + self.image_shape)],
                class_labels = list(map(str, range(self.ptModel.num_classes))),
                preprocessor_args = preprocessor_args, verbose = False)
