# One Shot Object Detection

One shot object detection is the task of simultaneously classifying (*what*) and
localizing (*where*) object instances in an image, but it's subtly different from regular [object detection](../object_detection/README.md). With the regular object detector, you must provide a sampling of real-world images with labeled bounding boxes around each category you want to predict. In contrast, One Shot object detection allows you to provide just a single clean, representative image for each category (with only a string or int label annotation), and the resulting model can predict both the class and location of instances of this category in the real world. Because only a single input image per category is provided at training time, this image must be representative of all examples in the real world; this works especially well for human-designed objects that appear the same no matter where they appear, like street signs or logos.

Given an image and a one-shot detector trained on category 'stop sign', the output prediction will look like:

!!! TODO insert images here !!!

#### Introductory Example

In this example, the goal is to **predict** if there are **stop signs** in a picture and where in the picture they are located. For this task, we supply just a single representative image of a stop sign, and the label "stop_sign".
