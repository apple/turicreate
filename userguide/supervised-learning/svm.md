# Support Vector Machines
Support Vector Machines (SVM) is another popular model used for
classification tasks. In logistic regression, the probability that a
**binary target is True** is modeled as a [logistic
function](http://en.wikipedia.org/wiki/Logistic_function) of the
features. The following figure illustrates how an SVM is used to create a
2-dimensional classifier. The training data consists of positive
examples (depicted in orange) and negative examples (in blue). The
decision boundary (depicted in pink) separates out the data into two
classes.

<div id="svm-plot"></div>
<script src="images/svm-plot.js"></script>

##### Background

Currently, Turi Create implements a linear C-SVM (SVC). In this model, given
a set of features $$x_i$$, and a label $$y_i \in \{0,1\}$$ the linear SVM minimizes
the loss function:

$$
          f_i(\theta) =  \max(1 - \theta^T x, 0)
$$

As with other models, an intercept term is added by appending a column of 1's to
the features. The composite objective being
optimized for is the following:

$$
           \min_{\theta} \lambda \sum_{i = 1}^{n} f_i(\theta) + \|\theta \|^{2}_{2}
$$

where $$\lambda$$ is the ``penalty`` parameter (the C in the C-SVM) that
determines the weight in the loss function towards the regularizer. The larger
the value of $$\lambda$$, the more is the weight given to the mis-classification
loss. Turi Create solves the Linear-SVM formulation by approximating the
hinge-loss with a smooth function (see
[Zhang et. al.](https://www.aaai.org/Papers/ICML/2003/ICML03-115.pdf)
for details).

##### Introductory Example

Using the same example as we did for logistic regression, we will predict **if a
restaurant is good or bad**, with 1 and 2 star ratings indicating a bad business
and 3-5 star ratings indicating a good one. We will use the following features:

* Average rating of a given business
* Average rating made by a user
* Number of reviews made by a user
* Number of reviews that concern a business

The usage is similar to the logistic regression module:


```python
import turicreate as tc

# Load the data
data =  tc.SFrame('ratings-data.csv')

# Restaurants with rating >=3 are good
data['is_good'] = data['stars'] >= 3

# Make a train-test split
train_data, test_data = data.random_split(0.8)

# Create a model.
model = tc.svm_classifier.create(train_data, target='is_good',
                                    features = ['user_avg_stars',
                                                'business_avg_stars',
                                                'user_review_count',
                                                'business_review_count'])

# Save predictions (class only) to an SFrame
predictions = model.predict(test_data)

# Evaluate the model and save the results into a dictionary
results = model.evaluate(test_data)
```

##### Advanced Usage

Refer to the chapter on linear regression for the following features:

* [Accessing attributes of the model](linear-regression.md#linregr-model-access)
* [Interpreting results](linear-regression.md#linregr-interpreting-results)
* [Using categorical features](linear-regression.md#linregr-categorical-features)
* [Sparse features](linear-regression.md#linregr-sparse-features)
* [List features](linear-regression.md#linregr-list-features)
* [Feature rescaling](linear-regression.md#linregr-feature-rescaling)
* [Chosing the solver](linear-regression.md#linregr-solver)
* [Regularizing models](linear-regression.md#linregr-regularizer)
* [Evaluating Results](logistic-regression.md#logregr-evaluation)
* [Working with imbalanced data](logistic-regression.md#logregr-imbalaced-data)

We will now discuss some advanced features that are **specific to SVM**.

###### Making Predictions

Predictions using a Turi classifier is easy. The **classify()** method
provides a one-stop shop for all that you need from a classifier. In the
following example, the first prediction was class **1**. Currently, the
SVM classifier is not calibrated for probability predictions. Stay tuned
for that feature in an upcoming release.

```python
predictions = model.classify(test_data)
print predictions
```
```no-highlight
+-------+
| class |
+-------+
|   1   |
|   1   |
|   1   |
|   1   |
|   1   |
|   1   |
|   1   |
|   1   |
|   1   |
|   1   |
|  ...  |
+-------+
[43414 rows x 1 columns]
Note: Only the head of the SFrame is printed.
You can use print_rows(num_rows=m, num_columns=n) to print more rows and columns.
```


###### Making detailed predictions

SVM predictions can take one of two forms:

* **Margins** : Distance to the linear decision boundary learned by the model.
The larger the distance, the more confidence we have that it belongs to one
class or the other.
* **Classes** (default) : Thresholds the margin at 0.0 to predict a class label
i.e. **0/1**.

SVM does not currently support predictions as probability estimates.


```python
pred_class = model.predict(test_data, output_type = "class")    # Class
pred_margin = model.predict(test_data, output_type = "margin")  # Margins
```

###### Penalty Term

The SVM model contains a ``penalty`` term on the mis-classification loss of the
model. The smaller this weight, the lower is the emphasis placed on
misclassified examples which in-turn results in smaller coefficients. The
``penalty`` term can be set as follows:


```python
model = tc.svm_classifier.create(train_data, target='is_good', penalty=100,
                                    features = ['user_avg_stars',
                                                'business_avg_stars',
                                                'user_review_count',
                                                'business_review_count'])
```
