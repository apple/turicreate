# Custom Recommender Model for iOS Applications

The Turi Create Recommender Model is also available for use in your iOS/macOS
Applications as a custom model via exporting to Core ML. After creating your model in
Turi Create, you need to export your model to Core ML by leveraging the simple 
`export_coreml` API as follows:

```python
# assume my_recommender is the trained Turi Create Recommender Model
my_recommender.export_coreml("MyRecommender.mlmodel")
```

After you drag and drop the exported Core ML model in your iOS Application, it will  look something like this:
![Core ML Model Screenshot in Xcode](xcode-shot.png)

As you can see above, we need to give the model two inputs, `interactions` and `k`. 

Here is some sample code to demonstrate how to use your exported custom model in a Swift
iOS Application. 

* First, we identify the model we dragged and dropped and assign it to a variable. 

```swift
let model = MyRecommender();
```

* We now construct the two inputs we need to give to our model to get back some
predictions.

```swift
let interactions = NSMutableDictionary();
interactions[242] = 1.0; // this signifies that the user interacted with item 242.
let modelInput = MyRecommenderInput(interactions: interactions as! [Int64 : Double], k: 1);
```

* Now that we have created the input to our `MyRecommender` model, we call `prediction`
on it to look at the model predictions. 

```swift
guard let modelOutput = try? model.prediction(input: modelInput) else {
    fatalError("Unexpected runtime error.");
}
print(modelOutput.probabilities);
print(modelOutput.recommendations);

```

In your app, you would probably have more complex inputs to the model and also want to
perform more complicated tasks with the output you get from the model -- this is just a
walkthrough of what the pipeline looks like!
