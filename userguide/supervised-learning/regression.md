# Regression

 **Regression** is the problem of learning a functional relationship between
**input features** and an **output target** using training data where the
specific functional form learned depends on the choice of model.  The
parameters of the function are learned using data where the target values are
known, so that the machine can make predictions about data where the target is
unknown.  The goal of a regression model is to learn to **predict** an output
based on an input set of features. The following figure depicts a sample
supervised learning model where the training data (depicted in blue) is used to
learn a function (depicted in pink) which can then be used in the future to
make predictions.

<div id="supervised-plot"></div>
<script src="images/regression-plot.js"></script>

Creating regression models is easy with Turi Create! The regression
toolkit implements the following models:

* [Linear regression](linear-regression.md)
* [Boosted Decision Trees](boosted_trees_regression.md)

These algorithms differ in how they make predictions, but conform to the same
API. With all models, call **create()** to create a model, **predict()** to make
predictions on the returned model, and **evaluate()** to measure performance of
the predictions. All models can incorporate:

* Numeric features
* Categorical variables
* Sparse features (i.e feature sets that have a large set of features,
of which only a small subset of values are non-zero)
* Dense features (i.e
feature sets with a large number of numeric features)
* Text data

#### Model Selector

It isn't always clear that we know exactly which model is suitable for a
given task.  Turi Create's model selector automatically picks the right
model for you based on statistics collected from the data set.

```python
import turicreate as tc

# Load the data
data =  tc.SFrame('ratings-data.csv')

# Make a train-test split
train_data, test_data = data.random_split(0.8)

# Automatically picks the right model based on your data.
model = tc.regression.create(train_data, target='stars',
                                    features = ['user_avg_stars',
                                                'business_avg_stars',
                                                'user_review_count',
                                                'business_review_count'])

# Save predictions to an SArray
predictions = model.predict(test_data)

# Evaluate the model and save the results into a dictionary
results = model.evaluate(test_data)
```

Turi Create implementations are built to work with up to billions of
examples and up to millions of features.
