import mxnet as mx
from mxnet.gluon import HybridBlock
import numpy as np
from mxnet.gluon import nn
import json

mps_input = np.random.random_sample((1, 32, 32, 3))
mx_input = mps_input.transpose(0, 3, 1, 2)

class Vgg16(HybridBlock):
    def __init__(self):
        super(Vgg16, self).__init__(prefix='vgg16_')

        with self.name_scope():
            self.conv1_1 = nn.Conv2D(in_channels=3, channels=64, kernel_size=3, padding=1)
            self.conv1_2 = nn.Conv2D(in_channels=64, channels=64, kernel_size=3, padding=1)

            self.conv2_1 = nn.Conv2D(in_channels=64, channels=128, kernel_size=3, padding=1)
            self.conv2_2 = nn.Conv2D(in_channels=128, channels=128, kernel_size=3, padding=1)

            self.conv3_1 = nn.Conv2D(in_channels=128, channels=256, kernel_size=3, padding=1)
            self.conv3_2 = nn.Conv2D(in_channels=256, channels=256, kernel_size=3, padding=1)
            self.conv3_3 = nn.Conv2D(in_channels=256, channels=256, kernel_size=3, padding=1)

            self.conv4_1 = nn.Conv2D(in_channels=256, channels=512, kernel_size=3, padding=1)
            self.conv4_2 = nn.Conv2D(in_channels=512, channels=512, kernel_size=3, padding=1)
            self.conv4_3 = nn.Conv2D(in_channels=512, channels=512, kernel_size=3, padding=1)

    def hybrid_forward(self, F, X):
        h = F.Activation(self.conv1_1(X), act_type='relu')
        h = F.Activation(self.conv1_2(h), act_type='relu')
        relu1_2 = h
        h = F.Pooling(h, pool_type='avg', kernel=(2, 2), stride=(2, 2))

        h = F.Activation(self.conv2_1(h), act_type='relu')
        h = F.Activation(self.conv2_2(h), act_type='relu')
        relu2_2 = h
        h = F.Pooling(h, pool_type='avg', kernel=(2, 2), stride=(2, 2))

        h = F.Activation(self.conv3_1(h), act_type='relu')
        h = F.Activation(self.conv3_2(h), act_type='relu')
        h = F.Activation(self.conv3_3(h), act_type='relu')
        relu3_3 = h
        h = F.Pooling(h, pool_type='avg', kernel=(2, 2), stride=(2, 2))

        h = F.Activation(self.conv4_1(h), act_type='relu')
        h = F.Activation(self.conv4_2(h), act_type='relu')
        h = F.Activation(self.conv4_3(h), act_type='relu')
        relu4_3 = h

        return [relu1_2, relu2_2, relu3_3, relu4_3]

net = Vgg16()
net.collect_params().initialize()
output = net(mx.nd.array(mx_input))

vgg_weight_dict = dict()

param_keys = list(net.collect_params())
for key in param_keys:
    weight = net.collect_params()[key].data().asnumpy()
    if len(weight.shape) == 4:
        vgg_weight_dict[key] = weight.transpose(0, 2, 3, 1)
    else:
        vgg_weight_dict[key] = weight

output_1 = output[0]
output_2 = output[1]
output_3 = output[2]
output_4 = output[3]

json_config = {
    "block1": {
        "conv_1":{
            "kernel_width":9,
            "kernel_height":9,
            "input_feature_channels":32,
            "output_feature_channels":3,
            "stride_width":1,
            "stride_height":1,
            "padding_width":4,
            "padding_height":4,
            "update_weights":False
        },
        "conv_2":{
            "kernel_width":3,
            "kernel_height":3,
            "input_feature_channels":64,
            "output_feature_channels":64,
            "stride_width":1,
            "stride_height":1,
            "padding_width":1,
            "padding_height":1,
            "update_weights":False
        },
        "pooling":{
            "kernel": 2,
        "stride": 2
        }
    },
    "block2": {
        "conv_1":{
            "kernel_width":3,
            "kernel_height":3,
            "input_feature_channels":64,
            "output_feature_channels":128,
            "stride_width":1,
            "stride_height":1,
            "padding_width":1,
            "padding_height":1,
            "update_weights":False
        },
        "conv_2":{
            "kernel_width":3,
            "kernel_height":3,
            "input_feature_channels":128,
            "output_feature_channels":128,
            "stride_width":1,
            "stride_height":1,
            "padding_width":1,
            "padding_height":1,
            "update_weights":False
        },
        "pooling":{
            "kernel": 2,
        "stride": 2
        }
    },
    "block3": {
        "conv_1":{
            "kernel_width":3,
            "kernel_height":3,
            "input_feature_channels":128,
            "output_feature_channels":256,
            "stride_width":1,
            "stride_height":1,
            "padding_width":1,
            "padding_height":1,
            "update_weights":False
        },
        "conv_2":{
            "kernel_width":3,
            "kernel_height":3,
            "input_feature_channels":256,
            "output_feature_channels":256,
            "stride_width":1,
            "stride_height":1,
            "padding_width":1,
            "padding_height":1,
            "update_weights":False
        },
        "conv_3":{
            "kernel_width":3,
            "kernel_height":3,
            "input_feature_channels":256,
            "output_feature_channels":256,
            "stride_width":1,
            "stride_height":1,
            "padding_width":1,
            "padding_height":1,
            "update_weights":False
        },
        "pooling":{
            "kernel": 2,
        "stride": 2
        }
    },
    "block4": {
        "conv_1":{
            "kernel_width":3,
            "kernel_height":3,
            "input_feature_channels":256,
            "output_feature_channels":512,
            "stride_width":1,
            "stride_height":1,
            "padding_width":1,
            "padding_height":1,
            "update_weights":False
        },
        "conv_2":{
            "kernel_width":3,
            "kernel_height":3,
            "input_feature_channels":512,
            "output_feature_channels":512,
            "stride_width":1,
            "stride_height":1,
            "padding_width":1,
            "padding_height":1,
            "update_weights":False
        },
        "conv_3":{
            "kernel_width":3,
            "kernel_height":3,
            "input_feature_channels":512,
            "output_feature_channels":512,
            "stride_width":1,
            "stride_height":1,
            "padding_width":1,
            "padding_height":1,
            "update_weights":False
        },
        "pooling":{
            "kernel": 2,
        "stride": 2
        }
    }
}

json_weights = {
    "vgg_block_1_conv_1_weights": vgg_weight_dict["vgg16_conv0_weight"].flatten().tolist(),
    "vgg_block_1_conv_1_biases": vgg_weight_dict["vgg16_conv0_bias"].flatten().tolist(),
    "vgg_block_1_conv_2_weights": vgg_weight_dict["vgg16_conv1_weight"].flatten().tolist(),
    "vgg_block_1_conv_2_biases": vgg_weight_dict["vgg16_conv1_bias"].flatten().tolist(),
    "vgg_block_2_conv_1_weights": vgg_weight_dict["vgg16_conv2_weight"].flatten().tolist(),
    "vgg_block_2_conv_1_biases": vgg_weight_dict["vgg16_conv2_bias"].flatten().tolist(),
    "vgg_block_2_conv_2_weights": vgg_weight_dict["vgg16_conv3_weight"].flatten().tolist(),
    "vgg_block_2_conv_2_biases": vgg_weight_dict["vgg16_conv3_bias"].flatten().tolist(),
    "vgg_block_3_conv_1_weights": vgg_weight_dict["vgg16_conv4_weight"].flatten().tolist(),
    "vgg_block_3_conv_1_biases": vgg_weight_dict["vgg16_conv4_bias"].flatten().tolist(),
    "vgg_block_3_conv_2_weights": vgg_weight_dict["vgg16_conv5_weight"].flatten().tolist(),
    "vgg_block_3_conv_2_biases": vgg_weight_dict["vgg16_conv5_bias"].flatten().tolist(),
    "vgg_block_3_conv_3_weights": vgg_weight_dict["vgg16_conv6_weight"].flatten().tolist(),
    "vgg_block_3_conv_3_biases": vgg_weight_dict["vgg16_conv6_bias"].flatten().tolist(),
    "vgg_block_4_conv_1_weights": vgg_weight_dict["vgg16_conv7_weight"].flatten().tolist(),
    "vgg_block_4_conv_1_biases": vgg_weight_dict["vgg16_conv7_bias"].flatten().tolist(),
    "vgg_block_4_conv_2_weights": vgg_weight_dict["vgg16_conv8_weight"].flatten().tolist(),
    "vgg_block_4_conv_2_biases": vgg_weight_dict["vgg16_conv8_bias"].flatten().tolist(),
    "vgg_block_4_conv_3_weights": vgg_weight_dict["vgg16_conv9_weight"].flatten().tolist(),
    "vgg_block_4_conv_3_biases": vgg_weight_dict["vgg16_conv9_bias"].flatten().tolist()
}

json_inputs = {
    "content": mps_input.flatten().tolist(),
    "height": mps_input.shape[1],
    "width": mps_input.shape[2],
    "channels": mps_input.shape[3]
}

json_outputs = {
    "output_1": output_1.asnumpy().transpose(0, 2, 3, 1).flatten().tolist(),
    "output_2": output_2.asnumpy().transpose(0, 2, 3, 1).flatten().tolist(),
    "output_3": output_3.asnumpy().transpose(0, 2, 3, 1).flatten().tolist(),
    "output_4": output_4.asnumpy().transpose(0, 2, 3, 1).flatten().tolist()
}

with open('data/vgg16/test_1/config.json', 'w') as fp:
    json.dump(json_config, fp)

with open('data/vgg16/test_1/weights.json', 'w') as fp:
    json.dump(json_weights, fp)

with open('data/vgg16/test_1/outputs.json', 'w') as fp:
    json.dump(json_outputs, fp)

with open('data/vgg16/test_1/inputs.json', 'w') as fp:
    json.dump(json_inputs, fp)