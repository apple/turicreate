# Recommender systems

A recommender system allows you to provide personalized recommendations
to users. With this toolkit, you can create a model based on past
interaction data and use that model to make recommendations.

#### Input data

Creating a recommender model typically requires a data set to use for
training the model, with columns that contain the user IDs, the item
IDs, and (optionally) the ratings. For this example, we use the [MovieLense
 dataset](https://grouplens.org/datasets/movielens/).

```python
import turicreate as tc
actions = tc.SFrame.read_csv('./dataset/ml-20m/ratings.csv')
```
```no-highlight
+--------+---------+--------+------------+
| userId | movieId | rating | timestamp  |
+--------+---------+--------+------------+
|   1    |    2    |  3.5   | 1112486027 |
|   1    |    29   |  3.5   | 1112484676 |
|   1    |    32   |  3.5   | 1112484819 |
|   1    |    47   |  3.5   | 1112484727 |
|   1    |    50   |  3.5   | 1112484580 |
|   1    |   112   |  3.5   | 1094785740 |
|   1    |   151   |  4.0   | 1094785734 |
|   1    |   223   |  4.0   | 1112485573 |
|   1    |   253   |  4.0   | 1112484940 |
|   1    |   260   |  4.0   | 1112484826 |
+--------+---------+--------+------------+
```

For information on how to load data into an SFrame from other sources,
see the chapter on [SFrames](../sframe/sframe-intro.md).

You may have additional data about users or items. For example we might
have a dataset of movie metadata.

```python
items = tc.SFrame.read_csv('./dataset/ml-20m/movies.csv')
```
```no-highlight
+---------+---------------------+---------------------+------+
| movieId |        title        |        genres       | year |
+---------+---------------------+---------------------+------+
|    1    |      Toy Story      | [Adventure, Anim... | 1995 |
|    2    |       Jumanji       | [Adventure, Chil... | 1995 |
|    3    |   Grumpier Old Men  |  [Comedy, Romance]  | 1995 |
|    4    |  Waiting to Exhale  | [Comedy, Drama, ... | 1995 |
|    5    | Father of the Br... |       [Comedy]      | 1995 |
+---------+---------------------+---------------------+------+
```

If you have data like this associated with each item, you can build a
model from just this data using the item content recommender.  In this
case, providing the user and item interaction data at model creation
time is optional.

#### Creating a model

There are a variety of machine learning techniques that can be used to
build a recommender model.  Turi Create provides a method
`turicreate.recommender.create` that will automatically choose an
appropriate model for your data set.

First we create a random split of the data to produce a validation set
that can be used to evaluate the model.

```python
training_data, validation_data = tc.recommender.util.random_split_by_user(actions, 'userId', 'movieId')
model = tc.recommender.create(training_data, 'userId', 'movieId')
```

Now that you have a model, you can make recommendations

```python
results = model.recommend()
```

#### Learn more

The following sections provide more information about the recommender model:

- [Using models](using-trained-models.md)
  * making recommendations
  * finding similar items and users
  * evaluating the model
  * interactive visualizations
  * saving models
  * and more
- [Choosing a model](choosing-a-model.md)
  * data you might encounter (implicit or explicit)
  * types of models worth considering (item-based similarity, factorization-based models, and so on).
- [API Docs](https://apple.github.io/turicreate/docs/api/turicreate.toolkits.recommender.html)
