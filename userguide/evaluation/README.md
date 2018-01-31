# Evaluating models

When evaluating models, choice of evaluation metrics is tied to the
specific machine learning task. For example, if you built a classifier
to detect spam emails vs. normal emails, then you should consider
classification performance metrics, such as average accuracy, log-loss,
and AUC. If you are trying to predict a score, such as Google’s daily
stock price, then you might want to consider regression metrics like the
root mean-squared error (RMSE). If you are ranking items by relevance to
a query, such as in a search engine, then you'll want to look into
ranking losses such as precision-recall (also popular as a
classification metric), or NDCG. These are all examples of task-specific
performance metrics.


### [Regression Metrics](regression.md)

In a regression task, the model learns to predict numeric scores. An
example is predicting the price of a stock on future days given past
price history and other information about the company and the market.
Another example is personalized recommendations, where the goal is to
predict a user’s rating for an item.


Here are a couple ways of measuring regression performance:

- [Root-Mean-Squared-Error](regression.md#rmse)
- [Max-Error](regression.md#max_error)


## [Classification Metrics](classification.md)

Classification is about predicting class labels given input data. In
binary classification, there are two possible output classes. In
multi-class classification, there are more than two possible classes. An
example of binary classification is spam detection, where the input data
could be the email text and metadata (sender, sending time), and the
output label is either “spam” or “not spam.” Sometimes, people use
generic names for the two classes: “positive” and “negative,” or “class
1” and “class 0.”

There are many ways of measuring classification performance:

- [Accuracy](classification.md#accuracy)
- [Confusion matrix](classification.md#confusion_matrix)
- [Log-loss](classification.md#log_loss)
- [Precision and Recall](classification.md#precision_recall)
- [F-Scores](classification.md#f_scores)
- [Receiver operating characteristic (ROC) curve](classification.md#roc_curve)
- [Area under curve (AUC) ("curve" corresponds to the ROC curve)](classification.md#auc)
