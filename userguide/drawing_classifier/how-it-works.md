# How it works

At its heart, the Drawing Classifier toolkit is a very simple 
Convolutional Neural Network that takes in a 28x28 grayscale bitmap as input.
This network consists of three Convolutions with ReLU 
activations followed by Max Poolings, with two fully connected layers in the 
end. 

When a grayscale bitmap is provided as the feature in the SFrame to the toolkit
during training or during inference, the toolkit will take care of resizing 
bitmaps from the size they were to 28x28.

When stroke-based drawing data is provided as the feature in the SFrame 
to the toolkit during training or during inference, the stroke-based 
drawing data must adhere to the following format:

Each drawing must be represented as a list of strokes, where each stroke must 
be represented as a list of points in the order that they were drawn. 
Each point must be represented as a dictionary with exactly two keys, 
"x" and "y", the values of which must be numerical, i.e. integer or float.
Here is an example of a drawing with two strokes that have five points each:

```python
example_drawing = [
    [
        {"x": 1.0, "y": 2.0},
        {"x": 2.0, "y": 2.0},
        {"x": 3.0, "y": 2.0},
        {"x": 4.0, "y": 2.0},
        {"x": 5.0, "y": 2.0}
    ], # end of first stroke
    [
        {"x": 10.0, "y": 10.0},
        {"x": 10.5, "y": 10.5},
        {"x": 11.0, "y": 11.0},
        {"x": 12.5, "y": 12.5},
        {"x": 15.0, "y": 15.0}
    ]
]
```

When stroke-based drawing data is given as input to 
`turicreate.drawing_classifier.create`, the toolkit performs preprocessing to 
convert the stroke-based drawings to bitmaps that the Neural Network behind 
the scenes can consume. This data transformation can be visualized through 
the `turicreate.drawing_classifier.util.draw_strokes(...)` Python API as well. 

The data preprocessing to convert stroke-based drawings into 28x28 bitmaps 
is three-fold:

1. **Align and Normalize Points**: We normalize all the strokes by 
scaling or stretching them so that all the coordinates lie in [0, 255].

2. **Simplify Normalized Strokes**: In this step, we reduce the number of 
points needed to represent each stroke in an effort to reduce the amount of 
data we have to deal with during the rasterization step. In most 
drawing/rendering ecosystems, points are usually sampled at a frequency high 
enough that some key points can capture the drawing that all the points 
rendered. To simplify drawings and get rid of redundant points, we employ the
Ramer-Douglas-Peucker algorithm, which decimates a stroke composed of 
line segments to a similar curve with fewer points.

3. **Rasterization**: Rasterization from the simplified normalized strokes to 
the final bitmap that the Neural Network can consume, can be further broken 
down into three steps: 
    1. First, we render the simplified drawing that we 
    got as an output from Step 2 as an intermediate bitmap of size 256x256 
    (since all our normalized points lie in [0, 255]). 
    2. Next, we blur the intermediate bitmap.
    3. Finally, we resize the blurred intermediate bitmap down to a final bitmap
    size of 28x28, which the Neural Network can consume.

As an example, here is what the processed bitmap looks like for the stroke data
in the example provided above (run using 
`turicreate.drawing_classifier.util.draw_strokes(example_drawing).show()`):

![Example Rendered Stroke Data](images/rendered_toy_strokes.png)

### Warm Start

To boost the accuracy of the models Turi Create users train, and to help those 
models converge faster, we provide the option of loading in a pretrained model
for a warm start. We have published this pre-trained model 
[here](https://docs-assets.developer.apple.com/turicreate/). 
We trained this published model on 1,000 examples each of the first 245 of the
345 classes in the Quick, Draw! dataset.