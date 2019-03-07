# Quick, Draw! Data Preparation

In this section, we will show you how to download a publicly available dataset
and load it into an SFrame. This will allow you to try out the Drawing Classification
toolkit for yourself. To get this dataset into the format expected by our
toolkit, we will rely on many useful SFrame functions.

*Note: Please make sure that you have Turi Create 4.3 or above for these steps*

The dataset that we will use is 
[Quick, Draw!](https://quickdraw.withgoogle.com/data)[^1]. Our goal is to
make a drawing classifier for squares and triangles. 

First, we will download all the data points in the "Quick, Draw!" dataset for
the "square" and "triangle" classes -- around 120,000 examples each, 
which amounts to about 93 megabytes each.
Open a new terminal window and run the following commands:

```
$ mkdir -p ~/Downloads/quickdraw
$ cd ~/Downloads/quickdraw
$ curl https://storage.googleapis.com/quickdraw_dataset/full/numpy_bitmap/square.npy > square.npy
$ curl https://storage.googleapis.com/quickdraw_dataset/full/numpy_bitmap/triangle.npy > triangle.npy
```

Now, you should have the following file structure:

```
~/Downloads/quickdraw/
    square.npy
    triangle.npy
```

Next, we will sample 50 examples from each of the classes and turn it into an
SFrame.
Here is a snippet to accomplish that:

```python
import turicreate as tc
import numpy as np
import os

random_state = np.random.RandomState(100)

# Change if applicable
quickdraw_dir = '~/Downloads/quickdraw'
npy_ext = '.npy'
num_examples_per_class = 50
classes = ["square", "triangle"]

num_classes = len(classes)
np_data = [None] * (num_examples_per_class * num_classes)
np_labels = [""] * (num_examples_per_class * num_classes)
index = 0
bitmaps_list, labels_list = [], []
for class_name in classes:
    class_data = np.load(os.path.join(quickdraw_dir, class_name + npy_ext))
    random_state.shuffle(class_data)
    class_data_selected = class_data[:num_examples_per_class]
    class_data_selected = class_data_selected.reshape(
        class_data_selected.shape[0], 28, 28, 1)
    for np_pixel_data in class_data_selected:
        FORMAT_RAW = 2
        drawing = tc.Image(_image_data = np_pixel_data.tobytes(),
                           _width = np_pixel_data.shape[1],
                           _height = np_pixel_data.shape[0],
                           _channels = np_pixel_data.shape[2],
                           _format_enum = FORMAT_RAW,
                           _image_data_size = np_pixel_data.size)
        bitmaps_list.append(bitmap)
        labels_list.append(class_name)
sf = tc.SFrame({"bitmap": bitmaps_list, "label": labels_list})
sf.save(os.path.join(quickdraw_dir, "square_triangle.sframe"))
```

## References

The [Quick, Draw!](https://quickdraw.withgoogle.com/data) dataset is described
further in:

[^1]: [Exploring and Visualizing an Open Global Dataset](https://ai.googleblog.com/2017/08/exploring-and-visualizing-open-global.html)
