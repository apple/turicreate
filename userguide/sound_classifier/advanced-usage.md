# Advanced Sound Classifier Usage


#### Reusing Deep Features
Often it is necessary to train more than one instance of a model on the same data. For example this is required by both [K-Folds Cross Validation](https://en.wikipedia.org/wiki/Cross-validation_(statistics)) and [Hyperparameter Tuning](https://en.wikipedia.org/wiki/Hyperparameter_optimization).

Much of the work across different training runs can be reused. Sound Classification is done using a [three stage process](how-it-works.md). The first two of these stages (signal preprocessing and VGGish feature extraction) will be identical and can be reused.

The code below use the ESC-10 dataset from the [Introductory Example](./README.md#introductory-example) to do K-Folds Cross Validation. In this example reusing the deep feature allow us to perform 5-fold cross validation more than twice as fast.
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


#### Tune Custom Neural Network Architecture
The [custom neural network](how-it-works.html#custom-neural-network-stage) is made up of a series of dense layers. Using more layers or more units in each layer can have a significant affect on accuracy. This will also affect the size of your model.

The `custom_layer_sizes` parameter allows you specify how many layers and the number of units in each layer. The default values for this parameter is `[100, 100]` which corresponds to two dense layers with a 100 units each.

Using a smaller number of overall units will result in a smaller model, potentially with minimal affects on accuracy. If you have a large amount of training data, you should get better accuracy with more units.

The code below tries several different network configurations and reports the validation accuracy for each one.
```python
import turicreate as tc

data = tc.load_sframe('./ESC-10')

# Calculate the deep features just once.
data['deep_features'] = tc.sound_classifier.get_deep_features(data['audio'])

# Try several different neural network configurations
models = []
network_configurations = ([100, 100], [100], [1000, 1000], [100, 100, 100])
for cur_hyper_parameter in network_configurations:
    cur_model = tc.sound_classifier.create(data, target='category',
                                             custom_layer_sizes=cur_hyper_parameter,
                                             feature='deep_features')
    models.append(cur_model)

# Report results
for m, p in zip(models, network_configurations):
    print("{}, {}".format(p, m.validation_accuracy))
```
