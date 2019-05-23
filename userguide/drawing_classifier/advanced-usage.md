# Advanced Usage

### Advanced Parameters during Training

Note that you can change how long the model trains for by tweaking either 
the `num_epochs` or the `max_iterations` parameters during training, 
passed into `turicreate.drawing_classifier.create`. Here are a few optional 
parameters that you can pass to `.create` that can change how you train models:

* `batch size` is the number of training examples in one forward/backward pass.
The larger the batch size, the more memory you would need during training.
* `num_epochs` is the number of forward/backward passes of *all* the 
training examples.
* `max_iterations` is the number of passes, each pass using a `batch_size` 
number of examples. 

If you specify both `max_iterations` and `num_epochs`, `max_iterations` would
take precedence and the model would train for that number of iterations and the 
provided value of `num_epochs` would be ignored.

### Warm Start

To boost the accuracy of the Drawing Classifier models you train, and to help
those models converge faster, we provide the option of loading in a 
pretrained model for a warm start.

A cold start to training would be when the weights are randomly initialized. 
A warm start is when the weights in the neural network are loaded from those 
of an already trained model. This improves the initial values of the weights 
in the network to something better than random, thereby improving accuracy 
and also helping the model converge faster.

We have published a pre-trained model that automatically gets downloaded when you pass in 
`auto` to the `warm_start` parameter. This model is trained on millions of drawings from 
the “Quick,Draw!” dataset