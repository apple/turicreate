# Clustering

**Clustering** is the task of grouping data so that points in the same
cluster are highly similar to each other, while points in different
clusters are dissimilar. Clustering is a form of unsupervised learning
because there is no target variable indicating which groups the training
data belong to. The Turi Create clustering toolkit includes two models:
K-Means and DBSCAN.

**K-Means** finds cluster centers for a predetermined number of clusters
("K") by minimizing the sum of squared distances from each point to its
assigned cluster. Points are assigned to the cluster whose center is
closest. It is usually the faster of the two options, and can be
accelerated further by setting the `batch_size` parameter to use only a
small subset of data for each training iteration.

**DBSCAN** is the most popular probability density-based clustering
method. It creates clusters by connecting neighboring points that have
high estimated probability density. Although less computationally
efficient than K-Means, DBSCAN captures more flexible cluster shapes,
automatically determines the best number of clusters, and detects
outliers.
