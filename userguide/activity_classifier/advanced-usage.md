# Advanced Usage

In this section, we will cover the input data format for the activity classifier, as well as the different output formats available. For these purposes we will use the HAPT dataset from the [previous chapter](README.md).

#### Input Data Format

An activity classifier is created using data from different sensors collected at a certain frequency over a period of time. **The activity classifier in Turi Create expects all sensors to be sampled at the same frequency**. For example, in the HAPT dataset the sensors sampled were a 3-axial accelerometer, and a 3-axial gyroscope, resulting in 6 values (or features) per timestamp. Both sensors were sampled at 50Hz, i.e. 50 samples were collected per second. The figure below shows 3 seconds of walking data collected from a single subject in the HAPT experiment.

<img src="images/walking.png"></img>

The sensor sampling frequency depends on the activities being classified and various practical constraints. For example, if you are trying to detect minuscule motions such as a jerk of a finger you may require high sampling frequency, while a lower one may be required for detecting coarser motions such as swimming. Further considerations are device battery usage and model creation time. A high sampling frequency requires more calls to the different sensors. This results in higher battery drain and larger volumes of data, increasing model complexity and creation time.

Commonly, an application using an activity classifier would provide the user with predictions at a slower rate than the sensor sampling rate, depending on the activities. For example, counting steps could require a prediction every second, while for detecting sleep patterns a rate of once per minute is probably sufficient. While creating a model it is important to feed it with labels at the same rate as the desired predictions rate. The number of sensor samples associated with a single label is called the **prediction window**. The activity classifier uses the prediction window to determine the prediction frequency, i.e. producing a prediction after every ```prediction_window``` samples. For the HAPT dataset we use a ```prediction_window``` of 50, producing a prediction every second as the sensors were sampled at 50Hz.

Each set of consecutive samples produced from a single recording of a subject is called a **session**. A session can contain demonstrations of multiple activities. However, sessions aren't required to contain all activities or be of the same length. The input data to the activity classifier must contain a column to uniquely assign each sample to a session. **The activity classifier in Turi Create expects the samples associated with each session id to be in an ascending order by time**. More sessions from more subjects allow the activity classifier to generalize better to new sessions it has not seen before.

Below is an example of an input SFrame expected by the activity classifier, taken from the HAPT dataset. The example contains 2 sessions, distinguished by the ```exp_id``` column. In this example, the first session contains samples for walking only, while the second session contains samples for standing and sitting.

```no-highlight
+--------+----------+----------+-----------+----------+-----------+-----------+-----------+
| exp_id | activity |  acc_x   |   acc_y   |  acc_z   |   gyro_x  |   gyro_y  |   gyro_z  |
+--------+----------+----------+-----------+----------+-----------+-----------+-----------+
|   1    | walking  | 0.708333 | -0.197222 | 0.095833 | -0.751059 |  0.345444 |  0.038179 |
|   1    | walking  | 0.756944 | -0.173611 | 0.169444 | -0.545503 |  0.218995 |  0.046426 |
|   1    | walking  | 0.902778 | -0.169444 | 0.147222 | -0.465785 |  0.440128 | -0.045815 |
|   1    | walking  | 0.970833 | -0.183333 | 0.118056 | -0.357662 |  0.503964 | -0.206472 |
|   1    | walking  | 0.972222 | -0.176389 | 0.166667 | -0.312763 |  0.64263  | -0.309709 |
|   2    | standing | 1.036111 | -0.290278 | 0.130556 |  0.039095 | -0.021075 |  0.034208 |
|   2    | standing | 1.047222 | -0.252778 |   0.15   |  0.135612 |  0.015272 | -0.045815 |
|   2    | standing |  1.0375  | -0.209722 | 0.152778 |  0.171042 |  0.009468 | -0.094073 |
|   2    | standing | 1.026389 |  -0.1875  | 0.148611 |  0.210138 | -0.039706 | -0.094073 |
|   2    | sitting  | 1.013889 | -0.065278 | 0.127778 | -0.020464 | -0.142332 |  0.091324 |
|   2    | sitting  | 1.005556 | -0.058333 | 0.127778 | -0.059254 | -0.138972 |  0.055589 |
|   2    | sitting  |   1.0    | -0.070833 | 0.147222 | -0.058948 | -0.124922 |  0.026878 |
+--------+----------+----------+-----------+----------+-----------+-----------+-----------+
[12 rows x 8 columns]
```

In this example, if ```prediction_window``` is set to 2, then every 2 rows within a session will be considered for a single prediction. Less than ```prediction_window``` rows at the end of a session also produce a single prediction. A ```prediction_window``` of 2 will produce 3 predictions for ```exp_id``` 1 and 4 predictions for ```exp_id``` 2, while a ```prediction_window``` of 5 will produce a single prediction for ```exp_id``` 1 and 2 predictions for ```exp_id``` 2.

#### Prediction Frequency

Remember the prediction frequency of the activity classifier is determined by the ```prediction_window``` parameter. Thus, a prediction is produced for every ```prediction_window``` rows in a session. For the above example from the HAPT dataset, setting the prediction window to 50 will produce a single prediction per 50 samples, which corresponds to a prediction per second.

```python
model.predict(walking_3_sec, output_frequency='per_window')
```
```no-highlight
+---------------+--------+---------+
| prediction_id | exp_id |  class  |
+---------------+--------+---------+
|       0       |   1    | walking |
|       1       |   1    | walking |
|       2       |   1    | walking |
+---------------+--------+---------+
[3 rows x 3 columns]
```

However, in many machine learning workflows it is common to use predictions from one model as input for further analysis or modeling. In this case it may be more beneficial to return a prediction per row of the input data. We can require the model to do so by setting the ```output_frequency``` parameter in ```predict``` to 'per_row'.

```python
model.predict(walking_3_sec, output_frequency='per_row')
```
```no-highlight
dtype: str
Rows: 150
['walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', 'walking', ... ]
```

These predictions are produced by duplicating each prediction per ```prediction_window``` across all samples associated with said window.


#### Using GPUs

GPUs can make creating an activity classifier model much faster. If you have
macOS 10.14 or higher, Turi Create will automatically use an available discrete
GPU. (Integrated Intel GPUs are not supported.) If your Linux machine has an
NVIDIA GPU, you can setup Turi Create to use the GPU,
[see instructions](https://github.com/apple/turicreate/blob/master/LinuxGPU.md).

The `turicreate.config.set_num_gpus` function allows you to control if GPUs are used:
```python
# Use all GPUs (default)
turicreate.config.set_num_gpus(-1)

# Use only 1 GPU
turicreate.config.set_num_gpus(1)

# Use CPU
turicreate.config.set_num_gpus(0)
```

