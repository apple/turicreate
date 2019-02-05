# How Does this Work?

**Deep learning** is a phrase being thrown around everywhere in the
world of machine learning. It seems to be helping make tremendous
breakthroughs, but what is it? It's a methodology for learning
high-level concepts about data, frequently through models that have
multiple layers of non-linear transformations.  Let's take a moment to
analyze that last sentence. **Learning high-level concepts about data**
means that deep learning models take data, for instance raw pixel values
of an image, and learns abstract ideas like 'is animal' or 'is cat'
about that data.  OK, easy enough, but what does having 'multiple layers
of non-linear transformations' mean.  Conceptually, all this means is
that you have a composition of simple non-linear functions, forming a
complex non-linear function, which can map things as complex as raw
pixel values to image category. This is what allows deep learning models
to attain such amazing results.

The most common class of methods within the computer vision domain are
Convolutional Neural Nets (CNNs). Typically, the challenge arises in how
you have to choose how many layers your network has. And how to
initialize the model parameter values (also known as weights).  There’s
a lot more, too.  Basically, a deep learning model is a machine with
many confusing knobs (called hyper-parameters, basically parameters that
are not learned by the algorithm) and dials that will not work if set
randomly. Yet, when good hyper-parameter settings come together, results
are very strong.

The key goal of the image classifier toolkit is to reduce the complexity
in creating a model that is suitable for a wide variety of datasets
(small or large) with images of various categories.

## Transfer Learning

It’s not uncommon for the task you want to solve to be related to
something that has already been solved. Take, for example, the task of
distinguishing cats from dogs. The famous ImageNet Challenge, for which
CNN’s are the state-of-the-art, asks the trained model to categorize
input into one of 1000 classes. Shouldn't features that distinguish
between categories like lions and wolves also be useful for
discriminating between cats and dogs?

The answer is a definitive yes. It is accomplished by simply removing
the output layer of the Deep Neural Network for 1000 categories, and
taking the signals that would have been propagating to the output layer
and feeding them as features to a classifier for our new cats vs dogs
task.

So, when you run the Turi Create image classifier, it breaks things down
into something like this:

* **Stage 1**: Create a CNN classifier on a large, general dataset. A
  good example is ImageNet, with 1000 categories and 1.2 million images.
The models are already trained by researchers and are available for us
to use.

* **Stage 2**: The outputs of each layer in the CNN can be viewed as a
  meaningful vector representation of each image. Extract these feature
vectors from the layer prior to the output layer on each image of your
task.

* **Stage 3**: Create a new classifier with those features as input for
  your own task.

At first glance, this seems even more complicated than just training the
deep learning model. However, Stage 1 is reusable for many different
problems, and once done, it doesn't have to be changed often.

In the end, this pipeline results in not needing to adjust
hyper-parameters, faster training, and better performance even in cases
where you don't have enough data to create a convention deep learning
model. What's more, this technique is effective even if your Stage 3
classification task is relatively unrelated to the task Stage 1 is
trained on. This idea was first explored by Donahue et al. (2013), and
has since become one of the best ways to create image classifier models.

## Pretrained Image Classifiers

The following shows the built-in state-of-the-art network architectures
for image classification. We hope to be adding many more as the research
in the field evolves:


#### Resnet

Detects the dominant objects present in an image from a set of 1000
categories such as trees, animals, food, vehicles, people, and more.
The top-5 error from the original publication is 7.8%. The model is
roughly 102.6 MB in size.

* Source Link: <https://github.com/fchollet/deep-learning-models>
* Project Page: https://github.com/KaimingHe/deep-residual-networks>
* Paper: Kaiming He and Xiangyu Zhang and Shaoqing Ren and Jian Sun
* Keras Implementation: François Chollet
* Citations: Kaiming He, Xiangyu Zhang, Shaoqing Ren, Jian Sun. "Deep Residual Learning for Image Recognition." Paper <https://arxiv.org/abs/1512.03385>
* License: MIT License
* Core ML exported models are usually at least 90MB

#### Squeezenet

Detects the dominant objects present in an image from a set of 1000
categories such as trees, animals, food, vehicles, people, and more.
With an overall footprint of only 5 MB, SqueezeNet has a similar level
of accuracy as AlexNet but with 50 times fewer parameters.

* Source Link: https://github.com/DeepScale/SqueezeNet
* Project Page: https://github.com/DeepScale/SqueezeNet
* Citation: Forrest N. Iandola and Song Han and Matthew W. Moskewicz and
  Khalid Ashraf and William J. "SqueezeNet: AlexNet-level accuracy with
50x fewer parameters and <0.5MB model size."
<https://arxiv.org/abs/1602.07360>
* Caffe Implementation: http://deepscale.ai
* License: BSD License
* Core ML exported models are usually less than 5MB

#### VisionFeaturePrint_Scene

Only available on macOS 10.14 and higher. This model is included in the
operating system, so the exported model size is very small.

* Core ML exported models are about 40KB
