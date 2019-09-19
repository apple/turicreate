import mxnet as mx
from mxnet.gluon import HybridBlock
import numpy as np
from mxnet.gluon import nn
import json

mps_input = np.random.random_sample((1, 32, 32, 3))
mx_input = mps_input.transpose(0, 3, 1, 2)

class InstanceNorm(HybridBlock):
    """
    Conditional Instance Norm
    """
    def __init__(self, in_channels, num_styles, batch_size, epsilon=1e-5,
                 center=True, scale=True, beta_initializer='zeros',
                 gamma_initializer='ones', **kwargs):
        super(InstanceNorm, self).__init__(**kwargs)
        self._kwargs = {'eps': epsilon}
        self.gamma = self.params.get('gamma', grad_req='write' if scale else 'null',
                                     shape=(num_styles, in_channels, ), init=gamma_initializer,
                                     allow_deferred_init=True)
        self.beta = self.params.get('beta', grad_req='write' if center else 'null',
                                    shape=(num_styles, in_channels, ), init=beta_initializer,
                                    allow_deferred_init=True)
        self.num_styles = num_styles
        self.in_channels = in_channels
        self.batch_size = batch_size

    def hybrid_forward(self, F, X, style_idx, gamma, beta):
        if F == mx.sym and self.batch_size == 0:  # for coreml
            gamma = mx.sym.Embedding(data=style_idx, input_dim=self.num_styles, output_dim=self.in_channels)
            beta = mx.sym.Embedding(data=style_idx, input_dim=self.num_styles, output_dim=self.in_channels)
            return F.InstanceNorm(X, gamma, beta, name='_fwd', **self._kwargs)

        em_gamma = F.take(gamma, indices=style_idx, axis=0)
        em_beta = F.take(beta, indices=style_idx, axis=0)
        
        sp_gammas = F.split(em_gamma, axis=0, num_outputs=self.batch_size, squeeze_axis=True)
        sp_betas = F.split(em_beta, axis=0, num_outputs=self.batch_size, squeeze_axis=True)
        
        if self.batch_size == 1:
            return F.InstanceNorm(X, sp_gammas, sp_betas, name='_fwd', **self._kwargs)
        else:
            Xs = F.split(X, axis=0, num_outputs=self.batch_size)

            res = []
            for idx in range(self.batch_size):
                gamma0 = sp_gammas[idx]
                beta0 = sp_betas[idx]
                X_slice = Xs[idx]
                res.append(F.InstanceNorm(X_slice, gamma0, beta0, name='_fwd', **self._kwargs))

            return F.concat(*res, dim=0)

class ResidualBlock(HybridBlock):
    """
    Residual network
    """

    def __init__(self, num_styles, batch_size):
        super(ResidualBlock, self).__init__()

        with self.name_scope():
            self.conv1 = nn.Conv2D(128, 3, 1, 1, in_channels=128, use_bias=False)
            self.inst_norm1 = InstanceNorm(in_channels=128, num_styles=num_styles, batch_size=batch_size)
            self.conv2 = nn.Conv2D(128, 3, 1, 1, in_channels=128, use_bias=False)
            self.inst_norm2 = InstanceNorm(in_channels=128, num_styles=num_styles, batch_size=batch_size)

        self._batch_size = batch_size

    @property
    def batch_size(self):
        return self._batch_size

    @batch_size.setter
    def batch_size(self, batch_size):
        self.inst_norm1.batch_size = batch_size
        self.inst_norm2.batch_size = batch_size
        self._batch_size = batch_size

    def hybrid_forward(self, F, x, style_idx):
        h1 = self.conv1(x)
        h1 = self.inst_norm1(h1, style_idx)
        h1 = F.Activation(h1, 'relu')

        h2 = self.conv2(h1)
        h2 = self.inst_norm2(h2, style_idx)

        return x + h2


class Transformer(HybridBlock):
    def __init__(self, num_styles, batch_size):
        super(Transformer, self).__init__(prefix='transformer_')
        self.num_styles = num_styles
        block = ResidualBlock
        self.scale255 = False

        with self.name_scope():
            self.conv1 = nn.Conv2D(32, 9, 1, 4, in_channels=3, use_bias=False)
            self.inst_norm1 = InstanceNorm(in_channels=32, num_styles=num_styles, batch_size=batch_size)

            self.conv2 = nn.Conv2D(64, 3, 2, 1, in_channels=32, use_bias=False)
            self.inst_norm2 = InstanceNorm(in_channels=64, num_styles=num_styles, batch_size=batch_size)

            self.conv3 = nn.Conv2D(128, 3, 2, 1, in_channels=64, use_bias=False)
            self.inst_norm3 = InstanceNorm(in_channels=128, num_styles=num_styles, batch_size=batch_size)

            self.residual1 = block(num_styles, batch_size=batch_size)
            self.residual2 = block(num_styles, batch_size=batch_size)
            self.residual3 = block(num_styles, batch_size=batch_size)
            self.residual4 = block(num_styles, batch_size=batch_size)
            self.residual5 = block(num_styles, batch_size=batch_size)

            self.decoder_conv1 = nn.Conv2D(64, 3, 1, 1, in_channels=128, use_bias=False)
            self.inst_norm4 = InstanceNorm(in_channels=64, num_styles=num_styles, batch_size=batch_size)

            self.decoder_conv2 = nn.Conv2D(32, 3, 1, 1, in_channels=64, use_bias=False)
            self.inst_norm5 = InstanceNorm(in_channels=32, num_styles=num_styles, batch_size=batch_size)

            self.decoder_conv3 = nn.Conv2D(3, 9, 1, 4, in_channels=32, use_bias=False)
            self.inst_norm6 = InstanceNorm(in_channels=3, num_styles=num_styles, batch_size=batch_size)

    @property
    def batch_size(self):
        return self._batch_size

    @batch_size.setter
    def batch_size(self, batch_size):
        inst_norm_layers = [
            self.inst_norm1, self.inst_norm2, self.inst_norm3,
            self.inst_norm4, self.inst_norm5, self.inst_norm6,
            self.residual1, self.residual2, self.residual3,
            self.residual4, self.residual5,
        ]
        for layer in inst_norm_layers:
            layer.batch_size = batch_size

    def hybrid_forward(self, F, X, style_idx):
        h1 = self.conv1(X)
        h1 = self.inst_norm1(h1, style_idx)
        h1 = F.Activation(h1, 'relu')
        
        h2 = self.conv2(h1)
        h2 = self.inst_norm2(h2, style_idx)
        h2 = F.Activation(h2, 'relu')
        
        h3 = self.conv3(h2)
        h3 = self.inst_norm3(h3, style_idx)
        h3 = F.Activation(h3, 'relu')

        r1 = self.residual1(h3, style_idx)
        r2 = self.residual2(r1, style_idx)
        r3 = self.residual3(r2, style_idx)
        r4 = self.residual4(r3, style_idx)
        r5 = self.residual5(r4, style_idx)

        d1 = F.UpSampling(r5, scale=2, sample_type='nearest')
        d1 = self.decoder_conv1(d1)
        d1 = self.inst_norm4(d1, style_idx)
        d1 = F.Activation(d1, 'relu')

        d2 = F.UpSampling(d1, scale=2, sample_type='nearest')
        d2 = self.decoder_conv2(d2)
        d2 = self.inst_norm5(d2, style_idx)
        d2 = F.Activation(d2, 'relu')

        d3 = self.decoder_conv3(d2)
        d3 = self.inst_norm6(d3, style_idx)

        z = F.Activation(d3, 'sigmoid')
        if self.scale255:
            return z * 255
        else:
            return z

net = Transformer(8, 1)
net.collect_params().initialize()
output = net(mx.nd.array(mx_input), mx.nd.array([0]))

transformer_weight_dict = dict()

param_keys = list(net.collect_params())
for key in param_keys:
    weight = net.collect_params()[key].data().asnumpy()
    if len(weight.shape) == 4:
        transformer_weight_dict[key] = weight.transpose(0, 2, 3, 1)
    else:
        transformer_weight_dict[key] = weight

json_weights= {
    "transformer_encode_1_conv_weights": transformer_weight_dict["transformer_conv0_weight"].flatten().tolist(),
    "transformer_encode_1_inst_beta": transformer_weight_dict["transformer_instancenorm0_beta"].flatten().tolist(),
    "transformer_encode_1_inst_gamma": transformer_weight_dict["transformer_instancenorm0_gamma"].flatten().tolist(),
    "transformer_encode_2_conv_weights": transformer_weight_dict["transformer_conv1_weight"].flatten().tolist(),
    "transformer_encode_2_inst_beta": transformer_weight_dict["transformer_instancenorm1_beta"].flatten().tolist(),
    "transformer_encode_2_inst_gamma": transformer_weight_dict["transformer_instancenorm1_gamma"].flatten().tolist(),
    "transformer_encode_3_conv_weights": transformer_weight_dict["transformer_conv2_weight"].flatten().tolist(),
    "transformer_encode_3_inst_beta": transformer_weight_dict["transformer_instancenorm2_beta"].flatten().tolist(),
    "transformer_encode_3_inst_gamma": transformer_weight_dict["transformer_instancenorm2_gamma"].flatten().tolist(),
    "transformer_residual_1_conv_1_weights": transformer_weight_dict["transformer_residualblock0_conv0_weight"].flatten().tolist(),
    "transformer_residual_1_conv_2_weights": transformer_weight_dict["transformer_residualblock0_conv1_weight"].flatten().tolist(),
    "transformer_residual_1_inst_1_beta": transformer_weight_dict["transformer_residualblock0_instancenorm0_beta"].flatten().tolist(),
    "transformer_residual_1_inst_1_gamma": transformer_weight_dict["transformer_residualblock0_instancenorm0_gamma"].flatten().tolist(),
    "transformer_residual_1_inst_2_beta": transformer_weight_dict["transformer_residualblock0_instancenorm1_beta"].flatten().tolist(),
    "transformer_residual_1_inst_2_gamma": transformer_weight_dict["transformer_residualblock0_instancenorm1_gamma"].flatten().tolist(),
    "transformer_residual_2_conv_1_weights": transformer_weight_dict["transformer_residualblock1_conv0_weight"].flatten().tolist(),
    "transformer_residual_2_conv_2_weights": transformer_weight_dict["transformer_residualblock1_conv1_weight"].flatten().tolist(),
    "transformer_residual_2_inst_1_beta": transformer_weight_dict["transformer_residualblock1_instancenorm0_beta"].flatten().tolist(),
    "transformer_residual_2_inst_1_gamma": transformer_weight_dict["transformer_residualblock1_instancenorm0_gamma"].flatten().tolist(),
    "transformer_residual_2_inst_2_beta": transformer_weight_dict["transformer_residualblock1_instancenorm1_beta"].flatten().tolist(),
    "transformer_residual_2_inst_2_gamma": transformer_weight_dict["transformer_residualblock1_instancenorm1_gamma"].flatten().tolist(),
    "transformer_residual_3_conv_1_weights": transformer_weight_dict["transformer_residualblock2_conv0_weight"].flatten().tolist(),
    "transformer_residual_3_conv_2_weights": transformer_weight_dict["transformer_residualblock2_conv1_weight"].flatten().tolist(),
    "transformer_residual_3_inst_1_beta": transformer_weight_dict["transformer_residualblock2_instancenorm0_beta"].flatten().tolist(),
    "transformer_residual_3_inst_1_gamma": transformer_weight_dict["transformer_residualblock2_instancenorm0_gamma"].flatten().tolist(),
    "transformer_residual_3_inst_2_beta": transformer_weight_dict["transformer_residualblock2_instancenorm1_beta"].flatten().tolist(),
    "transformer_residual_3_inst_2_gamma": transformer_weight_dict["transformer_residualblock2_instancenorm1_gamma"].flatten().tolist(),
    "transformer_residual_4_conv_1_weights": transformer_weight_dict["transformer_residualblock3_conv0_weight"].flatten().tolist(),
    "transformer_residual_4_conv_2_weights": transformer_weight_dict["transformer_residualblock3_conv1_weight"].flatten().tolist(),
    "transformer_residual_4_inst_1_beta": transformer_weight_dict["transformer_residualblock3_instancenorm0_beta"].flatten().tolist(),
    "transformer_residual_4_inst_1_gamma": transformer_weight_dict["transformer_residualblock3_instancenorm0_gamma"].flatten().tolist(),
    "transformer_residual_4_inst_2_beta": transformer_weight_dict["transformer_residualblock3_instancenorm1_beta"].flatten().tolist(),
    "transformer_residual_4_inst_2_gamma": transformer_weight_dict["transformer_residualblock3_instancenorm1_gamma"].flatten().tolist(),
    "transformer_residual_5_conv_1_weights": transformer_weight_dict["transformer_residualblock4_conv0_weight"].flatten().tolist(),
    "transformer_residual_5_conv_2_weights": transformer_weight_dict["transformer_residualblock4_conv1_weight"].flatten().tolist(),
    "transformer_residual_5_inst_1_beta": transformer_weight_dict["transformer_residualblock4_instancenorm0_beta"].flatten().tolist(),
    "transformer_residual_5_inst_1_gamma": transformer_weight_dict["transformer_residualblock4_instancenorm0_gamma"].flatten().tolist(),
    "transformer_residual_5_inst_2_beta": transformer_weight_dict["transformer_residualblock4_instancenorm1_beta"].flatten().tolist(),
    "transformer_residual_5_inst_2_gamma": transformer_weight_dict["transformer_residualblock4_instancenorm1_gamma"].flatten().tolist(),
    "transformer_decoding_1_conv_weights": transformer_weight_dict["transformer_conv3_weight"].flatten().tolist(),
    "transformer_decoding_2_conv_weights": transformer_weight_dict["transformer_conv4_weight"].flatten().tolist(),
    "transformer_conv5_weight": transformer_weight_dict["transformer_conv5_weight"].flatten().tolist(),
    "transformer_decoding_1_inst_beta": transformer_weight_dict["transformer_instancenorm3_beta"].flatten().tolist(),
    "transformer_decoding_1_inst_gamma": transformer_weight_dict["transformer_instancenorm3_gamma"].flatten().tolist(),
    "transformer_decoding_2_inst_beta": transformer_weight_dict["transformer_instancenorm4_beta"].flatten().tolist(),
    "transformer_decoding_2_inst_gamma": transformer_weight_dict["transformer_instancenorm4_gamma"].flatten().tolist(),
    "transformer_instancenorm5_beta": transformer_weight_dict["transformer_instancenorm5_beta"].flatten().tolist(),
    "transformer_instancenorm5_gamma": transformer_weight_dict["transformer_instancenorm5_gamma"].flatten().tolist()
}

json_inputs = {
	"content": mps_input.flatten().tolist(),
	"height": mps_input.shape[1],
	"width": mps_input.shape[2],
	"channels": mps_input.shape[3]
}

json_outputs = {
	"output": output.asnumpy().transpose(0, 2, 3, 1).flatten().tolist()
}

with open('data/resnet/test_1/weights.json', 'w') as fp:
    json.dump(json_weights, fp)

with open('data/resnet/test_1/outputs.json', 'w') as fp:
    json.dump(json_outputs, fp)

with open('data/resnet/test_1/inputs.json', 'w') as fp:
    json.dump(json_inputs, fp)