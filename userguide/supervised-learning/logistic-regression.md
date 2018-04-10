# Logistic Regression

Logistic regression is a regression model that is popularly used for
classification tasks. In logistic regression, the probability that a  **binary
target is True** is modeled as a [logistic
function](http://en.wikipedia.org/wiki/Logistic_function) of a linear
combination of features.

The following figure illustrates how logistic regression is used to
create a 1-dimensional classifier. The training data consists of
positive examples (depicted in blue) and negative examples (in orange).
The decision boundary (depicted in pink) separates out the data into two
classes.

<div id="logregr-plot"></div>
<script src="images/logregr-plot.js"></script>


##### Background


Given a set of features $$x_i$$, and a label $$y_i \in \{0,1\}$$, logistic
regression interprets the probability that the label is in one class as a
logistic function of a linear combination of the features:

$$
    f_i(\theta) =  p(y_i = 1 | x) = \frac{1}{1 + \exp(-\theta^T x)}
$$

Analogous to linear regression, an intercept term is added by appending a column
of 1's to the features and L1 and L2 regularizers are supported. The composite
objective being optimized for is the following:

$$
    \min_{\theta} \sum_{i = 1}^{n} f_i(\theta) + \lambda_1 \|\theta\|_1 +
\lambda_2 \|\theta\|^2_2
$$

where $$\lambda_1$$ is the ``L1_penalty`` and $$\lambda_2$$ is the ``L2_penalty``.

##### Introductory Example

First, let's construct a binary target variable. In this example, we will
predict **if a restaurant is good or bad**, with 1 and 2 star ratings indicating
a bad business and 3-5 star ratings indicating a good one. We will use the
following features.
* Average rating of a given business
* Average rating made by a user
* Number of reviews made by a user
* Number of reviews that concern a business


```python
import turicreate as tc

# Load the data
data =  tc.SFrame('ratings-data.csv')

# Restaurants with rating >=3 are good
data['is_good'] = data['stars'] >= 3

# Make a train-test split
train_data, test_data = data.random_split(0.8)

# Create a model.
model = tc.logistic_classifier.create(train_data, target='is_good',
                                    features = ['user_avg_stars',
                                                'business_avg_stars',
                                                'user_review_count',
                                                'business_review_count'])

# Save predictions (probability estimates) to an SArray
predictions = model.classify(test_data)

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
* [Choosing the solver](linear-regression.md#linregr-solver)
* [Regularizing models](linear-regression.md#linregr-regularizer)

We will now discuss some advanced features that are **specific to logistic
regression**.

###### Making Predictions

Predictions using a Turi Create classifier is easy. The **classify()** method provides
a one-stop shop for all that you need from a classifier.

* A class prediction
* Probability/Confidence associated with that class prediction.

In the following example, the first prediction was class **1** with a **90.5%**
probability.

```python
predictions = model.classify(test_data)
print(predictions)
```
```no-highlight
+-------+----------------+
| class |  probability   |
+-------+----------------+
|   1   | 0.905618772131 |
|   1   | 0.941576249302 |
|   1   | 0.948254387657 |
|   0   | 0.996952633956 |
|   1   | 0.944229260472 |
|   1   | 0.951769966846 |
|   1   | 0.905561314917 |
|   1   | 0.957697429248 |
|   1   | 0.98527411871  |
|   1   | 0.973282185166 |
|  ...  |      ...       |
+-------+----------------+
[43018 rows x 2 columns]
Note: Only the head of the SFrame is printed.
You can use print_rows(num_rows=m, num_columns=n) to print more rows and columns.
```

###### Making detailed predictions


Logistic regression predictions can take one of three forms:

* **Classes** (default): Thresholds the probability estimate at 0.5 to predict
* a class label i.e. **0/1**.
* **Probabilities**: A probability estimate (in the range [0,1]) that
the example is in the **True** class. Note that this is not the same as the
probability estimate in the classify function.
* **Margins** : Distance to the linear decision boundary learned by the model.
The larger the distance, the more confidence we have that it belongs to one
class or the other.

Turi Create's logistic regression model can return
[predictions](https://apple.github.io/turicreate/docs/api/generated/turicreate.logistic_classifier.LogisticClassifier.predict.html)
for any of these types:


```python
pred_class = model.predict(test_data, output_type = "class")          # Class
pred_prob_one = model.predict(test_data, output_type = 'probability') # Probability
pred_margin = model.predict(test_data, output_type = "margin")        # Margins
```


######  <a name="logregr-evaluation"></a> Evaluating Results

We can also evaluate our predictions by comparing them to known ratings. The
results are evaluated using two metrics:

* [Classification Accuracy](http://en.wikipedia.org/wiki/Accuracy_and_precision): Fraction of test
set examples with correct class label predictions.
* [Confusion Matrix](http://en.wikipedia.org/wiki/Confusion_matrix): Cross-
tabulation of predicted and actual class labels.


```python
result = model.evaluate(test_data)
print(Accuracy         : %s" % result['accuracy'])
print("Confusion Matrix : \n%s" % result['confusion_matrix'])
```
```no-highlight
Accuracy         : 0.860862092991
Confusion Matrix :
+--------------+-----------------+-------+
| target_label | predicted_label | count |
+--------------+-----------------+-------+
|      0       |        0        |  2348 |
|      0       |        1        |  4816 |
|      1       |        0        |  912  |
|      1       |        1        | 34942 |
+--------------+-----------------+-------+
[4 rows x 3 columns]
```

######  <a name="logregr-imbalanced-data"></a> Working with imbalanced data

Many difficult **real-world** problems have imbalanced data, where at least one
class is under-represented. Turi Create models can handle the imbalanced data by
assigning asymmetric costs of misclassifying elements of different classes.

```python
# Load the data
data =  tc.SFrame.read_csv('mushroom.csv')

# Label 'c' is edible
data['label'] = data['label'] == 'c'

# Make a train-test split
train_data, test_data = data.random_split(0.8)

# Create a model which weights classes based on frequency in the training data.
model = tc.logistic_classifier.create(train_data, target='label',
                                      class_weights = 'auto')
```

#####  <a name="logregr-multiclass"></a> Multiclass Classification

Multiclass classification is the problem of classifying instances into one of
many (i.e more than two) possible instances. As an example, binary
classification can be used to create a classifier that can distinguish between
two classes, say "cat" or "dog", while multiclass classification can be used to
create a model a finite set of labels at the same time, say "cat", "dog", "rat", and
"cow".

```python
import turicreate as tc

# Load the data
data = tc.SFrame('mnist/train6k-array')

# Make a train-test split
train_data, test_data = data.random_split(0.8)

# Create a model
model = tc.logistic_classifier.create(train_data, target='label')

# Save predictions to an SFrame (class and corresponding class-probabilities)
predictions = model.classify(test_data)

# Top 5 predictions with probabilities, rank, and margin
top = model.predict_topk(test_data, output_type='probability', k = 5)
top = model.predict_topk(test_data, output_type='rank', k = 5)
top = model.predict_topk(test_data, output_type='margin', k = 5)

# Evaluate the model and save the results into a dictionary
results = model.evaluate(test_data)
```

###### Top-k predictions

Multiclass classification provides the top-k class predictions for each class.
The predictions are either margins, probabilities, or a rank for the predicted
class for each example. In the following example, we provide the top 5
predictions, ordered by class probability, for each data point in the test set.

```python
top = model.predict_topk(test_data, output_type='probability', k = 3)
print(top)
```
```no-highlight
Columns:
  id  str
  class str
  probability float

  Rows: 3711

Data:
+-----+-------+------------------+
|  id | class |   probability    |
+-----+-------+------------------+
|  0  |   5   |  0.94296406887   |
|  0  |   1   | 0.0140330526641  |
|  0  |   8   | 0.00636249767982 |
|  1  |   8   |  0.929146865934  |
|  1  |   9   | 0.0139581314344  |
|  1  |   1   | 0.00982837828507 |
|  2  |   1   |  0.937192457289  |
|  2  |   7   | 0.0106293228679  |
|  2  |   4   | 0.00910849289074 |
|  3  |   5   |  0.900146607924  |
| ... |  ...  |       ...        |
+-----+-------+------------------+
```
