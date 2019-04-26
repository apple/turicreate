# Sound Classifier

Given a sound, the goal of the Sound Classifier is to assign it to
one of a pre-determined number of labels, such as baby crying, siren,
or dog barking. This Sound Classifier is not intended to be used for
speech recognition.

#### Loading Data

For example purposes, we will use the ESC-10 dataset. This dataset
contain ten classes. Each class has 40 examples with five seconds of
audio per example.

**Please note:** the ESC-10 dataset is part of a larger [ESC-50 dataset](https://github.com/karoldvl/ESC-50)
dataset. In order to use the ESC-10, you will need to download the
[ESC-50 dataset](https://github.com/karoldvl/ESC-50)[<sup>1</sup>](../datasets.md). These two datasets have different licenses.
Depending on your use case you may be able to use the larger ESC-50
dataset, see their [license section](https://github.com/karoldvl/ESC-50#license) for details.

#### Introductory Example
```python
import turicreate as tc
from os.path import basename

# Load the audio data and meta data.
data = tc.load_audio('./ESC-50/audio/')
meta_data = tc.SFrame.read_csv('./ESC-50/meta/esc50.csv')

# Join the audio data and the meta data.
data['filename'] = data['path'].apply(lambda p: basename(p))
data = data.join(meta_data)

# Drop all records which are not part of the ESC-10.
data = data.filter_by('True', 'esc10')

# Make a train-test split, just use the first fold as our test set.
test_set = data.filter_by(1, 'fold')
train_set = data.filter_by(1, 'fold', exclude=True)

# Create the model.
model = tc.sound_classifier.create(train_set, target='category', feature='audio')

# Generate an SArray of predictions from the test set.
predictions = model.predict(test_set)

# Evaluate the model and print the results
metrics = model.evaluate(test_set)
print(metrics)

# Save the model for later use in Turi Create
model.save('mymodel.model')

# Export for use in Core ML
model.export_coreml('mymodel.mlmodel')
```
