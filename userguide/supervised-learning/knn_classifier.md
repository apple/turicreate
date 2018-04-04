# Nearest Neighbor Classifier
The [nearest neighbor classifier](https://apple.github.io/turicreate/docs/api/generated/turicreate.nearest_neighbor_classifier.NearestNeighborClassifier.html)
is one of the simplest classification models, but it often performs nearly as
well as more sophisticated methods.

#### Background

The nearest neighbors classifier predicts the class of a data point to be the
most common class among that point's neighbors. Suppose we have $$n$$ training
data points, where the $$i$$'th point has both a vector of features $$x_i$$ and
class label $$y_i$$. For a new point $$x^*$$, the nearest neighbor classifier
first finds the set of neighbors of $$x^*$$, denoted $$N(x^*)$$. The class label
for $$x^*$$ is then predicted to be

$$
    y* = \max_c \sum_{i \in N(x^*)} I(y_i = c)
$$

where the indicator function $$I()$$ is 1 if the argument is true, and 0
otherwise. The simplicity of this approach makes the model relatively
straightforward to understand and communicate to others, and naturally lends
itself to multi-class classification.

Defining the criteria for the **neighborhood** of a prediction data point
requires careful thought and domain knowledge. A function must be specified to
measure the distance between any two data points, and then the size of
"neighborhoods" relative to this distance function must be set.

For the first step, there are many standard distance functions (e.g.
Euclidean, Jaccard, Levenshtein) that work well for data whose features
are all of the same type, but for heterogeneous data the task is a bit
trickier. Turi Create overcomes this problem with **composite
distances**, which are weighted sums of standard distance functions
applied to appropriate subsets of features. For more about distance
functions in Turi Create, including composite distances, please see the
[API documentation for the distances
module](https://apple.github.io/turicreate/docs/api/turicreate.toolkits.distances.html).
The end of this chapter describes how to use a composite distance with
the nearest neighbor classifier in particular.

Once the distance function is defined, the user must indicate the
criteria for deciding when training data are in the "neighborhood" of a
prediction point.  This is done by setting two constraints:

1. `radius` - the maximum distance a training example can be from the prediction
   point and still be considered a neighbor, and 

2. `max_neighbors` - the maximum number of neighbors for the prediction point.
   If there are more points within `radius` of the prediction point, the closest
   `max_neighbors` are used.

Unlike the other classifiers in the Turi Create classifier toolkit, the
nearest neighbors classifiers is an **instance-based** method, which means that
the model must store all of the training data. For each prediction, the model
must search all of the training data to find the neighbor points in the training
data. Turi Create performs this search intelligently, but predictions are
nevertheless typically slower than other classification models.


#### Basic Example

To illustrate basic usage of the nearest neighbor classifier, we again
use restaurant review data, with the goal of predicting how
many "stars" a user will give a particular business. Anticipating that
we will want to test the validity of the model, we first split the data
into training and testing subsets.

```python
import turicreate as tc

data =  tc.SFrame('ratings-data.csv')
train_data, test_data = data.random_split(0.9)
```

In this example we build a classifier using only the four numeric
features listed in the logistic regression chapter, namely the average
stars awarded by each user and to each business and the total count of
reviews given by each user and to each business. The review counts
features are typically much larger than the average stars features,
which would cause the review counts to dominate standard numeric
distance functions. To avoid this we standardize the features before
creating the model.

```python
numeric_features = ['user_avg_stars', 
                    'business_avg_stars', 
                    'user_review_count', 
                    'business_review_count']

for ftr in numeric_features:
    mean = train_data[ftr].mean()
    stdev = train_data[ftr].std()
    train_data[ftr] = (train_data[ftr] - mean) / stdev
    test_data[ftr] = (test_data[ftr] - mean) / stdev
```

Finally, we create the model and generate predictions.

```python
m = tc.nearest_neighbor_classifier.create(train_data, target='stars',
                                          features=numeric_features)
predictions = m.classify(test_data, max_neighbors=20, radius=None)
print(predictions)
```
```no-highlight
+-------+-------------+
| class | probability |
+-------+-------------+
|   4   |     0.65    |
|   4   |     0.5     |
|   5   |     0.8     |
|   5   |     0.65    |
|   5   |     1.0     |
|   4   |     0.55    |
|   4   |     0.35    |
|   3   |     0.45    |
|   4   |     0.45    |
|   5   |     0.4     |
+-------+-------------+
[21466 rows x 2 columns]
Note: Only the head of the SFrame is printed.
You can use print_rows(num_rows=m, num_columns=n) to print more rows and columns.
```

#### Advanced Usage

The `classify` method returns an SFrame with both the predicted class and the
probability score of that class, which is simply the fraction of the points'
neighbors which belong to the most common class. As with multiclass logistic
regression, the `predict_topk` method can be used to see the fraction of
neighbors belonging to every target class.

```python
topk = m.predict_topk(test_data[:5], max_neighbors=20, k=3)
print(topk)
```
```no-highlight
## -- End pasted text --
+--------+-------+-------------+
| row_id | class | probability |
+--------+-------+-------------+
|   0    |   5   |     0.7     |
|   0    |   4   |     0.3     |
|   3    |   4   |     0.45    |
|   3    |   5   |     0.3     |
|   3    |   3   |     0.2     |
|   1    |   5   |     0.8     |
|   1    |   4   |     0.15    |
|   1    |   3   |     0.05    |
|   2    |   4   |     0.6     |
|   2    |   5   |     0.25    |
+--------+-------+-------------+
[14 rows x 3 columns]
Note: Only the head of the SFrame is printed.
You can use print_rows(num_rows=m, num_columns=n) to print more rows and columns.
```

To get a sense of the model validity, pass the test data to the `evaluate`
method.

```python
evals = m.evaluate(test_data[:3000])
print(evals['accuracy'])
```
```no-highlight
0.46
```

46% accuracy seems low, but remember that we are in a multi-class
classification setting. The most common class (4 stars) only occurs in
34.8% of the test data, so our model has indeed learned something. The
confusion matrix produced by the `evaluate` method can help us to better
understand the model performance. In this case we see that 83.9% of our
predictions are actually within 1 star of the true number of stars.

```python
conf_matrix = evals['confusion_matrix']
conf_matrix['within_one'] = conf_matrix.apply(
    lambda x: abs(x['target_label'] - x['predicted_label']) <= 1)
num_within_one = conf_matrix[conf_matrix['within_one']]['count'].sum()
print(float(num_within_one) / len(test_data))
```
```no-highlight
0.8386693230783487
```

Suppose we want to add the `text` column as a feature. One way to do
this is to treat each entry as a "bag of words" by simply counting the
number of times each word appears but ignoring the order (please see the
text analytics chapter for more detail).

```python
train_data['word_counts'] = tc.text_analytics.count_words(train_data['text'],
                                                          to_lower=True)
test_data['word_counts'] = tc.text_analytics.count_words(test_data['text'],
                                                         to_lower=True)
```

For example, the (abbreviated) text of the first review in the training
set is:

> My wife took me here on my birthday for breakfast and it was excellent. The
> weather was perfect which made sitting outside overlooking their grounds an
> absolute pleasure....

while the (also abbreviated) bag-of-words representation is a dictionary
that maps each word to the number of times that word appears:

```python
{'2': 1, 'a': 1, 'absolute': 1, 'absolutely': 1, 'amazing': 2, 'an': 1,
'and': 8, 'anyway': 1, 'arrived': 1, 'back': 1, 'best': 2, 'better': 1,
'birthday': 1, 'blend': 1, 'bloody': 1, 'bread': 1, 'breakfast': 1, ... }
```

The `weighted_jaccard` distance measures the difference between two
sets, weighted by the counts of each element (please see the [API
documentation](https://apple.github.io/turicreate/docs/api/generated/turicreate.toolkits.distances.weighted_jaccard.html#turicreate.toolkits.distances.weighted_jaccard)
for details). To combine this output with the numeric distance we used
above, we specify a **composite distance**. Each element in this list
includes a list (or tuple) of feature names, a standard distance
function name, and a numeric weight. The weight on each component can be
adjusted to produce the same effect as normalizing features.

```python
my_dist = [
    [numeric_features, 'euclidean', 1.0],
    [['word_counts'], 'weighted_jaccard', 1.0]
    ]

m2 = tc.nearest_neighbor_classifier.create(train_data, target='stars',
                                          distance=my_dist)
accuracy = m2.evaluate(test_data[:3000], metric='accuracy')
print(accuracy)
```
```no-highlight
{'accuracy': 0.482}
```

Adding the text feature appears to slightly improve the accuracy of our
classifier. For more information, please see the following resources:
- [User Guide chapter on nearest neighbors search](https://apple.github.io/turicreate/docs/userguide/nearest_neighbors/nearest_neighbors.html)
- [API documentation on the nearest neighbor classifier](https://apple.github.io/turicreate/docs/api/generated/turicreate.nearest_neighbor_classifier.NearestNeighborClassifier.html)
- [API documentation on the distances module](https://apple.github.io/turicreate/docs/api/turicreate.toolkits.distances.html)
- [Wikipedia on the k-nearest neighbors algorithm](http://en.wikipedia.org/wiki/K-nearest_neighbors_algorithm)
