# Gradient Boosted Regression Trees

The Gradient Boosted Regression Trees (GBRT) model (also called Gradient
Boosted Machine or GBM), is one of the most effective machine learning
models for predictive analytics, making it the industrial workhorse for
machine learning. Refer to the chapter on [boosted tree
regression](boosted_trees_regression.md) for background on boosted
decision trees.


##### Introductory Example

In this example, we will use the [Mushrooms dataset](https://archive.ics.uci.edu/ml/datasets/mushroom).[<sup>1</sup>](../datasets.md)
```python
import turicreate as tc

# Load the data
data =  tc.SFrame.read_csv('https://raw.githubusercontent.com/apple/turicreate/master/src/python/turicreate/test/mushroom.csv')

# Label 'c' is edible
data['label'] = data['label'] == 'c'

# Make a train-test split
train_data, test_data = data.random_split(0.8)

# Create a model.
model = tc.boosted_trees_classifier.create(train_data, target='label',
                                           max_iterations=2,
                                           max_depth = 3)

# Save predictions to an SFrame (class and corresponding class-probabilities)
predictions = model.classify(test_data)

# Evaluate the model and save the results into a dictionary
results = model.evaluate(test_data)
```
##### Tuning hyperparameters

The Gradient Boosted Trees model has many tuning parameters. Here we
provide a simple guideline for tuning the model.

- `max_iterations`
  Controls the number of trees in the final model. Usually the more trees, the higher accuracy.
  However, both the training and prediction time also grows linearly in the number of trees.

- `max_depth`
  Restricts the depth of each individual tree to prevent overfitting.

- `step_size`
  Also called shrinkage, appeared as the $$\eta$$ in the equations in the Background section.
  It works similar to the learning rate of the gradient descent procedure: smaller value
  will take more iterations to reach the same level of training error of a larger step size.
  So there is a trade off between step_size and number of iterations.

- `min_child_weight`
  One of the pruning criteria for decision tree construction. In classification problem, this
  corresponds to the minimum observations required at a leaf node. Larger value
  produces simpler trees.

- `min_loss_reduction`
  Another pruning criteria for decision tree construction. This restricts the reduction of
  loss function for a node split. Larger value produces simpler trees.

- `row_subsample`
  Use only a fraction of data at each iteration. This is
  similar to the mini-batch [stochastic gradient descent](http://en.wikipedia.org/wiki/Stochastic_gradient_descent)
  which not only reduce the computation cost of each iteration, but may also produce
  more robust model.

- `column_subsample`
  Use only a subset of the columns to use at each iteration.

See the chapter on [boosted trees
regression](boosted_trees_regression.md) for additional tips and tricks
of using the boosted trees classifier model.

##### Advanced Features

Refer to the earlier chapters for the following features:

* [Using categorical features](linear-regression.md#linregr-categorical-features)
* [Sparse features](linear-regression.md#linregr-sparse-features)
* [List features](linear-regression.md#linregr-list-features)
* [Evaluating Results](logistic-regression.md#logregr-evaluation)
* [Multiclass Classification](logistic-regression.md#logregr-multiclass)
* [Working with imbalanced data](logistic-regression.md#logregr-imbalaced-data)
