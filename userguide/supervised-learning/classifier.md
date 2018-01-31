# Classification

Classification is a core task in machine learning. For instance, a
classifier could take an image and predict whether it is a cat or a dog.
The pieces of information fed to a classifier for each data point are
called features, and the category they belong to is a ‘target’ or
‘label’.  Typically, the classifier is given data points with both
features and labels, so that it can learn the correspondence between the
two. Later, the classifier is queried with a data point and the
classifier tries to predict what category it belongs to. A large group
of these query data-points constitute a prediction-set, and the
classifier is usually evaluated on its accuracy, or how many prediction
queries it gets correct.

There are many methods to perform classification, such as SVMs, logistic
regression, deep learning, and more. Creating classification models is
easy with Turi Create!

Currently, the following models are supported for classification:

* [Logistic regression](logistic-regression.md)
* [Nearest neighbor classifier](knn_classifier.md)
* [Support vector machines (SVM) ](svm.md)
* [Boosted Decision Trees](boosted_trees_classifier.md)
* [Random Forests](random_forest_classifier.md)
* [Decision Tree](decision_tree_classifier.md)
* [Image Classifier](../image_classifier/README.md)

These algorithms differ in how they make predictions, but conform to the same
API. With all models, call **create()** to create a model, **predict()** to make
flexible predictions on the returned model, **classify()** which provides
all the sufficient statistics for classifying data, and **evaluate()** to
measure performance of the predictions. Models can incorporate:

* Numeric features
* Categorical variables
* Dictionary features (i.e sparse features)
* List features (i.e dense arrays)
* Text data
* Images (using the image classifier)

#### Model Selector

It isn't always clear that we know exactly which model is suitable for a given
task.  Turi Create's model selector automatically picks the right model for
you based on statistics collected from the data set.

```python
import turicreate as tc

# Load the data
data =  tc.SFrame('ratings-data.csv')

# Restaurants with rating >=3 are good
data['is_good'] = data['stars'] >= 3

# Make a train-test split
train_data, test_data = data.random_split(0.8)

# Automatically picks the right model based on your data.
model = tc.classifier.create(train_data, target='is_good',
                             features = ['user_avg_stars',
                                         'business_avg_stars',
                                         'user_review_count',
                                         'business_review_count'])

# Generate predictions (class/probabilities etc.), contained in an SFrame.
predictions = model.classify(test_data)

# Evaluate the model, with the results stored in a dictionary
results = model.evaluate(test_data)
```

Turi Create implementations are built to work with up to billions of
examples and up to millions of features.
