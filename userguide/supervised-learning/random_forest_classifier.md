# Random Forest Classifier

A Random Forest classifier is one of the most effective machine learning
models for predictive analytics. Refer to the chapter on [random forest
regression](random_forest_regression.md) for background on random
forests.


##### Introductory Example

In this example, we will use the [Mushrooms dataset](https://archive.ics.uci.edu/ml/datasets/mushroom).[<sup>1</sup>](../datasets.md)
```python
import turicreate as tc

# Load the data
data =  tc.SFrame.read_csv('https://raw.githubusercontent.com/apple/turicreate/master/src/unity/python/turicreate/test/mushroom.csv')

# Label 'c' is edible
data['label'] = data['label'] == 'c'

# Make a train-test split
train_data, test_data = data.random_split(0.8)

# Create a model.
model = tc.random_forest_classifier.create(train_data, target='label',
                                           max_iterations=2,
                                           max_depth = 3)

# Save predictions to an SArray.
predictions = model.predict(test_data)

# Evaluate the model and save the results into a dictionary
results = model.evaluate(test_data)
```
See the chapter on [random forest
regression](random_forest_regression.md) for additional tips and tricks
of using the random forest classifier model.

##### Advanced Features

Refer to the earlier chapters for the following features:

* [Using categorical features](linear-regression.md#linregr-categorical-features)
* [Sparse features](linear-regression.md#linregr-sparse-features)
* [List features](linear-regression.md#linregr-list-features)
* [Evaluating Results](logistic-regression.md#logregr-evaluation)
* [Multiclass Classification](logistic-regression.md#logregr-multiclass)
* [Working with imbalanced data](logistic-regression.md#logregr-imbalaced-data)
