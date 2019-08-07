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
consists of 3 convolutional layers, 5 residual layers (2 convolutional
layers in each) and 3 upsampling layers each followed by a convolutional
layer.  There are a total of 16 convolutional layers.

There are three aspects about this technique that are worth noting:
- It is designed to be incredibly fast at stylizing images, allowing deployment
  on device. As a trade off, the model creation takes longer.
- A single model can incorporate a large number of styles without any
  significant increase in the size of the model.
- The model can take input of any size and output a stylized image of
  the same size.

During training, we employ [Transfer
Learning](../image_classifier/how-it-works.md#transfer-learning). The 
model uses the visual semantics of an already trained VGG-16 network to
understand and mimic stylistic elements. If the `finetune_all_params` parameter is set to `False` only a small set of parameters of the stylization network are updated - the rest of the parameters were
already trained and remain unchanged. This allows you to achieve great results
with little data and shorter training time. If  `finetune_all_params` is set to `True`, the entire model is updated in training. For more information on advanced options in training, refer to the [Advanced Usage](../advanced-usage.md) section.
