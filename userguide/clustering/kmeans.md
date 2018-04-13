#K-means

**K-means** finds cluster centers for a predetermined number of clusters ("K")
by minimizing the sum of squared distances from each point to its assigned
cluster. Points are assigned to the cluster whose center is closest.

Lloyd's algorithm is the standard way to compute K-means clusters, and it
describes the essential intuition for the method. After initial centers are
chosen, two steps repeat until the cluster assignment no longer changes for any
point (which is equivalent to the cluster centers no longer moving):

1. Assign each point to the cluster with the closest center.
2. Update each cluster center to the be mean of the assigned points.

The Turi Create implementation of K-means uses several wrinkles to improve
the speed of the method and quality of the results. Initial cluster centers are
chosen with the K-means++ algorithm (if not provided explicitly by the user).
This algorithm chooses cluster centers that are far apart with high probability,
which tends to reduce the number of iterations needed for convergence, and make
it less likely that the method returns sub-optimal results.

In addition, Turi Create's K-means uses the triangle inequality to reduce
the number of exact distances that need to be computed in each iteration.
Conceptually, if we know that a data point $$x$$ is close to center $$A$$, which
is in turn far from center $$B$$, then there is no need to compute the exact
distance from point $$x$$ to center $$B$$ when assigning $$x$$ to a cluster.


#### Basic Usage

We illustrate usage of Turi Create K-means with the dataset from the [June
2014 Kaggle competition to classify schizophrenic subjects based on MRI
scans](https://www.kaggle.com/c/mlsp-2014-mri). Download **Train.zip** from the data tab.[<sup>1</sup>](../datasets.md) The original data consists of
two sets of features: functional network connectivity (FNC) features and
source-based morphometry (SBM) features, which we incorporate into a single
[`SFrame`](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.html)
with [`SFrame.join`](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.join.html).

```python
import turicreate as tc

sf_functional = tc.SFrame.read_csv('train_FNC.csv')
sf_morphometry = tc.SFrame.read_csv('train_SBM.csv')

sf = sf_functional.join(sf_morphometry, on="Id")
sf = sf.remove_column('Id')

sf.save('schizophrenia_clean.sframe')
```

The most basic usage of K-means clustering requires only a choice for the number
of clusters, $$K$$. We rarely know the correct number of clusters *a priori*,
but the following simple heuristic sometimes works well:

$$
    K \approx \sqrt{n/2}
$$

where $$n$$ is the number of rows in your dataset. By default, the maximum
number of iterations is 10, and all features in the input dataset are used.

Analogous to all other Turi Create toolkits the model is created through the [`kmeans.create`](https://apple.github.io/turicreate/docs/api/generated/turicreate.kmeans.create.html) API:

```python
from math import sqrt
K = int(sqrt(sf.num_rows() / 2.0))

kmeans_model = tc.kmeans.create(sf, num_clusters=K)
kmeans_model.summary()
```
```no-highlight
Class                           : KmeansModel

Schema
------
Number of clusters              : 6
Number of examples              : 86
Number of feature columns       : 410
Number of unpacked features     : 410
Row label name                  : row_id

Training Summary
----------------
Training method                 : elkan
Number of training iterations   : 2
Batch size                      : 86
Total training time (seconds)   : 0.2836

Accessible fields               :
   cluster_id                   : An SFrame containing the cluster assignments.
   cluster_info                 : An SFrame containing the cluster centers.
```

The model summary shows the usual fields about model schema, training
time, and training iterations. It also shows that the K-means results
are returned in two SFrames contained in the model: `cluster_id` and
`cluster_info`. The `cluster_info` SFrame indicates the final cluster
centers, one per row, in terms of the same features used to create the
model.

```python
kmeans_model.cluster_info.print_rows(num_columns=5, max_row_width=80,
                                        max_column_width=10)
```
```no-highlight
+-----------+-----------+-----------+------------+-----------+-----+
|    FNC1   |    FNC2   |    FNC3   |    FNC4    |    FNC5   | ... |
+-----------+-----------+-----------+------------+-----------+-----+
| 0.1870... | 0.0801... | -0.092... | -0.0957298 | 0.0893... | ... |
|  0.21752  | 0.0363... | -0.027... | -0.063...  | 0.0556... | ... |
| 0.2293... | 0.1017... | -0.046... | -0.051...  | 0.2313... | ... |
| 0.1654... | -0.156... | -0.327... | -0.278...  | -0.033... | ... |
| 0.2549... |  0.02532  | 0.0081... | -0.134...  | 0.3875... | ... |
| 0.1072... | 0.0754... | -0.119422 | -0.312...  | 0.1100... | ... |
+-----------+-----------+-----------+------------+-----------+-----+
[6 rows x 413 columns]
```

The last three columns of the `cluster_info` SFrame indicate metadata about the
corresponding cluster: ID number, number of points in the cluster, and the
within-cluster sum of squared distances to the center.

```python
kmeans_model.cluster_info[['cluster_id', 'size', 'sum_squared_distance']]
```
```no-highlight
+------------+------+----------------------+
| cluster_id | size | sum_squared_distance |
+------------+------+----------------------+
|     0      |  7   |     340.44890213     |
|     1      |  11  |    533.886421204     |
|     2      |  49  |    2713.56332016     |
|     3      |  13  |     714.04801178     |
|     4      |  3   |    177.421077728     |
|     5      |  3   |     151.59986496     |
+------------+------+----------------------+
[6 rows x 3 columns]
```

The `cluster_id` field of the model shows the cluster assignment for each input
data point, along with the Euclidean distance from the point to its assigned
cluster's center.

```python
kmeans_model.cluster_id.head()
```
```no-highlight
+--------+------------+---------------+
| row_id | cluster_id |    distance   |
+--------+------------+---------------+
|   0    |     3      | 6.52821207047 |
|   1    |     2      | 6.45124673843 |
|   2    |     2      | 7.58535766602 |
|   3    |     2      | 7.64395523071 |
|   4    |     3      | 7.42247104645 |
|   5    |     2      | 8.29837036133 |
|   6    |     4      | 7.61347103119 |
|   7    |     2      | 6.98522281647 |
|   8    |     2      | 8.56831073761 |
|   9    |     0      | 7.91477823257 |
+--------+------------+---------------+
[10 rows x 3 columns]
```


#### Assigning *New* Points to Clusters

New data points can be assigned to the clusters of a K-means model with
the
[`KmeansModel.predict`](https://apple.github.io/turicreate/docs/api/generated/turicreate.kmeans.KmeansModel.predict.html)
method. For K-means, the assignment is simply the nearest cluster center
(in Euclidean distance), which is how the training data are assigned as
well. Note that the model's cluster centers *are not updated* by the
`predict` method.

For illustration purposes, we predict the cluster assignments for the
first 5 rows of our existing data. The assigned clusters are identical
to the assignments in the model results (above), which is a good sanity
check.

```python
new_clusters = kmeans_model.predict(sf[:5])
new_clusters
```
```no-highlight
dtype: int
Rows: 5
[3, 2, 2, 2, 3]
```


#### Advanced Usage

For large datasets K-means clustering can be a time-consuming method. One
simple way to reduce the computation time is to reduce the number of training
iterations with the `max_iterations` parameter. The model prints a warning
during training to indicate that the algorithm stops before convergence is
reached.

```python
kmeans_model = tc.kmeans.create(sf, num_clusters=K, max_iterations=1)
```
```no-highlight
PROGRESS: WARNING: Clustering did not converge within max_iterations.
```

It can also save time to set the initial centers manually, rather than having
the tool choose the initial centers automatically. These initial centers can be
chosen randomly from a sample of the original dataset, then passed to the final
K-means model.

```python
kmeans_sample = tc.kmeans.create(sf.sample(0.2), num_clusters=K,
                                 max_iterations=0)

my_centers = kmeans_sample.cluster_info
my_centers = my_centers.remove_columns(['cluster_id', 'size',
                                        'sum_squared_distance'])

kmeans_model = tc.kmeans.create(sf, initial_centers=my_centers)
```

For really large datasets, the tips above may not be enough to get results in a
reasonable amount of time; in this case, we can switch to **minibatch K-means**,
using the same Turi Create model. The `batch_size` parameter indicates how
many randomly sampled points to use in each training iteration when updating
cluster centers. Somewhat counter-intuitively, the results for minibatch K-means
tend to be very similar to the exact algorithm, despite typically using only a
small fraction of the training data in each iteration. Note that for the
minibatch version of K-means, the model will always compute a number of
iterations equal to `max_iterations`; it does not stop early.

```python
kmeans_model = tc.kmeans.create(sf, num_clusters=K, batch_size=30,
                                max_iterations=10)
kmeans_model.summary()
```
```no-highlight
Class                           : KmeansModel

Schema
------
Number of clusters              : 6
Number of examples              : 86
Number of feature columns       : 410
Number of unpacked features     : 410
Row label name                  : row_id

Training Summary
----------------
Training method                 : minibatch
Number of training iterations   : 10
Batch size                      : 30
Total training time (seconds)   : 0.3387

Accessible fields               :
   cluster_id                   : An SFrame containing the cluster assignments.
   cluster_info                 : An SFrame containing the cluster centers.
```

The model summary shows the training method here is "minibatch" with our
specified batch size of 30, unlike the previous model which used the "elkan"
(exact) method with a batch size of 86 - the total number of examples in our
dataset.


#### References and more information

- [Wikipedia - k-means
  clustering](http://en.wikipedia.org/wiki/K-means_clustering>)

- Artuhur, D. and Vassilvitskii, S. (2007) [k-means++: The Advantages of Careful
  Seeding](http://ilpubs.stanford.edu:8090/778/1/2006-13.pdf). In Proceedings of
  the Eighteenth Annual ACM-SIAM Symposium on Discrete Algorithms. pp.
  1027-1035.

- Elkan, C. (2003) [Using the triangle inequality to accelerate k-means]
  (http://www.aaai.org/Papers/ICML/2003/ICML03-022.pdf). In Proceedings of the
  Twentieth International Conference on Machine Learning, Volume 3, pp. 147-153.

- Sculley, D. (2010) [Web Scale K-Means
  Clustering](http://www.eecs.tufts.edu/~dsculley/papers/fastkmeans.pdf). In
  Proceedings of the 19th International Conference on World Wide Web. pp.
  1177-1178
