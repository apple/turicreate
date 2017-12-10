# Using models

All recommender objects in the
[turicreate.recommender](https://apple.github.io/turicreate/docs/api/turicreate.toolkits.recommender.html)
module expose a common set of methods, such as
[recommend](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.factorization_recommender.FactorizationRecommender.recommend.html#turicreate.recommender.factorization_recommender.FactorizationRecommender.recommend)
and
[evaluate](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.factorization_recommender.FactorizationRecommender.evaluate.html).

In this section we will cover

- [Making recommendations](#making-recommendations)
- [Finding similar items](#finding-similar-items)
- [Saving and loading](#saving-and-loading-models)

##### Making recommendations

Once a model is created, you can now make recommendations of new items
for users.  To do so, call `model.recommend()` with an SArray of user
ids.  If `users` is set to None, then `model.recommend()` will make
recommendations for all the users seen during model creation, automatically
excluding the items that are observed for each user.  In other words, if
`data` contains a row "Alice, The Great Gatsby", then
`model.recommend()` will not recommend "The Great Gatsby" for user
"Alice".  It will return at most `k` new items for each user, sorted by
their rank.  It will return fewer than `k` items if there are not enough
items that the user has not already rated or seen.

The `score` column of the output contains the *unnormalized*
prediction scores for each user-item pair.  The semantic meanings of
these scores may differ between models.  For the linear regression
model, for instance, a higher average score for a user means that the
model thinks that this user is generally more enthusiastic than
others.

There are a number of ways to make recommendations: for known users or
new users, with new observation data or side information, and with
different ways to explicitly control item inclusion or exclusion.  Let's
walk through these options together.

##### Making recommendations for all users

By default, calling `m.recommend()` without any arguments returns the
top 10 recommendations for all users seen during model creation.  It
automatically excludes items that were seen during model creation. Hence
all generated recommendations are for items that the user has not
already seen.

```python
data = turicreate.SFrame({'user_id': ["Ann", "Ann", "Ann", "Brian", "Brian", "Brian"],
                  		'item_id': ["Item1", "Item2", "Item4", "Item2", "Item3", "Item5"],
                  		'rating': [1, 3, 2, 5, 4, 2]})
m = turicreate.factorization_recommender.create(data, target='rating')

recommendations = m.recommend()
```

##### Making recommendations for specific users

If you specify a `list` or `SArray` of users, `recommend()` returns
recommendations for only those user(s). The user names must correspond
to strings in the `user_id` column in the training data.

```python
recommendations = m.recommend(users=['Brian'])
```

##### Making recommendations for specific users and items

In situations where you build a model for all of your users and items,
you may wish to limit the recommendations for particular users based on
known item attributes. For example, for US-based customers you may want
to limit recommendations to US-based products. The following code sample
restricts recommendations to a subset of users and items -- specifically
those users and items whose value in the 'country' column is equal to
"United States".

```
country = 'United States'
m.recommend(users=users['user_id'][users['country']==country].unique(),
            items=items['item_id'][items['country']==country])
```

##### Making recommendations for new users

This is known as the "cold-start" problem.  The `recommend()` function
works seamlessly with new users. If the model has never seen the user,
then it defaults to recommending popular items:

```python
m.recommend(['Charlie'])
```

Here 'Charlie' is a new user that does not appear in the training data.
Also note that you don't need to explicitly write down `users=`; Python
automatically assumes that arguments are provided in order, so the first
unnamed argument to `recommend()` is taken to be the user list.


##### Incorporating information about a new user

To improve recommendations for new users, it helps to have side
information or new observation data for the user.

##### Incorporating new side information

To incorporate side information, you must have already created a
recommender model that knows how to incorporate side features.  This can
be done by passing in side information to `create()`.  For example:

```python
user_info = turicreate.SFrame({'user_id': ['Ann', 'Brian'],
                       		 'age_category': ['2', '3']})
m_side_info = turicreate.factorization_recommender.create(data, target='rating',
           		      		                            user_data=user_info)
```

Now, we can add side information for the new user at recommendation
time. The new side information must contain a column with the same name
as the column in the training data that's designated as the 'user_id'.
(For more details, please see the API documentation for
[turicreate.recommender.create](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.create.html#turicreate.recommender.create).)

```python
new_user_info = turicreate.SFrame({'user_id' : ['Charlie'],
								 'age_category' : ['2']})
recommendations = m_side_info.recommend(['Charlie'],
										new_user_data = new_user_info)
```

Given Charlie's age category, the model can incorporate what it knows about the importance of age categories for item recommendations.  Currently, the following models can take side information into account when making recommendations: [LinearRegressionModel](https://apple.github.io/turicreate/docs/api/generated/turicreate.linear_regression.LinearRegression.html), [FactorizationRecommender](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.factorization_recommender.FactorizationRecommender.html#turicreate.recommender.factorization_recommender.FactorizationRecommender).  LinearRegressionModel is the simpler model, and FactorizationRecommender the more powerful.  For more details on how each model makes use of side information, please refer to the model definition sections in the individual models' API documentation.

#### Incorporating new observation data

`recommend()` accepts new observation data. Currently, the [ItemSimilarityModel](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.item_similarity_recommender.ItemSimilarityRecommender.html) makes the best use of this information.

```python
m_item_sim = turicreate.item_similarity_recommender.create(data)
new_obs_data = turicreate.SFrame({'user_id' : ['Charlie', 'Charlie'],
	                        	'item_id' : ['Item1', 'Item5']})
recommendations = m_item_sim.recommend(['Charlie'], new_observation_data = new_obs_data)
```

##### Controlling the number of recommendations

The input parameter `k` controls how many items to recommend for each user.

```python
recommendations = m.recommend(k = 5)
```

##### Excluding specific items from recommendation

Suppose you make some recommendations to the user and they ignored them.
So now you want other recommendations.  This can be done by explicitly
excluding those undesirable items via the `exclude` keyword argument.

```python
exclude_pairs = turicreate.SFrame({'user_id' : ['Ann'],
                           		 'item_id' : ['Item3']})

recommendations = m.recommend(['Ann'], k = 5, exclude = exclude_pairs)
```

By default, `recommend()` excludes items seen during training, so that
it would not recommend items that the user has already seen.  To change
this behavior, you can specify `exclude_known=False`.

```python
recommendations = m.recommend(exclude_known = False)
```

##### Including specific items in recommendation

Suppose you want to see only recommendations within a subset of items.
This can be done via the `items` keyword argument.  The input must be an
SArray of items.

```python
item_subset = turicreate.SArray(["Item3", "Item5", "Item2"])
recommendations = m.recommend(['Ann'], items = item_subset)
```

#### Finding Similar Items

Many of the above models make recommendations based on some notion of
similarity between a pair of items. Querying for similar items can help
you understand the model's behavior on your data.

We have made this process very easy with the
[get_similar_items](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.item_similarity_recommender.ItemSimilarityRecommender.get_similar_items.html#turicreate.recommender.item_similarity_recommender.ItemSimilarityRecommender.get_similar_items)
function:

```python
similar_items = model.get_similar_items(my_list_of_items, k=20)
```

The above will return an SFrame containing the 20 nearest items for
every item in `my_list_of_items`. The definition of "nearest" depends on
the type of similarity used by the model. For instance, "jaccard"
similarity measures the two item's overlapping users. The 'score' column
contains a similarity score ranging between 0 and 1, where larger values
indicate increasing similarity. The mathematical formula used for each
type of similarity can be found in the API documentation for
[ItemSimilarityRecommender](https://apple.github.io/turicreate/docs/api/generated/turicreate.recommender.item_similarity_recommender.ItemSimilarityRecommender.html#turicreate.recommender.item_similarity_recommender.ItemSimilarityRecommender).

For a factorization-based model, the similarity used for is the
Euclidean distance between the items' two factors, which can be obtained
using m['coefficients'].

#### Saving and loading models

The model can be saved for later use, either on the local machine or in
an AWS S3 bucket. The saved model sits in its own directory, and can be
loaded back in later to make more predictions.

```python
model.save("my_model.model")
```
Like other models in Turi Create, we can load the model back:

```python
model = tc.load_model("my_model.model")
```
