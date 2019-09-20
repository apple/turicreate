import mxnet as mx
from mxnet.gluon import HybridBlock
import numpy as np
from mxnet.gluon import nn
import json

mps_input = np.random.random_sample((1, 16, 16, 128))
mx_input = mps_input.transpose(0, 3, 1, 2)

class VGGBlock2(HybridBlock):
    """
    VGGBlock2 network
    """

    def __init__(self):
        super(VGGBlock2, self).__init__()

        with self.name_scope():
            self.conv1_1 = nn.Conv2D(in_channels=128, channels=256, kernel_size=3, padding=1)
            self.conv1_2 = nn.Conv2D(in_channels=256, channels=256, kernel_size=3, padding=1)
            self.conv1_3 = nn.Conv2D(in_channels=256, channels=256, kernel_size=3, padding=1)
    
    def hybrid_forward(self, F, X):
        h = F.Activation(self.conv1_1(X), act_type='relu')
        h = F.Activation(self.conv1_2(h), act_type='relu')
        h = F.Activation(self.conv1_3(h), act_type='relu')
        
        h = F.Pooling(h, pool_type='avg', kernel=(2, 2), stride=(2, 2))
        
        return h

net = VGGBlock2()
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

json_config = {
  "conv_1": {
    "kernel_width": 3,
    "kernel_height": 3,
    "input_feature_channels": 128,
    "output_feature_channels": 256,
    "stride_width": 1,
    "stride_height": 1,
    "padding_width": 1,
    "padding_height": 1,
    "update_weights": True
  },
  "conv_2": {
    "kernel_width": 3,
    "kernel_height": 3,
    "input_feature_channels": 256,
    "output_feature_channels": 256,
    "stride_width": 1,
    "stride_height": 1,
    "padding_width": 1,
    "padding_height": 1,
    "update_weights": True
  },
  "conv_3": {
    "kernel_width": 3,
    "kernel_height": 3,
    "input_feature_channels": 256,
    "output_feature_channels": 256,
    "stride_width": 1,
    "stride_height": 1,
    "padding_width": 1,
    "padding_height": 1,
    "update_weights": True
  },
  "pooling": {
    "kernel": 2,
    "stride": 2
  }
}

json_weights = {
    "block2_conv_1_weights": vgg_block_weight_dict["vggblock20_conv0_weight"].flatten().tolist(),
    "block2_conv_1_biases": vgg_block_weight_dict["vggblock20_conv0_bias"].flatten().tolist(),
    "block2_conv_2_weights": vgg_block_weight_dict["vggblock20_conv1_weight"].flatten().tolist(),
    "block2_conv_2_biases": vgg_block_weight_dict["vggblock20_conv1_bias"].flatten().tolist(),
    "block2_conv_3_weights": vgg_block_weight_dict["vggblock20_conv2_weight"].flatten().tolist(),
    "block2_conv_3_biases": vgg_block_weight_dict["vggblock20_conv2_bias"].flatten().tolist()
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

with open('data/block2/test_1/config.json', 'w') as fp:
    json.dump(json_config, fp)

with open('data/block2/test_1/weights.json', 'w') as fp:
    json.dump(json_weights, fp)

with open('data/block2/test_1/outputs.json', 'w') as fp:
    json.dump(json_outputs, fp)

with open('data/block2/test_1/inputs.json', 'w') as fp:
    json.dump(json_inputs, fp)