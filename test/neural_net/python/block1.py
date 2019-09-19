import mxnet as mx
from mxnet.gluon import HybridBlock
import numpy as np
from mxnet.gluon import nn
import json

mps_input = np.random.random_sample((1, 32, 32, 3))
mx_input = mps_input.transpose(0, 3, 1, 2)

class VGGBlock1(HybridBlock):
    """
    VGGBlock1 network
    """

    def __init__(self):
        super(VGGBlock1, self).__init__()

        with self.name_scope():
            self.conv1_1 = nn.Conv2D(in_channels=3, channels=64, kernel_size=3, padding=1)
            self.conv1_2 = nn.Conv2D(in_channels=64, channels=64, kernel_size=3, padding=1)
    
    def hybrid_forward(self, F, X):
        h = F.Activation(self.conv1_1(X), act_type='relu')
        h = F.Activation(self.conv1_2(h), act_type='relu')
        relu1_2 = h
        h = F.Pooling(h, pool_type='avg', kernel=(2, 2), stride=(2, 2))
        
        return h

net = VGGBlock1()
net.collect_params().initialize()
output = net(mx.nd.array(mx_input))

vgg_block_weight_dict = dict()

param_keys = list(net.collect_params())
for key in param_keys:
    weight = net.collect_params()[key].data().asnumpy()
    if len(weight.shape) == 4:
        vgg_block_weight_dict[key] = weight.transpose(0, 2, 3, 1)
    else:
        vgg_block_weight_dict[key] = weight

json_weights= {
    "block1_conv_1_weights": vgg_block_weight_dict["vggblock10_conv0_weight"].flatten().tolist(),
    "block1_conv_1_biases": vgg_block_weight_dict["vggblock10_conv0_bias"].flatten().tolist(),
    "block1_conv_2_weights": vgg_block_weight_dict["vggblock10_conv1_weight"].flatten().tolist(),
    "block1_conv_2_biases": vgg_block_weight_dict["vggblock10_conv1_bias"].flatten().tolist()
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

with open('data/block1/test_1/weights.json', 'w') as fp:
    json.dump(json_weights, fp)

with open('data/block1/test_1/outputs.json', 'w') as fp:
    json.dump(json_outputs, fp)

with open('data/block1/test_1/inputs.json', 'w') as fp:
    json.dump(json_inputs, fp)