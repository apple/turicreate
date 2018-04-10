# Regression Metrics

In a regression task, the model learns to predict numeric scores. An example is
predicting the price of a stock on future days given past price history and
other information about the company and the market.


This section will deal with two ways of measuring regression performance:

- [Root-Mean-Squared-Error](regression.md#rmse)
- [Max-Error](regression.md#max_error)


## Root-Mean-Squared-Error (RMSE) {#rmse}

The most commonly used metric for regression tasks is RMSE (Root Mean Square
Error). This is defined as the square root of the average squared distance
between the actual score and the predicted score:

$$
\mbox{rmse} = \sqrt{\frac{\sum_{i=1}^{n}(y_i - \hat{y_i})^2}{n}}
$$

Here, $$y_i$$ denotes the true score for the i-th data point, and
$$\hat{ŷ_i$}$$ denotes the predicted value. One intuitive way to understand
this formula is that it is the Euclidean distance between the vector of the
true scores and the vector of the predicted scores, averaged by $$\sqrt{n}$$,
where $$n$$ is the number of data points.


```python
import turicreate as tc

y    = tc.SArray([3.1, 2.4, 7.6, 1.9])
yhat = tc.SArray([4.1, 2.3, 7.4, 1.7])

print(tc.evaluation.rmse(y, yhat))
```
```
0.522015325446
```

## Max-Error {#max_error}


While RMSE is the most common metric, it can be hard to interpret. One
alternative is to look at quantiles of the distribution of the absolute
percentage errors. The Max-Error metric is the **worst case** error between the
predicted value and the true value.

```python
import turicreate as tc

y    = tc.SArray([3.1, 2.4, 7.6, 1.9])
yhat = tc.SArray([4.1, 2.3, 7.4, 1.7])

print(tc.evaluation.max_error(y, yhat))
```
```
1.0
```
