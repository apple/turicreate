# Deploying to Core ML

With the Core ML framework, you can use a machine learning model to
classify input data. Exporting this model in Core ML format can be
performed using the `export_coreml` function.

```python
model.export_coreml('MyCatDogClassifier.mlmodel')
```

When you open the model in Xcode, it looks like the following:

![Image classifier model in Xcode](images/image_classifier_model.png)

Through a simple drag and drop process, you can incorporate the model
into Xcode. The following Swift code is needed to consume the model in
an iOS app.

```swift
let model = try VNCoreMLModel(for: MyCustomImageClassifier().model)

let request = VNCoreMLRequest(model: model, completionHandler: { [weak self] request, error in
    self?.processClassifications(for: request, error: error)
})
request.imageCropAndScaleOption = .centerCrop
return request
```

Refer to the [Core ML sample application
](https://developer.apple.com/documentation/vision/classifying_images_with_vision_and_core_ml)
for more details on using image classifiers in Core ML and Vision
frameworks for iOS and macOS.
