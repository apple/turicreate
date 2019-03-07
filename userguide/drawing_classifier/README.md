# Drawing Classification

The Drawing Classifier is a toolkit focused on solving the task of classifying 
input from the Apple Pencil and/or mouse/touch input. This is the first effort 
towards bridging the gap between the Apple Pencil 2 and Core ML, which will 
further empower App Developers to build intelligent apps. 
Given a drawing, the drawing classifier aims to classify the drawing 
as one of a pre-determined number of classes/labels. 

#### Loading Data

The [Quick, Draw! dataset](https://quickdraw.withgoogle.com/data) is a 
crowd-sourced dataset that provides around 50 million labeled drawings for 
345 classes.[<sup>1</sup>](../datasets.md)
In this example, we use data for two of the 345 classes from "Quick,Draw!" -- 
square and triangle. Go to [Data Preparation](data-preparation.md) to create the 
`square_triangle.sframe` that we will use in the introductory example.


#### Introductory Example

In this example, the goal is to 
**predict if the drawing is a square or a triangle**. 
Go to [Data Preparation](data-preparation.md) to find out how to get 
`bitmap_square_triangle.sframe` or `stroke_square_triangle.sframe`).

```python
import turicreate as tc

# Try any one of the following
SFRAME_PATH = "sframes/bitmap_square_triangle.sframe"
SFRAME_PATH = "sframes/stroke_square_triangle.sframe"

# Load the data
data =  tc.SFrame(SFRAME_PATH)

# Make a small train-test split since our toolkit is not very data-hungry 
# for 2 classes
train_data, test_data = data.random_split(0.4)

# Create a model
model = tc.drawing_classifier.create(train_data)

# Save predictions to an SArray
predictions = model.predict(test_data)

# Evaluate the model and save the results into a dictionary
metrics = model.evaluate(test_data)
print(metrics["accuracy"])

# Save the model for later use in Turi Create
model.save("square_triangle.model")

# Export for use in Core ML
model.export_coreml("MySquareTriangleClassifier.mlmodel")
```

#### Advanced Usage

Refer to the following chapters for:
* Diving into the [Technical details](how-it-works.md) of how the drawing classifier works.
* Understanding the [Datasets](data-preparation.md) and preparing them for training.
* [Exporting to Core ML](export-coreml.md) and deploying the model on device.
* [Advanced](advanced-usage.md) usage options including using pre-trained models.
