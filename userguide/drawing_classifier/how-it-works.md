# How it works

The Drawing Classifier toolkit is a CNN (convolutional neural network) that operates on a 28x28 grayscale bitmap as input. The network consists of three convolutions (with ReLU activations) followed by max-pooling, with two fully connected layers in the 
end.  

The toolkit operates on (1) grayscale bitmap of any size (resizing is handled automatically) or (2) stroke-based drawing data which is converted to bit-map using a series of built in feature transformations. 

The stroke-based drawing data must adhere to the following format: Each drawing is a list of strokes, where each stroke is a list of points in sequence in which they were drawn. Each point is a dictionary with two keys, "x" and "y" with numerical values representing the intensity of the stroke. Here is an example of a drawing with two strokes that have five points each:

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

When stroke-based drawing data is given as input to `turicreate.drawing_classifier.create`, a series of pre-processing steps are applied to convert the stroke-based drawings to bitmaps which is then consumed by the neural network. This data transformation can be visualized through the `turicreate.drawing_classifier.util.draw_strokes(...)` Python API as well. 

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

The function `turicreate.drawing_classifier.util.draw_strokes(example_drawing).show()` helps visualize the strokes.

### Warm Start

To boost the accuracy of the models Turi Create users train, and to help those 
models converge faster, we provide the option of loading in a pretrained model
for a warm start.
