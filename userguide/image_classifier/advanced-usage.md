# Advanced Usage

In this section, we will cover some more advanced options in the image
classifier toolkit that let you do more.


#### Annotating Data

If you only have images without corresponding labels, you can 
use the annotation utility built into the image_classifier. An example
of its usage is shown below:

```python
import turicreate as tc

# Use the example provided in the `Introductory Example`

# Use the Annotation GUI to annotate your data.
annotated_data = tc.image_classifier.annotate(data)

```

If you forget to assign the output of your annotation to a variable, 
we've included a method to help you recover those annotations. The code
for that is shown below:

```python

import turicreate as tc

# Use the example provided in the `Introductory Example`

# If you forget to assign the output to a variable
tc.image_classifier.annotate(data)

# recover your annotation with this method
annotated_data = tc.image_classifier.recover_annotation()
```

The annotation utility supports label types of `str` and `int`.


##### Changing Models

The image classifier toolkit is based on a technique known as transfer
learning. At a high level, model creation is
accomplished by simply removing the output layer of the Deep Neural
Network for 1000 categories, and taking the signals that would have been
propagating to the output layer and feeding them as features to any
classifier for our task.

The advanced options let you select from a set of pre-trained models
which can result in a model having various size, performance, and
accuracy characteristics.

Using the following option, you can change to use *squeezenet* which can
trade off some accuracy for a smaller model with a lower memory and disk
foot-print.

```python
model = tc.image_classifier.create(
               train_data, target='label', model='squeezenet_v1.1')
```

##### Using GPUs

GPUs can make creating an image classifier model much faster. If you have
macOS 10.13 or higher, Turi Create will automatically use the GPU. If
your Linux machine has an NVIDIA GPU, you can setup Turi Create to use
the GPU, [see instructions](https://github.com/apple/turicreate/blob/master/LinuxGPU.md).

The `turicreate.config.set_num_gpus` function allows you to control if GPUs are used:
```python
# Use all GPUs (default)
turicreate.config.set_num_gpus(-1)

# Use only 1 GPU
turicreate.config.set_num_gpus(1)

# Use CPU
turicreate.config.set_num_gpus(0)
```

