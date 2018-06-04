# How it works

The style transfer system in Turi Create uses Convolutional Neural
Networks (CNNs) to create high quality artistic images. Broadly
speaking, we use CNNs to separate and recombine the content and style
elements of arbitrary images.

#### Style transfer model

The technique used in Turi Create is based on [*"A Learned
Representation For Artistic
Style"*](https://arxiv.org/pdf/1610.07629.pdf). The model is compact and
fast and hence can run on mobile devices like an iPhone. The model
consists of 3 convolutional layers, 5 residual layers (2 convolutonal
layers in each) and 3 upsampling layers each followed by a convolutional
layer.  There are totally 16 convolutional layers.

There are three aspects about this techinque that are worth noting:
- It trades off training time (which can be larger) to make sure the
  inference time is fast enough for use on a mobile device.
- A single model can incorporate a large number of styles without any
  significant increase in the size of the model.
- During inference, the model can take in any-sized input image and out
  the stlyized image of the same size.

During training, we employ [Transfer
Learning](../image_classifier/how-it-works.md#transfer-learning). The 
model uses  VGG-16 as a reference network to compute the losses. We 
fine-tune only the instance norm parameters of the styles and not the 
entire network.

#### Advanced parameters

We provide a few advanced training parameters that might be useful to
you as you develop more intuition for the model. You may specifiy these
parameters by specifying _advanced_parameters in
[style_transfer.create()](https://apple.github.io/turicreate/docs/api/generated/turicreate.style_transfer.create.html#turicreate.style_transfer.create).
Note that this may not result in a better model than the default
parameters (which are chosen automatically based on your data):

* `finetune_all_params`: Setting this to True will allow the fine-tune
  all the parameters of resnet-16.
* `print_loss_breakdown`: Setting this parameter to True will print the
  content loss and style loss separately.Higher the style loss, lesser
style elements will be captured in the content images.
