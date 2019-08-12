# HAPT Data Preparation

In this section we will see how to get the [HAPT experiment](http://archive.ics.uci.edu/ml/datasets/Smartphone-Based+Recognition+of+Human+Activities+and+Postural+Transitions) data into the SFrame format expected by the activity classifier.[<sup>1</sup>](../datasets.md)

First we need to download the data from [here](http://archive.ics.uci.edu/ml/machine-learning-databases/00341/HAPT%20Data%20Set.zip) in zip format. The code below assumes the data was unzipped into a directory named `HAPT Data Set`. This folder contains 3 types of files - a file containing the performed activities for each experiment, files containing the collected accelerometer samples, and files containing the collected gyroscope samples.

The first file is `labels.txt`, which contains activities performed for each experiment. The labels are specified by sample index ranges. For example, in experiment 1 the subject was performing activity number 5 between the 250th collected sample and the 1232th collected sample. The activities are encoded between numbers 1 and 6. We convert these to strings at the end of this section. The code below imports Turi Create, loads `labels.txt` into an SFrame, and defines a function to find the label given a sample index.

```python
import turicreate as tc

data_dir = './HAPT Data Set/RawData/'

def find_label_for_containing_interval(intervals, index):
    containing_interval = intervals[:, 0][(intervals[:, 1] <= index) & (index <= intervals[:, 2])]
    if len(containing_interval) == 1:
        return containing_interval[0]

# Load labels
labels = tc.SFrame.read_csv(data_dir + 'labels.txt', delimiter=' ', header=False,
                            verbose=False)
labels = labels.rename({'X1': 'exp_id', 'X2': 'user_id', 'X3': 'activity_id',
                        'X4': 'start', 'X5': 'end'})
labels
```

```no-highlight
+--------+---------+-------------+-------+------+
| exp_id | user_id | activity_id | start | end  |
+--------+---------+-------------+-------+------+
|   1    |    1    |      5      |  250  | 1232 |
|   1    |    1    |      7      |  1233 | 1392 |
|   1    |    1    |      4      |  1393 | 2194 |
|   1    |    1    |      8      |  2195 | 2359 |
|   1    |    1    |      5      |  2360 | 3374 |
|   1    |    1    |      11     |  3375 | 3662 |
|   1    |    1    |      6      |  3663 | 4538 |
|   1    |    1    |      10     |  4539 | 4735 |
|   1    |    1    |      4      |  4736 | 5667 |
|   1    |    1    |      9      |  5668 | 5859 |
+--------+---------+-------------+-------+------+
[1214 rows x 5 columns]
```

Next, we need to get the accelerometer and gyroscope data for each experiment. For each experiment, every sensor's data is in a separate file. In the code below we load the accelerometer and gyroscope data from all experiments into a single SFrame. While loading the collected samples, we also calculate the label for each sample using our previously defined function. The final SFrame contains a column named `exp_id` to identify each unique sessions. 

```python
from glob import glob

acc_files = glob(data_dir + 'acc_*.txt')
gyro_files = glob(data_dir + 'gyro_*.txt')

# Load data
data = tc.SFrame()
files = zip(sorted(acc_files), sorted(gyro_files))
for acc_file, gyro_file in files:
    exp_id = int(acc_file.split('_')[1][-2:])
    
    # Load accel data
    sf = tc.SFrame.read_csv(acc_file, delimiter=' ', header=False, verbose=False)
    sf = sf.rename({'X1': 'acc_x', 'X2': 'acc_y', 'X3': 'acc_z'})
    sf['exp_id'] = exp_id
    
    # Load gyro data
    gyro_sf = tc.SFrame.read_csv(gyro_file, delimiter=' ', header=False, verbose=False)
    gyro_sf = gyro_sf.rename({'X1': 'gyro_x', 'X2': 'gyro_y', 'X3': 'gyro_z'})
    sf = sf.add_columns(gyro_sf)
    
    # Calc labels
    exp_labels = labels[labels['exp_id'] == exp_id][['activity_id', 'start', 'end']].to_numpy()
    sf = sf.add_row_number()
    sf['activity_id'] = sf['id'].apply(lambda x: find_label_for_containing_interval(exp_labels, x))
    sf = sf.remove_columns(['id'])
    
    data = data.append(sf)
```

Finally, we encode the labels back into a readable string format, and save the resulting SFrame.

```python
target_map = {
    1.: 'walking',          
    2.: 'climbing_upstairs',
    3.: 'climbing_downstairs',
    4.: 'sitting',
    5.: 'standing',
    6.: 'laying'
}

# Use the same labels used in the experiment
data = data.filter_by(list(target_map.keys()), 'activity_id')
data['activity'] = data['activity_id'].apply(lambda x: target_map[x])
data = data.remove_column('activity_id')

data.save('hapt_data.sframe')
```

To learn more about the expected input format of the activity classifier please visit the [advanced usage](advanced-usage.md) section.
