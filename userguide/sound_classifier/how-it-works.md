# How Does this Work?

Training and making predictions for a sound classifier model is a three
stage process:
1 - Signal preprocessing
2 - A pretrained neural network is used to extract deep features
3 - A custom neural network is used to make the predictions
Details below about each stage.

## Signal Preprocessing Pipeline Stage
Several audio transformations happen during this stage. For someone without
sound domain knowledge, this is probably the most complicated part.
Nothing about this stage is updated based on the input data (i.e. nothing
is learned in this stage).

At a high level, the preprocessing pipeline does the following:
* The raw pulse code modulation data from the wav file is converted to
floats on a [-1.0, +1.0] scale.
* If there are two channels, the elements are averaged to produce one channel.
* The data is resampled to only 16,000 samples per second.
* The data is broken up into several overlapping windows.
* A [Hamming Window](https://en.wikipedia.org/wiki/Window_function#Hann_and_Hamming_windows) is applied to each windows.
* The [Power Spectrum](https://en.wikipedia.org/wiki/Spectral_density#Power_spectral_density) is calculated, using a [Fast Fourier Transformation](https://en.wikipedia.org/wiki/Fourier_transform).
* Frequencies above and below certain thresholds are dropped.
* [Mel Frequency](https://en.wikipedia.org/wiki/Mel_scale) [Filter Banks](https://en.wikipedia.org/wiki/Filter_bank) are applied.
* Finally the natural logarithm is taken of all values.

The preprocessing pipeline takes 975ms worth of audio as input (exact
input length depends on sample rate) and produces an array of shape
(96, 64).

## VGGish Feature Extraction Stage
VGGish is a pretrained [Convolutional Neural Network](https://en.wikipedia.org/wiki/Convolutional_neural_network) from Google,
see [their paper](https://ai.google/research/pubs/pub45611) and [their GitHub page](https://github.com/tensorflow/models/tree/master/research/audioset) for more details. As the name suggests, the architecture of
this network is inspired by the famous VGG networks used for image
classification. The network consists of a series of convolution and
activation layers, optionally followed by a max pooling layer.
This network contains 17 layers in total.

This network is kept static during model training. We have removed the
last three layers of the original VGGish model. We use the widest
layer, from the original network, as our input data for the final
stage. This modified VGGish model outputs a double vector of length
12,288. On non-Linux systems, the model has also been eight bit
quantized, to reduce its size.

## Custom Neural Network Stage
This is the only stage which is updated based on the input data.
During training of a sound classifier this model is trained using
the features from VGGish and the input labels. During prediction,
only a forward pass is done using this network. This custom neural
network is a simple three layer neural network. The first two
layers are dense layers, with 100 units each. These layers use [RELU activation](https://en.wikipedia.org/wiki/Rectifier_(neural_networks)).
The final layer is a softmax. The number of units in this layer is
equal to the number of labels.
