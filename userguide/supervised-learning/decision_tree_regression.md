# Decision Tree Regression 
The decision tree is a simple machine learning model for getting started 
with regression tasks. 

##### Background
A decision tree is a flow-chart-like structure, where each internal (non-leaf)
node denotes a test on an attribute, each branch represents the outcome of a
test, and each leaf (or terminal) node holds a class label. The topmost node
in a tree is the root node. (see [here](https://en.wikipedia.org/wiki/Decision_tree_learning) 
for more details).

##### Introductory Example

In this example, we will use the [Mushrooms dataset](https://archive.ics.uci.edu/ml/datasets/mushroom).[<sup>1</sup>](../datasets.md)
```python
import turicreate as tc

# Load the data
data =  tc.SFrame.read_csv('https://raw.githubusercontent.com/apple/turicreate/master/src/python/turicreate/test/mushroom.csv')

# Label 'p' is edible
data['label'] = data['label'] == 'p'

# Make a train-test split
train_data, test_data = data.random_split(0.8)

# Create a model.
model = tc.decision_tree_regression.create(train_data, target='label',
                                           max_depth =  3)

# Save predictions to an SArray
predictions = model.predict(test_data)

# Evaluate the model and save the results into a dictionary
results = model.evaluate(test_data)
```

##### Why chose decision trees?

Different kinds of models have different advantages. The decision tree
model is very good at handling tabular data with numerical features, or
categorical features with fewer than hundreds of categories. Unlike
linear models, decision trees are able to capture non-linear interaction
between the features and the target.

One important note is that tree based models are not designed to work
with very sparse features. When dealing with sparse input data (e.g.
categorical features with large dimension), we can either pre-process
the sparse features to generate numerical statistics, or switch to a
linear model, which is better suited for such scenarios.

##### Advanced Features

Refer to other chapters in this section for the following features:

* [Using categorical features](linear-regression.md#categorical-features)
* [Sparse features](linear-regression.md#sparse-features)
* [List features](linear-regression.md#list-features)
* [Evaluating Results](logistic-regression.md#evaluating-results)
