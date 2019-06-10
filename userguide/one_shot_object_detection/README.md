# One-Shot Object Detection

One-Shot object detection is the task of learning to detect a set of categories from a handful of images. Unlike the Object Detector which requires many varied examples of objects in the real world, the One-Shot detector only requires a single canonical example of an object to train the classifier on and performs predictions for that category in the wild.

Given an image and a one-shot detector trained on category 'stop sign', the output prediction will look like:

![Stop Sign vector image](images/red_stop_sign.png)

![Stop Sign prediction image](images/stop_sign_prediction_sample.png)


#### Introductory Example

In this example, the goal is to localize where in the image instances of the category 'stop sign' are present. For this task we supply a single image of a stop sign as a starter image:


```import turicreate as tc

# Load the starter images
starter_images = tc.SFrame({'image':[tc.Image('stop_sign.png')],
                   'label':['stop_sign']})

# Load test images
test_data = tc.SFrame({'image':[tc.Image('stop_sign_photo1.png'), 
                                tc.Image('stop_sign_photo2.png')],
                        'label':['stop_sign', 'stop_sign']})

# Create a model                                      
model = tc.one_shot_object_detector.create(starter_images, 'label')

# Save predictions on the test set as a column in the SFrame
test_data['predictions'] = model.predict(test_data)

# Draw prediction bounding boxes on the test images
test_data['image_pred'] = tc.object_detector.util.draw_bounding_boxes(test_data['image'], test_data['predictions']) 

# Evaluate the model and save the results in a dictionary
metrics = model.evaluate(test_data)

# Save the model for later use in TuriCreate
model.save('mymodel.model')

# Export for use in Core ML
model.export_coreml('MyCustomOneShotDetector.mlmodel')
```

We can preview the augmented images generated and used by the model for training:  


```augmented_images = tc.one_shot_object_detector.util.preview_synthetic_training_data(starter_images, 'label')

augmented_images.explore()
```

[Explore augmented images](images/augmented_images_explore.png)

Some examples of synthetic training data generated:

[Sample augmented images](images/augmented_images_collage.png)

*Note: In order to view the exact augmented images used to train the model, the same seed should be used which was used to train the model.*


To visualize the predictions made on the test set:


```test_data['image_pred'] = tc.object_detector.util.draw_bounding_boxes(test_data['image'], test_data['predictions'])

test_data.explore()
```

[Prediction test data](images/test_data_explore.png)

Examples of test image predictions:

[Sample prediction image - 1](images/sample_prediction_image1.png)

[Sample prediction image - 2](images/sample_prediction_image2.png)


#### How it works

To begin training for one-shot object detector, a set of synthetic training data needs to be generated using the provided starter training image(s). These images are augmented with operations: horizontal flip, vertical flip, yaw, pitch, roll, blur, noise and lightening perturbations. These operations are applied on a set of background images. The background images may be provided by the user. If no background images are provided, a set of default background images are downloaded and used.

To learn more about the training implementation details, refer to the Object Detector: How it Works (https://apple.github.io/turicreate/docs/userguide/object_detection/how-it-works.html) chapter.


#### Deployment to CoreML

To learn more about deploying One-Shot to CoreML details, refer to the Object Detector: Deployment to Core ML (https://apple.github.io/turicreate/docs/userguide/object_detection/export-coreml.html) chapter.


#### Advanced Usage

The One-Shot Object Detectors allows using custom background images. To do so:


```import turicreate as tc

# Load the starter images
starter_images = tc.SFrame({'image':[tc.Image('stop_sign.png')],
                   'label':['stop_sign']})

# Load background images
my_backgrounds = tc.SArray('custom_backgrounds.sarray')

# Create a model using custom background images                                      
model = tc.one_shot_object_detector.create(starter_images, 'label', backgrounds = my_backgrounds)
```