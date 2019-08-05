# Advanced Usage

In this section, we will cover some more advanced options in the style transfer
toolkit that let you do more.

#### style_loss_mult

The larger the values of the `style_loss_mult`, the more stylized the image 
is. The diagram below illustrates this effect (Note: the model was trained for 
`64,000` iterations.)

![Style Transfer Style Loss Multiplier](images/style_loss_mult.png)

An example of setting the `style_loss_mult` flag is show below:

```
model = tc.style_transfer.create(styles, content, _advanced_parameters={"style_loss_mult":[1e-4, 1e-4, 1e-4, 1e-4]})
```

#### finetune_all_params

There are two different approaches to updating parameters in the style transfer network.

- Only update the instance normalization layers (`finetune_all_params: False`)
- Update both the convolutional and instance normalization layers. (`finetune_all_params: True`)

An example of setting the `finetune_all_params` flag is show below:

```
model = tc.style_transfer.create(styles, content, _advanced_parameters={"finetune_all_params":False})
```

##### finetune_all_params = False

**Pros**

- Trains quicker because there are less parameters to train.
- When training a model with multiple styles, the network usually converges, though the quality may suffer as a side-effect.

**Cons**

- Pre-trained weights must be used or the model doesn’t converge.
- Really limited by the convolutonal weights, the model might not be able to train past artifacts due to the low number of updatable parameters.

##### finetune_all_params = True

**Pros**

- Can converge without pre-trained weights.
- Usually leads to a model with a higher stylization quality.
- Increasing the number of iterations usually decreases the atrifacts present in the stylization.

**Cons**

- Requires a larger number of iterations to converge.
- When training a model with multiple styles, the network may never converge.

#### pretrained_weights

This option allows for the loading of pre-trained weights from a model trained 
with `32` distinct style images. For some styles, pre-trained weights can act 
as a warm-start for the training; for other styles these weights can by a 
hinderance, potentially introducting artifacts when stylizing images. As 
convergence is heavily determined by the style images chosen, further user 
experimentation is required.


When `finetune_all_params` is `False`, `pretrained_weights` is required to be `True` else the model won't converge.

An example of setting the `pretrained_weights` flag is show below:

```
model = tc.style_transfer.create(styles, content, _advanced_parameters={"pretrained_weights":True})
```

See [how-it-works](advanced-usage.md) for the paper from which these weights were procured.

#### Checkpointing

To assist with the qualatative nature of training the style transfer network, a checkpointing feature is exposed in the advanced parameters section. Rather than create multiple training loops for training models with different iterations, simply set the `checkpoint` flag to true and a turicreate model is saved with the prefix specified by `checkpoint_prefix` parameter every nth iteration which is specified by the `checkpoint_increment` parameter.

An example of using the checkpointing feature is show below:

```
model = tc.style_transfer.create(styles, content, _advanced_parameters={"checkpoint":True, "checkpoint_prefix":"./awesome-model", "checkpoint_increment":100})
```