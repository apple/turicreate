from ast import literal_eval as _make_tuple
import json as _json
import mxnet as _mx
import numpy as _np
import turicreate as _turicreate
from .._mps_utils import mxnet_to_mps as _mxnet_to_mps

def create_mps_graph(sym_model, weight_dict, input_shape, outputs):
	net = sym_model.symbol
	net_shapes = net.infer_shape(**dict(input_shape))
	net_shape_dict = dict(zip(net.list_arguments(), net_shapes[0]))
	net_json = _json.loads(net.tojson())
	net_nodes = net_json["nodes"]

	layers = []
	layer_dict = {}

	mps_graph = _turicreate.extensions.create_graph()

	# Set Input Node
	input_node_name = str(net_nodes[0]["name"])
	layer_dict[input_node_name] = _turicreate.extensions._InputNode()
	layer_dict[input_node_name].init(input_node_name)
	mps_graph.add_node(layer_dict[input_node_name])

	for idx, n in enumerate(net_nodes):
		if(n['op'] == "Convolution"):
			bias = _np.array([]).tolist()
			bias_key = None

			name = n['name']
			
			kernelWidth = _make_tuple(n['attrs']['kernel'])[0]
			kernelHeight = _make_tuple(n['attrs']['kernel'])[1]
			
			paddingWidth = _make_tuple(n['attrs']['pad'])[0]
			paddingHeight = _make_tuple(n['attrs']['pad'])[1]
			
			strideWidth = _make_tuple(n['attrs']['stride'])[0]
			strideHeight = _make_tuple(n['attrs']['stride'])[1]
			
			weight_key = net_nodes[n['inputs'][1][0]]['name']
			weights = weight_dict[weight_key]
			weights = _mxnet_to_mps(weights.asnumpy()).flatten().tolist()

			outputFeatureChannels = n['attrs']['num_filter']
				
			inputFeatureChannels =  net_shape_dict[weight_key][1]
			
			if(n['attrs']['no_bias'] == 'False'):
				bias = weight_dict[net_nodes[n['inputs'][2][0]]['name']]
				bias = _mxnet_to_mps(bias.asnumpy()).flatten().tolist()
			

			options = {
				"kernel_width": int(kernelWidth),
				"kernel_height": int(kernelHeight),
				"input_feature_channels": int(inputFeatureChannels),
				"output_feature_channels": int(outputFeatureChannels),
				"stride_width": int(strideWidth),
				"stride_height": int(strideHeight),
				"padding_width": int(paddingWidth),
				"padding_height": int(paddingHeight)
			}

			# TODO: convert into mps format
			data = {
				"weights": weights,
				"biases": bias
			}

			# TODO: get input layer
			input_to_layer = layer_dict[net_nodes[n['inputs'][0][0]]['name']]
			layer = _turicreate.extensions._ConvolutionNode()
			layer.init(name, input_to_layer, options, data)
			layers.append(layer)
			layer_dict[str(name)] = layer
			mps_graph.add_node(layer)

		elif(n['op'] == 'InstanceNorm'):
			
			styles = None
			channels = None
			gamma_layer_name = None
			beta_layer_name = None
			gamma = None
			beta = None

			# inputs
			input_instance_norm = n['inputs'][0]

			# gamma
			slice_gamma = net_nodes[n['inputs'][1][0]]
			gamma_layer_name = net_nodes[slice_gamma['inputs'][1][0]]['name']

			# beta weights
			slice_beta = net_nodes[n['inputs'][2][0]]
			beta_layer_name = net_nodes[slice_beta['inputs'][1][0]]['name']

			# get channels and styles
			channels = slice_gamma['attrs']['output_dim']
			styles = slice_gamma['attrs']['input_dim']

			gamma = weight_dict[gamma_layer_name]
			beta = weight_dict[beta_layer_name]
			name = n['name']

			data = {
				"gamma": gamma.asnumpy().tolist(),
				"beta": beta.asnumpy().tolist()
			}

			input_to_layer = layer_dict[net_nodes[n['inputs'][0][0]]['name']]
			layer = _turicreate.extensions._InstanceNormNode()
			layer.init(name, input_to_layer, int(channels), int(styles), data)
			layers.append(layer)
			layer_dict[str(name)] = layer
			mps_graph.add_node(layer)

		elif(n['op'] == 'take'):
			pass # HACK: right now the InstanceNorm defined isn't a pure instance norm, it also contains take

		elif(n['op'] == 'SliceChannel'):
			pass # HACK: right now the InstanceNorm defined isn't a pure instance norm, it also contains SliceChannel

		elif(n['op'] == 'Embedding'):
			pass # HACK: right now the InstanceNorm defined isn't a pure instance norm, it also contains Embedding

		elif(n['op'] == 'Activation'):
			inputs = net_nodes[n["inputs"][0][0]]["name"]
			activation = n["attrs"]["act_type"]
			name = n["name"]

			if(activation == "sigmoid"):
				input_to_layer = layer_dict[net_nodes[n['inputs'][0][0]]['name']]
				layer = _turicreate.extensions._SigmoidNode()
				layer.init(name, input_to_layer)
				layers.append(layer)
				layer_dict[str(name)] = layer
				mps_graph.add_node(layer)

			elif(activation == "relu"):
				input_to_layer = layer_dict[net_nodes[n['inputs'][0][0]]['name']]
				layer = _turicreate.extensions._ReluNode()
				layer.init(name, input_to_layer)
				layers.append(layer)
				layer_dict[str(name)] = layer
				mps_graph.add_node(layer)

		elif(n['op'] == 'UpSampling'):
			scale_x = n['attrs']['scale']
			scale_y = n['attrs']['scale']
			name = n['name']

			input_to_layer = layer_dict[net_nodes[n['inputs'][0][0]]['name']]
			layer = _turicreate.extensions._UpsamplingNode()
			layer.init(name, input_to_layer, int(scale_x), int(scale_y), 3)
			layers.append(layer)
			layer_dict[str(name)] = layer
			mps_graph.add_node(layer)

		elif(n['op'] == 'elemwise_add'):
			input_1 = net_nodes[n['inputs'][0][0]]["name"]
			input_2 = net_nodes[n['inputs'][1][0]]["name"]
			name = n["name"]

			left_input_to_layer = layer_dict[input_1]
			right_input_to_layer = layer_dict[input_2]

			layer = _turicreate.extensions._AdditionNode()
			layer.init(name, left_input_to_layer, right_input_to_layer)
			layers.append(layer)
			layer_dict[str(name)] = layer
			mps_graph.add_node(layer)

		elif(n['op'] != 'null' ):
			raise ValueError('We do not currently support the layer operation %s' % (n['op']))


	layer = _turicreate.extensions._OutputNode()
	out_arr = []
	for out in outputs:	
		out_arr.append(layer_dict[out])
	layer.init("output", out_arr)
	layers.append(layer)
	layer_dict["output"] = layer
	mps_graph.add_node(layer)

	mps_graph.compile(layers)