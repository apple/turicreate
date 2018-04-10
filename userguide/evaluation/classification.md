# Classification Metrics

As mentioned previously, evaluation metrics are tied to the machine learning task. In this section, we will cover metrics for classification tasks. In binary classification, there are two possible output classes. In multi-class
classification, there are more than two possible classes.

There are many ways of measuring classification performance:
- [Accuracy](classification.md#accuracy)
- [Confusion matrix](classification.md#confusion_matrix)
- [Log-loss](classification.md#log_loss)
- [Precision and Recall](classification.md#precision_recall)
- [F-Scores](classification.md#f_scores)
- [Receiver operating characteristic (ROC) curve](classification.md#roc_curve)
- [Area under curve (AUC) ("curve" corresponds to the ROC curve)](classification.md#auc)



## Accuracy {#accuracy}

Accuracy simply measures how often the classifier makes the correct prediction.
It’s the ratio between the number of correct predictions and the total number
of predictions (the number of test data points).

$$
    \mbox{accuracy} = \frac{\mbox{# correct}}{\mbox{# predictions}}
$$

```python
import turicreate as tc

y    = tc.SArray(["cat", "dog", "cat", "cat"])
yhat = tc.SArray(["cat", "dog", "cat", "dog"])

print(tc.evaluation.accuracy(y, yhat))
```
```
0.75
```

Accuracy looks easy enough. However, it makes no distinction between classes;
correct answers for each class are treated equally. Sometimes this is not
enough. You might want to look at how many examples failed for each class.
This would be the case if the cost of misclassification is different, or if you
have a lot more test data of one class than the other. For instance, making the
call that a patient has cancer when he doesn’t (known as a false positive) has
very different consequences than making the call that a patient doesn’t have
cancer when he does (a false negative).


### Multiclass Averaging {#multi-class-averaging}

The multi-class setting is an extension of the binary setting. The accuracy
metrics can be "averaged" across all the classes in many possible ways. Some of
them are:

* **micro**: Calculate metrics globally by counting the total number of times
  each class was correctly predicted and incorrectly predicted.
* **macro**: Calculate metrics for each "class" independently, and find their
  unweighted mean. This does not take label imbalance into account.
* **None**: Return a metric corresponding to each class.

```python
import turicreate as tc

y    = tc.SArray(["cat", "dog", "foosa", "cat"])
yhat = tc.SArray(["cat", "dog", "cat", "dog"])

print(tc.evaluation.accuracy(y, yhat, average = "micro"))
print(tc.evaluation.accuracy(y, yhat, average = "macro"))
print(tc.evaluation.accuracy(y, yhat, average = None))
```
```no-highlight
0.5
0.666666666667
{'dog': 0.75, 'foosa': 0.75, 'cat': 0.5}
```

In general, when there are different numbers of examples per class, the average
per-class accuracy will be different from the micro-average accuracy.  When the
classes are imbalanced, i.e., there are a lot more examples of one class than
the other, then the accuracy will give a very distorted picture, because the
class with more examples will dominate the statistic. In that case, you should
look at the average per-class accuracy (average="micro"), as well as the
individual per-class accuracy numbers (average=None). Per-class accuracy is not
without its own caveats, however: for instance, if there are very few examples of one
class, the test statistics for that class will be unreliable (i.e., they have
large variance), so it’s not statistically sound to average quantities with
different degrees of variance.


## Confusion matrix {#confusion_matrix}

A confusion matrix (or confusion table) shows a more detailed breakdown of
correct and incorrect classifications for each class. Here is an example of how
the confusion matrix can be computed.

The confusion table is an SFrame consisting of three columns:
* **target_label**: The label of the ground truth.
* **predicted_label**: The predicted label.
* **count**: The number of times the `target_label` was predicted as the
  `predicted_label`.


```python
y    = tc.SArray(["cat", "dog", "foosa", "cat"])
yhat = tc.SArray(["cat", "dog", "cat", "dog"])

cf_matrix = tc.evaluation.confusion_matrix(y, yhat)
```
```no-highlight
Columns:
	target_label	str
	predicted_label	str
	count	int

Rows: 4

Data:
+--------------+-----------------+-------+
| target_label | predicted_label | count |
+--------------+-----------------+-------+
|    foosa     |       cat       |   1   |
|     cat      |       dog       |   1   |
|     dog      |       dog       |   1   |
|     cat      |       cat       |   1   |
+--------------+-----------------+-------+
[4 rows x 3 columns]
```

Looking at the matrix, one can clearly get a better picture of which class the
model best identifies. This information is lost if one only looks at the
overall accuracy.


## Log-loss {#log_loss}

Log-loss, or logarithmic loss, gets into the finer details of a classifier. In
particular, if the raw output of the classifier is a numeric probability
instead of a class label, then log-loss can be used. The probability
essentially serves as a gauge of confidence. If the true label is "0" but the
classifier thinks it belongs to class "1" with probability 0.51, then the
classifier would be making a mistake. But it’s a near miss because the
probability is very close to the decision boundary of 0.5. Log-loss is a “soft”
measurement of accuracy that incorporates this idea of probabilistic
confidence.

Mathematically, log-loss for a binary classifier looks like this:

$$
\mbox{log-loss} = \sum_{i=1}^{N} y_i \log(p_i) + (1 - y_i) \log(1 - p_i)
$$

Here, $$p_i$$ is the probability that the i-th data point belongs to class "1",
as judged by the classifier. $$y_i$$ is the true label and is either "0" or
"1".  The beautiful thing about this definition is that it is intimately tied
to information theory: Intuitively, log-loss measures the unpredictability of
the “extra noise” that comes from using a predictor as opposed to the true
labels.  By minimizing the cross entropy, we maximize the accuracy of the
classifier.

Here is an example of how this is computed:

```python
import turicreate as tc

targets = tc.SArray([0, 1, 1, 0])
predictions = tc.SArray([0.1, 0.35, 0.7, 0.99])

log_loss = tc.evaluation.log_loss(targets, predictions)
```
```no-highlight
1.5292569425208318
```

Logloss is undefined when a probability value $$p_i = 0$$, or $$p_i = 1$$.
Hence, probabilities are clipped to $$\max(\epsilon, \min(1 - \epsilon, p_i))$$
where $$\epsilon=1.0 \times 10^{-15}$$


### Multi-class log-loss

In the multi-class setting, log-loss requires a vector of probabilities (that
sum to 1) for each class label in the input dataset. In this example, there are
three classes [0, 1, 2], and the vector of probabilities correspond to the
probability of prediction for each of the three classes (while maintaining
ordering).

```python
targets    = tc.SArray([ 1, 0, 2, 1])
predictions = tc.SArray([[.1, .8, 0.1],
                        [.9, .1, 0.0],
                        [.8, .1, 0.1],
                        [.3, .6, 0.1]])

log_loss = tc.evaluation.log_loss(targets, predictions)
```
```no-highlight
0.785478695933018
```

For multi-class classification, when the target label is of type **string**,
then the probability vector is assumed to be a vector of probabilities of class
as sorted alphanumerically. Hence, for the probability vector [0.1, 0.2, 0.7]
for a dataset with classes ["cat", "dog", "rat"; the 0.1 refers to "cat", 0.2
refers to "dog", and 0.7 to "rat".


```python
target    = tc.SArray([ "dog", "cat", "foosa", "dog"])
predictions = tc.SArray([[.1, .8, 0.1],
                        [.9, .1, 0.0],
                        [.8, .1, 0.1],
                        [.3, .6, 0.1]])
log_loss = tc.evaluation.log_loss(y, yhat)
```
```no-highlight
1.5292569425208318
```
## Precision & Recall {#precision_recall}

Precision and recall are actually two metrics. But they are often used
together. Precision answers the question: *Out of the items that the classifier
predicted to be true, how many are actually true?* Whereas, recall answers the
question: *Out of all the items that are true, how many are found to
be true by the classifier?*

The precision score quantifies the ability of a classifier to not label a
**negative** example as **positive**. The precision score can be interpreted as
the probability that a **positive** prediction made by the classifier is
**positive**.  The score is in the range [0,1] with 0 being the worst, and 1
being perfect.


The precision and recall scores can be defined as:

$$\mbox{precision} = \frac{\mbox{# true positive}}{\mbox{#true positive + #false positive}}$$
$$\mbox{recall} = \frac{\mbox{# true positive}}{\mbox{#true positive + #false negative}}$$

```python
targets = turicreate.SArray([0, 1, 0, 0, 0, 1, 0, 0])
predictions = turicreate.SArray([1, 0, 0, 1, 0, 1, 0, 1])

pr_score   = turicreate.evaluation.precision(targets, predictions)
rec_score  = turicreate.evaluation.recall(targets, predictions)
print pr_score, rec_score
```
```no-highlight
0.25, 0.5
```

Precision can also be defined then the target classes are of type **string**.
For binary classification, when the target label is of type **string**, then
the labels are sorted alphanumerically and the largest label is considered the
"positive" label.


```python
targets = turicreate.SArray(['cat', 'dog', 'cat', 'cat', 'cat', 'dog', 'cat', 'cat'])
predictions = turicreate.SArray(['dog', 'cat', 'cat', 'dog', 'cat', 'dog', 'cat', 'dog'])

pr_score   = turicreate.evaluation.precision(targets, predictions)
rec_score  = turicreate.evaluation.recall(targets, predictions)
print(pr_score, rec_score)
```
```no-highlight
0.25, 0.5
```

### Multi-class precision-recall

Precision and recall scores can also be defined in the multi-class setting.
Here, the metrics can be "averaged" across all the classes in many possible
ways. Some of them are:

* **micro**: Calculate metrics globally by counting the total number of times
  each class was correctly predicted and incorrectly predicted.
* **macro**: Calculate metrics for each "class" independently, and find their
  unweighted mean. This does not take label imbalance into account.
* **None**: Return the accuracy score for each class.corresponding to each
  class.


```python
targets = turicreate.SArray(['cat', 'dog', 'cat', 'cat', 'cat', 'dog', 'cat', 'foosa'])
predictions = turicreate.SArray(['dog', 'cat', 'cat', 'dog', 'cat', 'dog', 'cat', 'foosa'])

macro_pr = turicreate.evaluation.precision(targets, predictions, average='macro')
micro_pr = turicreate.evaluation.precision(targets, predictions, average='micro')
per_class_pr = turicreate.evaluation.precision(targets, predictions, average=None)

print(macro_pr, micro_pr)
print(per_class_pr)
```
```no-highlight
0.694444444444 0.625
{'foosa': 1.0, 'dog': 0.3333333333333333, 'cat': 0.75}
```

**Note:** The micro average precision, recall, and accuracy scores are
mathematically equivalent.

** Undefined Precision-Recall **
The precision (or recall) score is not defined when the number of true positives + false positives (true positives + false negatives) is zero. In
other words, then the denominators of the respective equations are 0, the
metrics are not defined. In those settings, we return a value of `None`. In the
multi-class setting, the `None` is skipped during averaging.

## F-scores (F1, F-beta) {#f_scores}

The F1-score is a single metric that combines both precision and recall via
their harmonic mean:

$$F_1 = 2 \frac{\mbox{precision} * \mbox{recall}}{\mbox{precision + recall}}$$

The score lies in the range [0,1] with 1 being ideal and 0 being the worst.
Unlike the arithmetic mean, the harmonic mean tends toward the smaller of the
two elements.  Hence the F1 score will be small if either precision or recall
is small.

The F1-score (sometimes known as the balanced F-beta score), is a special case
of a metric known as the F-Beta score, which *measures the effectiveness of
retrieval with respect to a user who attaches $$\beta$$ times as much
importance to recall as to precision*.


$$F_\beta = (1 + \beta^2) \frac{\mbox{precision} * \mbox{recall}}{\mbox{\beta^2 precision + recall}}.$$

```python
targets = turicreate.SArray([0, 1, 0, 0, 0, 1, 0, 0])
predictions = turicreate.SArray([1, 0, 0, 1, 0, 1, 0, 1])

f1    = turicreate.evaluation.f1_score(targets, predictions)
fbeta = turicreate.evaluation.fbeta_score(targets, predictions, beta = 2.0)
print(f1, fbeta)
```
```no-highlight
0.333333333333 0.416666666667
```

Like the other metrics, the F1-score (or F-beta score) can also be defined when
the target classes are of type **string**.  For binary classification, when the
target label is of type **string**, then the labels are sorted alphanumerically
and the largest label is considered the "positive" label.

```python
targets = turicreate.SArray(['cat', 'dog', 'cat', 'cat', 'cat', 'dog', 'cat', 'cat'])
predictions = turicreate.SArray(['dog', 'cat', 'cat', 'dog', 'cat', 'dog', 'cat', 'dog'])

f1    = turicreate.evaluation.f1_score(targets, predictions)
fbeta = turicreate.evaluation.fbeta_score(targets, predictions, beta = 2.0)
print(f1, fbeta)
```
```no-highlight
0.333333333333 0.416666666667
```

### Multi-class F-scores

F-scores can also be defined in the multi-class setting.  Here, the metrics can
be "averaged" across all the classes in many possible ways. Some of them are:

* **micro**: Calculate metrics globally by counting the total number of times
  each class was correctly predicted and incorrectly predicted.
* **macro**: Calculate metrics for each "class" independently, and find their
  unweighted mean. This does not take label imbalance into account.
* **None**: Return the accuracy score for each class.corresponding to each
  class.


**Note:** The micro average precision, recall, and accuracy scores are
mathematically equivalent.

## Receiver Operating Characteristic (ROC Curve)  {#roc_curve}

In statistics, a receiver operating characteristic (ROC), or ROC curve, is a
graphical plot that illustrates the performance of a binary classifier system
as its prediction **threshold** is varied. The ROC curve provides nuanced
details about the behavior of the classifier.  The curve is created by plotting
the **true positive rate** (TPR) against the **false positive rate** (FPR) at
various threshold settings.

This exotic sounding name originated in the 1950s from radio signal analysis,
and was made popular by a 1978 paper by Charles Metz called "Basic Principles
of ROC analysis." The ROC curve shows the sensitivity of the classifier by
plotting the rate of true positives to the rate of false positives. In other
words, it shows you how many correct positive classifications can be gained as
you allow for more and more false positives.  The perfect classifier that makes
no mistakes would hit a true positive rate of 100% immediately, without
incurring any false positives. This almost never happens in practice.


A good ROC curve has a lot of space under it (because the true positive rate
shoots up to 100% very quickly). A bad ROC curve covers very little area.

```python
targets = turicreate.SArray([0, 1, 1, 0])
predictions = turicreate.SArray([0.1, 0.35, 0.7, 0.99])

roc_curve = turicreate.evaluation.roc_curve(targets, predictions)
```
```no-highlight
Data:
+-----------+-----+-----+---+---+
| threshold | fpr | tpr | p | n |
+-----------+-----+-----+---+---+
|    0.0    | 1.0 | 1.0 | 2 | 2 |
|   1e-05   | 1.0 | 1.0 | 2 | 2 |
|   2e-05   | 1.0 | 1.0 | 2 | 2 |
|   3e-05   | 1.0 | 1.0 | 2 | 2 |
|   4e-05   | 1.0 | 1.0 | 2 | 2 |
|   5e-05   | 1.0 | 1.0 | 2 | 2 |
|   6e-05   | 1.0 | 1.0 | 2 | 2 |
|   7e-05   | 1.0 | 1.0 | 2 | 2 |
|   8e-05   | 1.0 | 1.0 | 2 | 2 |
|   9e-05   | 1.0 | 1.0 | 2 | 2 |
+-----------+-----+-----+---+---+
[100001 rows x 5 columns]
```

The result of the **roc curve** is a multi-column **SFrame** with the following
columns:

- **tpr**: True positive rate, the number of true positives divided by the number of positives.
- **fpr**: False positive rate, the number of false positives divided by the number of negatives.
- **p**: Total number of positive values.
- **n**: Total number of negative values.
- **class**: Reference class for this ROC curve (for multi-class classification).

**Note**: The ROC curve is computed using a binned histogram and hence always
contains 100K rows. The binned histogram provides a curve that is accurate to
the 5th decimal.

```python
targets = turicreate.SArray(["cat", "dog", "cat", "dog"])
predictions = turicreate.SArray([0.1, 0.35, 0.7, 0.99])

roc_curve = turicreate.evaluation.roc_curve(targets, predictions)
```
```no-highlight
Data:
+-----------+-----+-----+---+---+
| threshold | fpr | tpr | p | n |
+-----------+-----+-----+---+---+
|    0.0    | 1.0 | 1.0 | 2 | 2 |
|   1e-05   | 1.0 | 1.0 | 2 | 2 |
|   2e-05   | 1.0 | 1.0 | 2 | 2 |
|   3e-05   | 1.0 | 1.0 | 2 | 2 |
|   4e-05   | 1.0 | 1.0 | 2 | 2 |
|   5e-05   | 1.0 | 1.0 | 2 | 2 |
|   6e-05   | 1.0 | 1.0 | 2 | 2 |
|   7e-05   | 1.0 | 1.0 | 2 | 2 |
|   8e-05   | 1.0 | 1.0 | 2 | 2 |
|   9e-05   | 1.0 | 1.0 | 2 | 2 |
+-----------+-----+-----+---+---+
[100001 rows x 5 columns]
```

For binary classification, when the target label is of type **string**, then
the labels are sorted alphanumerically and the largest label is chosen as the
the **positive class**. The ROC curve can also be defined in the multi-class
setting by returning a single curve for each class.

## Area under the curve (AUC)  {#auc}

AUC stands for Area Under the Curve. Here, the curve is the ROC curve. As
mentioned above, a good ROC curve has a lot of space under it (because the true
positive rate shoots up to 100% very quickly). A bad ROC curve covers very
little area.


```python
targets = turicreate.SArray([0, 1, 1, 0])
predictions = turicreate.SArray([0.1, 0.35, 0.7, 0.99])

auc = turicreate.evaluation.auc(targets, predictions)
print(auc)
```
```no-highlight
0.5
```

**Note**: The AUC score is computed using a binned histogram and hence always
contains 100K rows. The binned histogram provides a curve that is accurate to
the 5th decimal.

The AUC score can also be defined when the target classes are of type
**string**.  For binary classification, when the target label is of type
**string**, then the labels are sorted alphanumerically and the largest label
is considered the "positive" label.

```python
targets = turicreate.SArray(["cat", "dog", "cat", "dog"])
predictions = turicreate.SArray([0.1, 0.35, 0.7, 0.99])

auc = turicreate.evaluation.auc(targets, predictions)
print(auc)
```
```no-highlight
0.5
```

### Multi-class area under curve

The AUC score can also be defined in the multi-class setting.  Here, the
metrics can be "averaged" across all the classes in many possible ways. Some of
them are:

* **macro**: Calculate metrics for each "class" independently, and find their
  unweighted mean. This does not take label imbalance into account.
* **None**: Return a metric corresponding to each class.


## Calibration curve  {#calibration_curve}

A calibration curve that compares the true probability of an event with its
predicted probability. This chart shows if the  created model has well
"calibrated" probability predictions. The closer the calibration curve is to
the "ideal" curve, the more confident one can be in interpreting the
probability predictions as a confidence score for predicting churn.

The graph is constructed by segmenting the probability space, from 0.0 to 1.0,
into a fixed number of discrete steps. For instance, for a step size of 0.1,
the probability space would be divided into 10 buckets: [0.0, 0.1), [0.1, 0.2)
... [0.9, 1.0]. Given a set of events for which a predicted probability has
been computed, each predicted probability is rounded to the appropriate bucket.
An aggregation is then performed to compute the total number of data points
within each probability range along with fraction of those data points that
were truly positive events. The calibration curve plots the fraction of events
that were truly positive for each probability bucket. A perfectly calibrated
model, the curve would be a line from (0.0, 0.0) to (1.0, 1.0).

Consider a simple example of a binary classification problem where a model
was created to predict a click or no-click event. If an event was “clicked”,
and had a predicted probability of 0.345, using a binning of 0.1, the “number
of events” for bin [0.3, 0.4) would be increased by one, and the “number of
true” events would be increased by one. If an event with probability 0.231 was
“not clicked”, the “number of events” for bin [0.2, 0.3) would be increased by
one, while the “number of true” events would not be increased. For each bin,
the “number of true” over the “total number of events” becomes the observed
probability (y-axis value). The average predicted probability for the bin
becomes the x-axis value.


## Precision-Recall Curve {#precision_recall_curve}

The precision-recall curve plots the inherent trade-off between precision and
recall. It is easier to understand and interpret this curve by understanding
each of the components of this curve at different "thresholds".

Consider a simple example of a binary classification problem where a
model was created to predict a click or no-click event. If an event was
“clicked”, Looking at the confusion matrix, the model predictions can be
broken down into four categories:

- Events that were predicted to be "click" and were actually "click" events
  (True Positive, TP).
- Events that were predicted to be "not-click" but were not "click" events
  (False Positive, FP).
- Events that were predicted to be "not-click" and were actually "not-click"
  events (True Negative, TN).
- Events that were predicted to be "click" but were actually "not-click" events
  (False Negative, FN).

As defined earlier, precision is fraction of predicted click events that were
actually click events while recall is the fraction of click events that were
correctly predicted by the model. The definitions of precision and recall are
not necessarily discrete events and can depend on the prediction probabilities.
One can define a "threshold" of probability above which the model prediction
can be considered a "click" prediction.

For example, a "threshold" of 0.5 implies that all probability predictions
above 0.5 are considered "click" events and all predictions below are
considered "not-click" events. By varying the threshold between 0.0 and 1.0, we
can compute different precision and recall numbers. At a threshold of 0.0 all
the model predictions are "click". At this point, the model exhibits perfect
recall but has the worst possible precision. At a threshold of 1.0, the model
predicts every event as "not-click"; this is prefect precision (the model was
never wrong at predicting clicks!) but the worst possible recall.

This by plotting precision and recall at various thresholds, the
precision-recall curve illustrates the trade-off of precision and recall for
the model, at varying thresholds. Two models can be compared easily by plotting
their precision-recall curve. A curve that is more to the top-right hand side
of the plot represents a better model.
