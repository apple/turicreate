# On-Device Model Personalization API Usage

This document explains how one could use CoreML 3's update task APIs to personalize updatable MNIST digit classifier model on-device. A python notebook describing the steps for creating an updatable neural network model can be found [here](https://github.com/apple/coremltools/blob/master/examples/updatable_models/updatable_mnist.ipynb).

Let us start by creating an inference-only instance of the `MLModel` using the auto-generated `UpdatableMNISTDigitClassifier` class. Xcode generates this class as soon as the CoreML model is added to the project.

```swift
let digitClassifier = UpdatableMNISTDigitClassifier()
```

Next, we'll define a utility method that reads sample images from the app bundle and creates training data that conforms to `MLBatchProvider`.

```swift
func createTrainingBatchProvider() throws -> MLBatchProvider {

    let trainingSamples = [MLFeatureProvider]()

    // sample images are assumed to be bundled with the app and named as "<TrueClassLabel>_<ImageIndex>.png"
    // we use 10 images per class for the purpose of training the MNIST model

    // iterate over all class labels
    for trueClassLabel in ["0", "1", "2", "3", "4", "5", "6", "7", "8", "9"] {

        // iterate over 10 images per class
        for imageIndex in 0 ..< 10 {

            // access image URL from the app bundle
            let imageURL = Bundle.main.url(forResource: "\(trueClassLabel)_\(imageIndex)",
			                               withExtension: "png")!

            // create a CVPixelBuffer containing the image used for training
            let imageBuffer = try MLFeatureValue(imageAt: imageURL,

			                                     pixelsWide: 28,
			                                     pixelsHigh: 28,
			                                     pixelFormatType: kCVPixelFormatType_OneComponent8,
			                                     options: nil).imageBufferValue!

            // create a training sample as a MLFeatureProvider
            let trainingSample = UpdatableMNISTDigitClassifierTrainingInput(image: imageBuffer,
                                                                            digit: trueClassLabel)

            // and, hold on to the training sample
            trainingSamples.append(trainingSample)
        }
    }

    // return training samples as a MLBatchProvider
    return MLArrayBatchProvider(array: trainingSamples)
}
```

We'll define a progress handler block that gets invoked during the training for all specified events. If necessary, we could update the `digitClassifier` model to use the updated model before running predictions to compute accuracy of the updated model so far.

```swift
func progressHandler(_ context: MLUpdateContext) {

    // replace the underlying MLModel instance with updated model from context
    digitClassifier.model = context.model

    // updated digitClassifier now can be used to monitor training by computing prediction accuracy on a test set (usually different from the training set)
}
```

We'll define a completion handler block which gets invoked when the training is successful or when it fails with an error. Similar to the progress handler, we could compute prediction accuracy with the updated model from completion handler. We could also save this model to disk for later use.

```swift
func completionHandler(_ context: MLUpdateContext) {

    if (context.task.error != nil) {
        // handle error and return from completionHandler
        return
    }

    // replace the underlying MLModel instance with updated model from context
    digitClassifier.model = context.model

    // updated digitClassifier now can be used to compute final prediction accuracy on a validation set (usually different from the training set)

    do {
        // obtain a URL to a writable location on disk to save the updated compiled model (.modelc)
        let updatedModelURL = URL(fileURLWithPath: "<NEW_PATH_TO_WRITABLE_MODELC_LOCATION>")

        // save the updated model to disk
        try context.model.write(to: updatedModelURL)
    }
    catch {
        // handle error while trying to save the model to disk
        return
    }
}
```

Once the handlers have been defined, we'll collect necessary information in order to kick off the training process.

* Get the updatable model URL from the app bundle:

```swift
let updatableModelURL = Bundle.main.url(forResource: "UpdatableMNISTDigitClassifier",
                                        withExtension: "mlmodelc")!
```

* Create the training data from samples images that were bundled with the app:

```swift
let trainingData = try createTrainingBatchProvider()
```

* Create an instance of `MLModelConfiguration` and set any parameters (if required):

```swift
let configuration = MLModelConfiguration()

// requests training loop run for 10 iterations and use 0.02 learning rate
configuration.parameters = [MLParameterKey.epochs : 10, MLParameterKey.learningRate : 0.02]
```

* Set up progressHandlers by specifying epoch end as an interested event:

```swift
let progressHandlers = MLUpdateProgressHandlers(forEvents: .epochEnd,
                                                progressHandler: progressHandler,
                                                completionHandler: completionHandler)
```

* Lastly, setup update task to update the model at location `updatableModelURL` with `trainingData` and model `configuration` that specifies number of epochs as 10 and learning rate as 0.02. `trainingData` contains all samples in the training set. It conforms to `MLBatchProvider` and can be built using an array of `UpdatableMNISTDigitClassifierTrainingInput` instances.

```swift
let updateTask = try MLUpdateTask(forModelAt: updatableModelURL,
                                  trainingData: trainingData,
                                  configuration: configuration,
                                  progressHandlers: progressHandlers)

// start the update process
updateTask.resume()
```
