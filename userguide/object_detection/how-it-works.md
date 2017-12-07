# How it works

For a long time, object detection models used to separate mechanisms to
perform localization (where) and classification (what). These models
are called *two-stage* detectors and still yield competitive results. However,
recent work has combined these two steps into a single deep learning model,
making them *one-stage* detectors:

* [*"You Only Look Once: Unified, Real-Time Object Detection"*](https://arxiv.org/abs/1506.02640) by Joseph Redmon, Santosh Divvala, Ross Girshick, Ali Farhadi (CVPR 2016) 
* [*"SSD: Single Shot MultiBox Detector"*](https://arxiv.org/abs/1512.02325) by Wei Liu, Dragomir Anguelov, Dumitru Erhan, Christian Szegedy, Scott Reed, Cheng-Yang Fu, Alexander C. Berg (ECCV 2016)
* [*"YOLO9000: Better, Faster, Stronger"*](https://arxiv.org/abs/1612.08242) by Joseph Redmon, Ali Farhadi (CVPR 2017)

These models are incredibly fast and can run at impressive frame rates even on
mobile devices like an iPhone. The overall model is similar to an image
classifier (see [How it works](../image_classifier/how-it-works.md)). The main
difference is that the network is instructed to predict the presence of multiple objects
per image, each object instance associated with a bounding box localization. In fact, our model predicts a fixed set of 2535 instances. The exact
number comes from 13x13x15, where 13-by-13 describes a fixed grid of center
locations. The last number represents a pre-defined list of 15 canonical
bounding box shapes (e.g. 32-by-32 and 256-by-128). Since most images have only
a handful of object instances, the vast majority of the 2535 instances are eliminated
either by having low confidence or by the non-maximum suppression algorithm
(see [Advanced Usage](advanced-usage.md)). The list of locations and shapes is
meant to provide *anchor* boxes that should roughly cover the prediction needs
of any image. In other words, given an image of an object, at least one of the
2535 anchor boxes should be reasonably close to the correct bounding box for
that object. However, a perfect match will be rare. To address this,
adjustment values for both location and shape are also predicted and used to
tweak the fixed anchor boxes to yield more precise localization.

#### Transfer Learning
During training, similar to the image classifier, we employ [Transfer
Learning](../image_classifier/how-it-works.md#transfer-learning). In fact,
our starting point is still an image classifier trained on 1000 classes. This
means that the network has seen millions of images before it even looked at any
of our data. This is great, because it reduces the data annotation burden on
us and is exactly what allows us to create reasonable detectors sometimes with
only 30 samples per class in our training data. However, since the starting
network was not primed for detection, we do need to adapt it for
the new task. This requires what is called *end-to-end fine-tuning*, which is
the process of gently updating all the weights (parameters) for the new task,
without forgetting all the useful visual semantics it previously learned.
Contrast this with image classification, where it was enough to adjust the top
layer. As a result, model creation time is longer for the object detector
than what it is for the image classifier.

#### YOLO
The model that we use is a re-implementation of TinyYOLO (YOLOv2 with a Darknet
base network).

* Source Link: https://github.com/pjreddie/darknet
* Project Page: https://pjreddie.com/darknet/yolo/
* Citation: [*"YOLO9000: Better, Faster, Stronger"*](https://arxiv.org/abs/1612.08242) by Joseph Redmon, Ali Farhadi (CVPR 2017)
* License: Multiple (see https://github.com/pjreddie/darknet)
