import mxnet as mx
from mxnet.gluon import HybridBlock
import numpy as np
from mxnet.gluon import nn
import json

mps_input = np.random.random_sample((1, 9, 9, 128))
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

class TestingDecoding(HybridBlock):
    def __init__(self):
        super(TestingDecoding, self).__init__(prefix='decoding_')
        with self.name_scope():
            self.decoder_conv1 = nn.Conv2D(64, 3, 1, 1, in_channels=128, use_bias=False)
            self.inst_norm1 = InstanceNorm(in_channels=64, num_styles=8, batch_size=1)
            
    def hybrid_forward(self, F, x, style_idx):
        h1 = F.UpSampling(x, scale=2, sample_type='nearest')
        h1 = self.decoder_conv1(h1)
        h1 = self.inst_norm1(h1, style_idx)
        h1 = F.Activation(h1, 'relu')
        return h1

net = TestingDecoding()
net.collect_params().initialize()
output = net(mx.nd.array(mx_input), mx.nd.array([0]))

decoding_weight_dict = dict()

param_keys = list(net.collect_params())
for key in param_keys:
    weight = net.collect_params()[key].data().asnumpy()
    if len(weight.shape) == 4:
        decoding_weight_dict[key] = weight.transpose(0, 2, 3, 1)
    else:
        decoding_weight_dict[key] = weight

json_weights = {
    "decode_conv_weights" : decoding_weight_dict["decoding_conv0_weight"].flatten().tolist(),
    "decode_inst_beta" : decoding_weight_dict["decoding_instancenorm0_beta"].flatten().tolist(),
    "decode_inst_gamma" : decoding_weight_dict["decoding_instancenorm0_gamma"].flatten().tolist()
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

with open('data/decode/test_1/weights.json', 'w') as fp:
    json.dump(json_weights, fp)

with open('data/decode/test_1/outputs.json', 'w') as fp:
    json.dump(json_outputs, fp)

with open('data/decode/test_1/inputs.json', 'w') as fp:
    json.dump(json_inputs, fp)