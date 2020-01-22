from _tf_model_architecture import ODTensorFlowModel


input_h = 416
input_w = 416
batch_size = 32
output_size = 165
out_h =  13
out_w = 13

model = ODTensorFlowModel(input_h, input_w, batch_size, output_size, out_h, out_w, [], {})