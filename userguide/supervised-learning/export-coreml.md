# Deploying to Core ML

With Core ML, you can integrate machine learning models into your macOS,
iOS, watchOS, and tvOS app. Classification and regression models created
in Turi Create can be exported for use in Core ML. These include

* Classifiers
  * [Logistic regression](../supervised-learning/logistic-regression.md)
  * [Support vector machines (SVM) ](../supervised-learning/svm.md)
  * [Boosted Decision Trees](../supervised-learning/boosted_trees_classifier.md)
  * [Random Forests](../supervised-learning/random_forest_classifier.md)
  * [Decision Tree](../supervised-learning/decision_tree_classifier.md)
* Regression
  * [Linear regression](../supervised-learning/linear-regression.md)
  * [Boosted Decision Trees](../supervised-learning/boosted_trees_regression.md)
  * [Random Forests](../supervised-learning/boosted_trees_regression.md)
  * [Decision Tree](../supervised-learning/boosted_trees_regression.md)


Export models in Core ML format with the
`export_coreml` function.

```python
model.export_coreml('MyModel.mlmodel')
```
Once the model is exported, you can also edit model metadata (author,
license, and description) using
[coremltools](https://github.com/apple/coremltools). [This
example](https://apple.github.io/coremltools/generated/coremltools.models.MLModel.html#coremltools.models.MLModel)
contains more details on how to do so.

#### Using Core ML

Add the model to your Xcode project by dragging the model into the
project navigator.  You can see information about the model—including
the model type and its expected inputs and outputs—by opening the model
in Xcode.

![Sample model in Xcode](images/sample_mlmodel_screenshot.png)

Xcode also uses information about the model’s inputs and outputs to
automatically generate a custom programmatic interface to the model,
which you use to interact with the model in your code. You can load the
model with the following code:

```swift
let model = MyModel()
```

The inputs to the model in Core ML are extracted from the input feature
names in Turi Create.  For example, if you have a model in Turi Create
that requires 3 inputs, `bedroom`, `bath`, and `size`, the
resulting code you would need to use in Xcode to make a prediction is
the following:

```swift
guard let output = try? model.prediction(bedroom: 1.0, bath: 2.0, size: 1200) else {
    fatalError("Unexpected runtime error.")
}
```

Refer to the [Core ML sample
application](https://developer.apple.com/documentation/coreml/integrating_a_core_ml_model_into_your_app)
for more details on using classifiers and regressors in Core ML.
