# Choosing a Model

In this section, we give some intuition for which modeling choices you
may make depending on your data and your task. Each recommender model in
Turi Create has certain strengths that fit well with certain types of
data and different objectives.

The easiest way to choose a model is to let Turi Create choose your
model for you.  This is done by simply using the default
recommender.create function, which chooses the model based on the data
provided to it.  As an example, the following code creates a basic item
similarity model and then generates recommendations for each user in the
dataset:

```python
m = turicreate.recommender.create(data, user_id='user', item_id='movie')
recs = m.recommend()
```

Using the default create method provides an excellent way to quickly get
a recommender model up and running, but in many cases it's desirable to
have more control over the process.

Effectively choosing and tuning a recommender model is best done in two
stages.  The first stage is to match the type of data up with the
correct model or models, and the second stage is to correctly evaluate
and tune the model(s) and assess their accuracy.  Sometimes one model
works better and sometimes another, depending on the data set. In a
later section, we'll look at evaluating a model so you can be confident
you chose the best one.


### Data Type: Explicit, Implicit, or Item Content Data?

With *Explicit data*, there is an associated target column that gives a
score for each interaction between a users and an items.  An example of
this type would be a data set of user's ratings for movies or books.
With this type of data, the objective is typically to either predict
which new items a user would rate highly or to predict a user's rating
on a given item.

*Implicit data* does not include any rating information.  In this case,
a dataset may have just two columns -- user ID and item ID.  For this
type of data, the recommendations are based on which items are similar
to the items a user has interacted with.

The third type of data that Turi Create can use to build a recommender
system is *item content data*.  In this case, information associated
with each individual item, instead of the user interaction patterns, is
used to recommend items similar to a collection of items in a query set.
For example, item content could be a text description of an item, a set
of key words, an address, categories, or even a list of similar items
taken from another model.

### Working with Explicit Data

If your data is *explicit*, i.e., the observations include an actual
rating given by the user, then the model you wish to use depends on
whether you want to predict the rating a user would give a particular
item, or if you want the model to recommend items that it believes the
user would rate highly.

If you have ratings data and care about accurately predicting the rating
a user would give a specific item , then we typically recommend you use
the `factorization_recommender`.  In this model the observed ratings are
modeled as a weighted combination of terms, where the weights (along
with some of the terms, also known as factors) are learned from data.
All of these models can easily incorporate user or item side features.

A linear model assumes that the rating is a linear combination of user
features, item features, user bias, and item popularity bias.  The
`factorization_recommender` goes one step further and allows each rating
to also depend on a term representing the inner product of two vectors,
one representing the user's affinity to a set of latent preference
modes, and one representing the item's affinity to these modes.  These
are commonly called latent factors and are automatically learned from
observation data.  When side data is available, the model allows for
interaction terms between these learned latent factors and all the side
features.  As a rule of  thumb, the presence of side data can make the
model more finicky to learn (due to its power and flexibility).

If you care about *ranking performance*, instead of simply predicting
the rating accurately, then choose
[ItemSimilarityRecommender](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.item_similarity_recommender.ItemSimilarityRecommender.html)
or
[RankingFactorizationRecommender](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.ranking_factorization_recommender.RankingFactorizationRecommender.html).
With rating data, the `item_similarity_recommender` model scores items
based on how likely they predict the user will rate them highly, but the
absolute values of the predicted scores may not match up with the actual
ratings a user would give the item.

The
[RankingFactorizationRecommender](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.ranking_factorization_recommender.RankingFactorizationRecommender.html)
tries to recommend items that are both similar to the items in a user's
dataset and, if rating information is provided, those that would be
rated highly by the user.  It tends to predict ratings with less
accuracy than the non-ranking `factorization_recommender`, but it tends
to do much better at choosing items that a user would rate highly.  This
is because it also penalizes the predicted rating of items that are
significantly different from the items a user has interacted with.  In
other words, it only predicts a high rating for user-item pairs in which
it predicts a high rating and is confident in that prediction.

Furthermore, this model works particularly well when the target ratings
are binary, i.e., if they come from thumbs up/thumbs down flags. In this
case, use the input parameter `binary_targets = True`.

When a `target` column is provided, the model returned by the default
`recommender.create` function is a matrix factorization model. The
matrix factorization model can also be called directly with
[ranking_factorization_recommender.create](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.ranking_factorization_recommender.create.html).
When using the model-specific create function, other arguments can be
provided to better tune the model, such as `num_factors` or
`regularization`.  See the documentation on
[RankingFactorizationRecommender](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.ranking_factorization_recommender.RankingFactorizationRecommender.html)
for more information.

```python
m = turicreate.ranking_factorization_recommender.create(data,
									user_id='user',
                                    item_id='movie',
                                    target='rating')
```

### Working with Implicit Data

The goal a recommender system built with implicit data is to recommend
items that are similar to those similar to the collection of items a
user has interacted with.  "Similar" in this case is determined by other
user interactions -- if most users with similar behavior to a given user
also interacted with a item the given user had not, that item would
likely in the given user's recommendations.

In this case, the default `recommender.create` function in the example
code above code returns an
[ItemSimilarityRecommender](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.item_similarity_recommender.ItemSimilarityRecommender.html)
which computes the similarity between each pair of items and recommends
items to each user that are closest to items they have already used or
liked:

```python
m = turicreate.item_similarity_recommender.create(data,
                                    user_id='user',
                                    item_id='movie')
```

The `ranking_factorization_recommender` is also great for implicit data,
and can be called the same way:


```python
m = turicreate.ranking_factorization_recommender.create(data,
                                    user_id='user',
                                    item_id='movie')
```

With implicit data, the ranking factorization model has two solvers, one
which uses a randomized sgd-based method to tune the results, and the
other which uses an implicit form of alternating least squares (iALS).
The default sgd-based method samples unobserved items along with the
observed ones, then treats them as negative examples.  This is the
default solver.  Implicit ALS is a version of the popular Alternating
Least Squares (ALS) algorithm that attempts to find factors that
distinguish between the given user-item pairs and all other negative
examples.  This algorithm can be faster than the sgd method,
particularly if there are many items, but it does not currently support
side features.  This solver can be activated by passing in ``solver =
"ials"`` to ``ranking_factorization_recommender.create``. On some
datasets, one of these solvers can yield better precision-recall scores
than the `item_similarity_recommender`.

### Item Content Data

The
[ItemContentRecommender](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.item_content_recommender.ItemContentRecommender.html)
builds a model similar to the item similarity model, but uses
similarities between item content to actually build the model.  In
this model, the similarity score between two items is calculated by
first computing the similarity between the item data for each column,
then taking a weighted average of the per-column similarities to get
the final similarity.  The recommendations are generated according to
the average similarity of a candidate item to all the items in a
user's set of rated items.  This model can be created without
observation data about user-item interactions, in which case such
information must be passed in at recommend time in order to make
recommendations.

Note that in most situations, the similarity patterns of items
can be inferred effectively from patterns in the user interaction
data, and the `factorization_recommender` and
`item_similarity_recommender` do this effectively.  However,
leveraging information about item content can be very useful,
particularly when the user-item interaction data is sparse or not
known until recommend time.

### Side information for users, items, and observations

In many cases, additional information about the users or items can
improve the quality of the recommendations.  For example, including
information about the genre and year of a movie can be useful
information in recommending movies.  We call this type of information
user side data or item side data depending on whether it goes with the
user or the item.

Including side data is easy with the `user_data` or `item_data`
parameters to the `recommender.create()` function.  These arguments
are SFrames and must have a user or item column that corresponds to
the `user_id` and `item_id` columns in the observation data.  Internally,
the data is joined to the particular user or item when training the
model, the data is saved with the model and also used to make
recommendations.

In particular, the
[FactorizationRecommender](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.factorization_recommender.FactorizationRecommender.html)
and the
[RankingFactorizationRecommender](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.ranking_factorization_recommender.RankingFactorizationRecommender.html)
both incorporate the side data into the prediction through additional
interaction terms between the user, the item, and the side feature. For
the actual formula, see the API docs for the
[FactorizationRecommender](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.factorization_recommender.FactorizationRecommender.html).
Both of these models also allow you to obtain the parameters that have
been learned for each of the side features via the `m['coefficients']`
argument.

Side data may also be provided for each observation. For example, it
might be useful to have recommendations change based on the time at
which the query is being made. To do so, you could create a model using
an SFrame that contains a time column, in addition to a user and item
column. For example, a "time" column could include a string indicating
the hour; this will be treated as a categorical variable and the model
will learn a latent factor for each unique hour.

```python
# sf has columns: user_id, item_id, time
m = tc.ranking_factorization_recommender.create(sf)
```

In order to include this information when requesting observations, you
may include the desired additional data as columns in an SFrame for the
`users` argument to `m.recommend()`. In our example above, when querying
for recommendations, you would include the time that you want to use for
each set of recommendations.

```python
users_query = tc.SFrame({'user_id': [1, 2, 3], 'time': ['10pm', '10pm', '11pm']})
m.recommend(users=user_query)
```

In this case, recommendations for user 1 and 2 would use the parameters
learned from observations that occurred at 10pm, whereas the
recommendations for user 3 would incorporate parameters corresponding to
11pm. For more details, check out
[recommend](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.factorization_recommender.FactorizationRecommender.recommend.html#turicreate.recommender.factorization_recommender.FactorizationRecommender.recommend)
in the API docs.

You may check the number of columns used as side information by querying
`m['observation_column_names']`, `m['user_side_data_column_names']`, and
`m['item_side_data_column_names']`. By printing the model, you can also
see this information. In the following model, we had four columns in the
observation data (two of which were `user_id` and `item_id`) and four
columns in the SFrame passed to `item_side_data` (one of which was
`item_id`):

```no-highlight
Class                           : RankingFactorizationRecommender

Schema
------
User ID                         : user_id
Item ID                         : item_id
Target                          : None
Additional observation features : 2
Number of user side features    : 0
Number of item side features    : 3
```

If new side data exists when recommendations are desired, this can be
passed in via the `new_observation_data`, `new_user_data`, and
`new_item_data` arguments. Any data provided there will take precedence
over the user and item side data stored with the model.

Not all of the models make use of side data: the
`popularity_recommender` and `item_similarity_recommender` create
methods currently do not use it.


### Suggested pre-processing techniques

Lastly, here are a couple of common data issues that can affect the
performance of a recommender.  First, if the observation data is very
sparse, i.e., contains only one or two observations for a large number
of users, then none of the models will perform much better than the
simple baselines available via the `popularity_recommender`.  In this
case, it might help to prune out the rare users and rare items and try
again.  Also, re-examine the data collection and data cleaning process
to see if mistakes were made.  Try to get more observation data per user
and per item, if you can.

Another issue often occurs when usage data is treated as ratings.
Unlike explicit ratings that lie on a nice linear interval, say 0 to 5,
usage data can be badly skewed.  For instance, in the Million Song
dataset, one user played a song more than 16,000 times.  All the models
would have a difficult time fitting to such a badly skewed target.  The
fix is to bucketize the usage data.  For instance, any play count
greater than 50 can be mapped to the maximum rating of 5.  You can also
clip the play counts to be binary, e.g., any number greater than 2 is
mapped to 1, otherwise it's 0.


### Evaluating Model Performance

When trying out different recommender models, it's critical to have a
principled way of evaluating their performance.  The standard approach
to this is to split the observation data into two parts, a training
set and a test set.  The model is trained on the training set, and
then evaluated on the test set -- evaluating your model on the same
dataset that it was trained on gives a very bad idea of how well it
will perform in practice.  Once the model type and associated parameters
are chosen, the model can be trained on the full dataset.

With recommender systems, we can evaluate models using two different
metrics, RMSE and precision-recall.  RMSE measures how well the model
predicts the score of the user, while precision-recall measures how
well the recommend() function recommends items that the user also
chooses.  For example, the best RMSE value is when the model exactly
predicts the value of all the ratings in the test set.  Similarly, the
best precision-recall value is when the user has 5 items in the test
set and recommend() recommends exactly those 5 items.  While both can
be important depending on the type of data and desired task,
precision-recall is often more useful in evaluating how well a
recommender system will perform in practice.

The Turi Create recommender toolkit includes a function,
[tc.recommender.random_split_by_user](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.util.random_split_by_user.html),
to easily generate training and test sets from observation data.
Unlike `tc.SFrame.random_split`, it only puts data for a subset of the
users into the test set.  This is typically sufficient for evaluating
recommender systems.

`tc.recommender.random_split_by_user` generates a test set by first
choosing a subset of the users at random, then choosing a random
subset of that user's items.  By default, it chooses 1000 users and,
for each of these users, 20% of their items on average.  Note that not
all users may be represented by the test set, as some users may not
have any of their items randomly selected for the test set.

Once training and test set are generated, the
[tc.recommender.util.compare_models](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.util.compare_models.html#turicreate.recommender.util.compare_models)
function allows easy evaluation of several models using either RMSE or
precision-recall.  These models may the same models with different
parameters or completely different types of model.

The Turi Create recommender toolkit provides several ways of
working with rating data while ensuring good precision-recall.  To
accurately evaluate the precision-recall of a model trained on explicit
rating data, it's important to only include highly rated items in your
test set as these are the items a user would likely choose.  Creating
such a test set can be done with a handful of SFrame operations and
`tc.recommender.random_split_by_user`:

```no-highlight
high_rated_data = data[data["rating"] >= 4]
low_rated_data = data[data["rating"] < 4]
train_data_1, test_data = tc.recommender.random_split_by_user(
                                    high_rated_data, user_id='user', item_id='movie')
train_data = train_data_1.append(low_rated_data)
```

Other examples of comparing models can be found in the API
documentation for
[tc.recommender.util.compare_models](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.util.compare_models.html#turicreate.recommender.util.compare_models).
