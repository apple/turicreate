# How does this work?

The underlying model of the activity classifier in Turi Create is a deep learning model capable of detecting temporal patterns in sensor data, lending itself well to the task of activity classification. The deep learning model relies on [convolutional layers](https://en.wikipedia.org/wiki/Convolutional_neural_network) to extract temporal features from a single prediction window, for example an arching movement could possibly be a strong indicator of swimming. Furthermore, it relies on [recurrent layers](https://en.wikipedia.org/wiki/Recurrent_neural_network) to extract temporal features over time, for example if a subject was swimming in the previous timestamp, then it is most likely not sky diving in the next. Below is a sketch of the neural network used for the activity classifier in Turi Create.

<img src="images/activity_classifier_network.png"></img>

A single input to the neural network is a session as defined in the previous section. The convolutional layer operates on each prediction window, finding spatial features that may be relevant to the labeled activities. Returning to our 3-seconds walking session from the previous section, a convolutional layer may determine the following "spike" as a feature relevant to walking:

<img src="images/convolutional_filter.png"></img>

The output of the convolutional layer is a vector representation for each prediction window, encoding these learnt features. The recurrent layer takes as input a sequence of these vectors. 

The recurrent layer is specialized for learning temporal features across sequences. For example it may learn that spatial features associated with walking are more likely to occur after detecting spatial features associated with running. These features are further encoded into the output of the recurrent layer.

In order to detect these features along sessions the recurrent layer takes into account its own **state** - the output of the recurrent layer for the previous prediction window. The output of the recurrent layer for the current prediction window is turned into a probability vector across all desired activities to produce the final classification.

More about this type of deep learning architecture can be found in [Deep Convolutional and LSTM Recurrent Neural Networks for Multimodal Wearable Activity Recognition](http://www.mdpi.com/1424-8220/16/1/115).