# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from . import _mxnet_utils


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
        import mxnet as _mx
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

        model = _mx.mod.Module(symbol=feature_layer_sym, label_names=None, context=context)
        model.bind(for_training=False, data_shapes=[(data_layer, (batch_size, ) + image_shape)])
        model.set_params(arg_params, aux_params)
        return model

    def extract_features(self, dataset, feature, batch_size=512, verbose=False):
        """
        Parameters
        ----------
        dataset: SFrame
            SFrame of images
        """
        from ..mx import SFrameImageIter as _SFrameImageIter
        import turicreate as _tc
        import array

        if len(dataset) == 0:
            return _tc.SArray([], array.array)

        # Resize images if needed
        preprocessed_dataset =  _tc.SFrame()
        if verbose:
            print("Resizing images...")
        preprocessed_dataset[feature] = _tc.image_analysis.resize(
                dataset[feature],  *tuple(reversed(self.image_shape)))

        batch_size = min(len(dataset), batch_size)
        # Make a data iterator
        dataIter = _SFrameImageIter(sframe=preprocessed_dataset, data_field=[feature], batch_size=batch_size)

        # Setup the MXNet model
        model = MXFeatureExtractor._get_mx_module(self.ptModel.mxmodel,
                self.data_layer, self.feature_layer, self.context, self.image_shape, batch_size)

        out = _tc.SArrayBuilder(dtype = array.array)
        num_processed = 0
        if verbose:
            print("Performing feature extraction on resized images...")
        while dataIter.has_next:
            if dataIter.data_shape[1:] != self.image_shape:
                raise RuntimeError("Expected image of size %s. Got %s instead." % (
                                               self.image_shape, dataIter.data_shape[1:]))
            model.forward(next(dataIter))
            mx_out = [array.array('d',m) for m in model.get_outputs()[0].asnumpy()]
            if dataIter.getpad() != 0:
                # If batch size is not evenly divisible by the length, it will loop back around.
                # We don't want that.
                mx_out = mx_out[:-dataIter.getpad()]
            out.append_multiple(mx_out)

            num_processed += batch_size
            num_processed = min(len(dataset), num_processed)
            if verbose:
                print('Completed {num_processed:{width}d}/{total:{width}d}'.format(
                    num_processed = num_processed, total=len(dataset), width = len(str(len(dataset)))))

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
            feature_layer = self.ptModel.feature_layer
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
                input_shape={self.data_layer: (1, ) + self.image_shape},
                class_labels = list(map(str, range(self.ptModel.num_classes))),
                preprocessor_args = preprocessor_args, verbose = False)
