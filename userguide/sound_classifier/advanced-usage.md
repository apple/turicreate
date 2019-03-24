# Advanced Sound Classifier Usage


#### Reusing Deep Features
Often it is necessary to train more than one instance of a model on the same data. For example this is required by both [K-Folds Cross Validataion](https://en.wikipedia.org/wiki/Cross-validation_(statistics)) and [Hyperparameter Tuning](https://en.wikipedia.org/wiki/Hyperparameter_optimization).

Much of the work accross different training runs can be reused. Sound Classification is done using a [three stage process](how-it-works.md). The first two of these stages (signal preprocessing and VGGish feature extraction) will be identical and can be reused.

The code below use the ESC-10 dataset from the [Introductory Example](./README.md#introductory-example) to do K-Folds Cross Validataion. In this example reusing the deep feature allow us to perform 5-fold cross validation more than twice as fast.
```python
import turicreate as tc

data = tc.load_sframe('./ESC-10')

# Calculate the deep features just once.
data['deep_features'] = tc.sound_classifier.get_deep_features(data['audio'])

accuracies = []
for cur_fold in data['fold'].unique():
    test_set = data.filter_by(cur_fold, 'fold')
    train_set = data.filter_by(cur_fold, 'fold', exclude=True)

    model = tc.sound_classifier.create(train_set, target='category', feature='deep_features')

    metrics = model.evaluate(test_set)
    accuracies.append(metrics['accuracy'])

print("Accuracies: {}".format(accuracies))
```
