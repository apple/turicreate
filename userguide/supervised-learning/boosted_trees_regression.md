# Gradient Boosted Regression Trees

The Gradient Boosted Regression Trees (GBRT) model (also called Gradient
Boosted Machine or GBM) is one of the most effective machine learning
models for predictive analytics, making it an industrial workhorse for
machine learning.

##### Background
The Boosted Trees Model is a type of additive model that makes
predictions by combining decisions from a sequence of base models. More
formally we can write this class of models as:

$$
    g(x) = f_0(x) + f_1(x) + f_2(x) + ...
$$

where the final classifier $$g$$ is the sum of simple base classifiers $$f_i$$.
For boosted trees model, each base classifier is a simple [decision
tree](decision_tree_regression.md). This broad technique of
using multiple models to obtain better predictive performance is called
[model ensembling](http://en.wikipedia.org/wiki/Ensemble_learning).

Unlike Random Forest which constructs all the base classifier independently,
each using a subsample of data, GBRT uses a particular model ensembling
technique called [gradient boosting](http://en.wikipedia.org/wiki/Gradient_boosting).

The name of Gradient Boosting comes from its connection to the Gradient Descent
in numerical optimization. Suppose you want to optimize a function $$f(x)$$,
assuming $$f$$ is differentiable, gradient descent works by iteratively find

$$
  x_{t+1} = x_{t} - \eta \left.\frac{\partial f}{\partial x} \right|_{x = x_t}
$$

where $$\eta$$ is called the step size.

Similarly, if we let $$g_t(x) = \sum_{i=0}^{t-1} f_i(x)$$ be the classifier
trained at iteration $$t$$, and $$L(y_i, g(x_i))$$ be the empirical loss function,
at each iteration we will move $$g_t$$ towards the negative gradient
direction $$-\left.\frac{\partial L}{\partial g} \right|_{g = g_t}$$ by $$\eta$$ amount.
Hence, $$f_{t}$$ is chosen to be

$$
  f_t = \arg\min_{f}\sum_{i=1}^N [\left.\frac{\partial L(y_i, g(x_i))}{\partial g(x_i)}\right|_{g=g_t} - f(x_i)]^2
$$

and the algorithm sets $$g_{t+1} = g_{t} + \eta f_t$$.

For regression problems with squared loss function, $$\frac{\partial L(y_i, g(x_i))}{\partial g(x_i)}$$ is simply $$y_i - g(x_i)$$.
The algorithm simply fit a new decision tree to the residual at each iteration.

##### Introductory Example

In this example, we will use the [Mushrooms dataset](https://archive.ics.uci.edu/ml/datasets/mushroom).[<sup>1</sup>](../datasets.md)
```python
import turicreate as tc

# Load the data
data =  tc.SFrame.read_csv('https://raw.githubusercontent.com/apple/turicreate/master/src/python/turicreate/test/mushroom.csv')

# Label 'p' is edible
data['label'] = data['label'] == 'p'

# Make a train-test split
train_data, test_data = data.random_split(0.8)

# Create a model.
model = tc.boosted_trees_regression.create(train_data, target='label',
                                           max_iterations=2,
                                           max_depth =  3)

# Save predictions to an SArray
predictions = model.predict(test_data)

# Evaluate the model and save the results into a dictionary
results = model.evaluate(test_data)
```

##### Tuning hyperparameters
The Gradient Boosted Trees model has many tuning parameters. Here we provide a simple guideline for tuning the model.

- `max_iterations`
  Controls the number of trees in the final model. Usually the more trees, the higher accuracy.
  However, both the training and prediction time also grows linearly in the number of trees.

- `max_depth`
  Restricts the depth of each individual tree to prevent overfitting.

- `step_size`
  Also called shrinkage, appeared as the $$\eta$$ in the equations in the Background section.
  It works similar to the learning rate of the gradient descent procedure: smaller value
  will take more iterations to reach the same level of training error of a larger step size.
  So there is a trade off between step_size and number of iterations.

- `min_child_weight`
  One of the pruning criteria for decision tree construction. In classification problem, this
  corresponds to the minimum observations required at a leaf node. Larger value
  produces simpler trees.

- `min_loss_reduction`
  Another pruning criteria for decision tree construction. This restricts the reduction of
  loss function for a node split. Larger value produces simpler trees.

- `row_subsample`
  Use only a fraction of data at each iteration. This is
  similar to the mini-batch [stochastic gradient descent](http://en.wikipedia.org/wiki/Stochastic_gradient_descent)
  which not only reduce the computation cost of each iteration, but may also produce
  more robust model.

- `column_subsample`
  Use only a subset of the columns to use at each iteration.

In general, you can choose `max_iterations` to be large and fit your computation budget.
You can then set `min_child_weight` to be a reasonable value around
(#instances/1000), and tune max_depth. When you have more training instances,
you can set max_depth to a higher value. When you find a large gap between
the training loss and validation loss, a sign of overfitting, you may want
to reduce depth, and increase min_child_weight.

##### When to use a boosted trees model?
Different kinds of models have different advantages. The boosted trees model is
very good at handling tabular data with numerical features, or categorical
features with fewer than hundreds of categories. Unlike linear models, the
boosted trees model are able to capture non-linear interaction between the
features and the target.

One important note is that tree based models are not designed to work with very
sparse features. When dealing with sparse input data (e.g. categorical features
                                                      with large dimension), we
can either pre-process the sparse features to generate numerical statistics, or
switch to a linear model, which is better suited for such scenarios.
