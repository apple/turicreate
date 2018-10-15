# Text analysis

Turi Create. Applications for automated text analytics include:
In this chapter we will show how to do standard text analytics using

* detecting user sentiment regarding product reviews
* creating features for use in other machine learning models
* understanding large collections of documents


# Topic Models

"Topic models" are a class of statistical models for text data. These
models typically assume documents can be described by a small set of
topics, and there is a probability of any word occurring for a given
"topic".

For example, suppose we are given the documents shown below, where the
first document begins with the text "The burrito was terrible. I..." and
continues with a long description of the eater's woes. A topic model
attempts to do two things:

* Learn "topics": collections of words that co-occur in a meaningful way
  (as shown by the colored word clouds).
* Learn how much each document pertains to each topic. This is
  represented by the colored circles, where larger circles indicate
larger probabilities.

![topic_model](images/topic_model.png)

There are many variations of topic models that incorporate other sources
of data, and there are a variety of algorithms for learning the
parameters of the model from data. This section focuses on creating and
using topic models with Turi Create.

##### Creating a model

The following example includes the  SArray of documents that we created
in previous sections: Each element represents a document in "bag of
words" representation -- a dictionary with word keys and whose values
are the number of times that word occurred in the document. Once in this
form, it is straightforward to learn a topic model.


```python
import turicreate as tc

docs = tc.SFrame('wikipedia_data')

# Remove stop words and convert to bag of words
docs = tc.text_analytics.count_words(docs['X1'])
docs = docs.dict_trim_by_keys(tc.text_analytics.stop_words(), exclude=True)

# Learn topic model
model = tc.topic_model.create(docs)
```

There are a variety of additional arguments available which are covered in the [API
 Reference](https://apple.github.io/turicreate/docs/api/generated/turicreate.topic_model.create.html).

The two most commonly used arguments are:

 - `num_topics`: Changes the number of topics to learn.
 - `num_iterations`: Changes how many iterations to perform.

The returned object is a TopicModel object, which exposes several useful
methods. For example,
[turicreate.topic_model.TopicModel.get_topics()](https://apple.github.io/turicreate/docs/api/generated/turicreate.topic_model.TopicModel.get_topics.html)
returns an SFrame containing the most probable words for each topic and
a score related to how high that word ranks for that topic.

You may get details on a subset of topics by supplying a list of topic
names or topic indices, as well as restrict the number of words returned
per topic.


```python
print(model.get_topics())
```
```no-highlight
+-------+--------+------------------+
| topic |  word  |      score       |
+-------+--------+------------------+
|   0   | state  | 0.00781031634036 |
|   0   |  team  | 0.0116096428466  |
|   0   | league | 0.00734010266384 |
|   0   | world  | 0.00707611127116 |
|   0   | party  | 0.00693101676527 |
|   1   |  area  | 0.00939951828831 |
|   1   |  film  | 0.00903846641308 |
|   1   | album  | 0.00810524480636 |
|   1   | states | 0.00592481420794 |
|   1   |  part  | 0.00591876305919 |
+-------+--------+------------------+
[50 rows x 3 columns]
```

If we just want to see the top words per topic, this code snippet will
rearrange the above SFrame to do that.

```python
print(model.get_topics(output_type='topic_words'))
```

```no-highlight
+-------------------------------+
|             words             |
+-------------------------------+
| [team, state, league, worl... |
| [area, film, album, states... |
| [city, family, war, built,... |
| [united, american, state, ... |
| [school, year, high, stati... |
| [season, music, church, ye... |
| [de, century, part, includ... |
| [university, 2006, line, s... |
| [age, population, people, ... |
| [time, series, work, life,... |
+-------------------------------+
```

The model object keeps track of various useful statistics about how the
model was trained and its current status.

```no-higlight
Topic Model
    Data:
        Vocabulary size:     632779
    Settings:
        Number of topics:    10
        alpha:               5.0
        beta:                0.1
        Iterations:          10
        Training time:       7.2635
        Verbose:             False
    Accessible fields:
        m['topics']          An SFrame containing the topics.
        m['vocabulary']      An SArray containing the topics.
    Useful methods:
        m.get_topics()       Get the most probable words per topic.
        m.predict(new_docs)  Make predictions for new documents.

```
To predict the topic of a given document, one can get an SArray of
integers containing the most probable topic ids:

```python
pred = model.predict(docs)
```

Combining the above method with standard SFrame capabilities, one can
use predict to find documents related to a particular topic

```python
docs_in_topic_0 = docs[model.predict(docs) == 0]
```

or join with other data in order to analyze an author's typical topics
or how topics change over time. For example, if we had author and
timestamp data, we could do the following:

```python
m = topic_model.create(doc_data['text'])
doc_data['topic'] = m.predict(doc_data['text'])
doc_data['author'][doc_data['topic'] == 1] # authors of docs in topic 1
```

Sometimes you want to know how certain the model's predictions are. One
can optionally also get the probability of each topic for a set of
documents. Each element of the returned SArray is a vector containing
the probability of each document.


```python
pred = model.predict(docs, output_type='probability')
```
```no-highlight
array('d', [0.049204587495375506, 0.14502404735479096,
0.028116907140214576, 0.1709211986681465, 0.1220865704772475,
0.15945246022937476, 0.08213096559378469, 0.017388087310395855,
0.219385867554569, 0.006289308176100629])
```

##### Working with TopicModel objects

The value for each metadata field listed in ```m.get_fields()``` is
accessible via ```m[field]```.

A topic model will learn parameters for every word that is present in
the corpus. The "vocabulary" stored by the model will return this list
of words.

```python
model['vocabulary']
```
```no-highlight
dtype: str
Rows: 632779
['definitional', 'diversity', 'countereconomics', 'level', 'simultaneously', 'process', 'technology', 'phenomena', 'attitudes', 'consumer', 'creation', 'selfdetermined', 'ivan', 'popularized', 'se', 'questioned', 'organizing', 'addition', 'germany', 'experiments', 'activity', 'childled', 'series', 'inspiration', 'moderna', 'field', 'country', 'projects', 'pedagogical', 'murray', 'anticlerical', 'fiercely', 'noncoercive', 'rational', 'widely', 'church', 'defiance', 'progressive', 'ferrer', 'freethinker', 'fragments', 'linvention', 'solaire', 'amoureux', 'du', 'onfray', 'joy', 'writing', 'recently', 'proponent', 'consenting', 'camaraderie', 'la', 'emile', 'teufel', 'beginning', 'homosexuality', 'spoke', '18491898', 'reitzel', 'fact', 'boldly', 'campaigned', 'noted', 'frequented', 'villagers', 'discussion', 'margaret', 'lesbian', 'millay', 'edna', 'bisexual', 'sexuality', 'encouraged', 'greenwich', 'lazarus', 'une', 'staunchly', '18721890', 'angela', 'heywood', 'existed', 'discriminated', 'womens', 'sexual', 'viewed', 'pleasure', 'experimental', 'love', 'proven', 'disputes', 'internal', 'overlapping', 'cryptography', 'intersecting', 'bonanno', 'alfredo', 'formal', 'critical', 'recent', ... ]
```

Similarly, we can obtain the current parameters for each word by
requesting the "topics" stored by the model. Each element of the
"topic_probabilities" column contains an array with length
```num_topics``` where element $k$ is the probability of that word under
topic $k$.

```python
model['topics']
```

```no-highlight
Columns:
        topic_probabilities     array
        vocabulary      str

Rows: 632779

Data:
+-------------------------------+------------------+
|      topic_probabilities      |    vocabulary    |
+-------------------------------+------------------+
| [8.49255529868e-08, 8.6181... |   definitional   |
| [8.49255529868e-08, 7.8347... |    diversity     |
| [8.49255529868e-08, 7.8347... | countereconomics |
| [8.49255529868e-08, 7.8347... |      level       |
| [8.49255529868e-08, 7.8347... |  simultaneously  |
| [8.49255529868e-08, 7.8347... |     process      |
| [8.49255529868e-08, 7.8347... |    technology    |
| [8.49255529868e-08, 2.4287... |    phenomena     |
| [2.13163137997e-05, 0.0001... |    attitudes     |
| [8.49255529868e-08, 2.8283... |     consumer     |
+-------------------------------+------------------+
```

As with other models in Turi Create, it's also easy to save and load
models.

```python
model.save('my_model.model')
new_model = tc.load_model('my_model.model')
```

##### Importing from other formats

In some cases your data may be in a format that some refer to as
"docword", where each row in the text file contains a document id, a
word id, and the number of times that word occurs in that document. For
this situation, check out the `parse_docword` utility:

```
docs = tc.text_analytics.parse_docword(doc_word_file, vocab_file)
```

##### Initializing from other models

It is also easy to create a new topic model from an old one – whether it
was created using Turi Create or another package.


```python
new_model = tc.topic_model.create(docs,
                                        num_topics=m['num_topics'],
                                        initial_topics=m['topics'])
```

##### Seeding the model with prior knowledge

To manually fix several words to always be assigned to a topic, use the
associations argument. This can be useful for experimentation purposes.
For example, the following will ensure that "season" and "club" will
have a high probability under topic 1 and "2008" and "2009" will have a
high probability under topic 2:

```python
associations = tc.SFrame({'word':['season', 'club', '2008', '2009'],
                                'topic': [1, 1, 2, 2]})
```

If we fit a topic model using this option, we indeed find that "season"
and "club" are present in topic 1, and we find other related words in
the same topic.

```python
m2 = tc.topic_model.create(docs,
                           num_topics=20,
                           num_iterations=50,
                           associations=associations,
                           verbose=False)

topics = m2.get_topics(num_words=10, output_type='topic_words')['words'][1]
```
```no-highlight
['season',
 'club',
 'league',
 'played',
 'won',
 'football',
 'world',
 'cup',
 'year',
 'championship']
```

```python
m2.get_topics(num_words=10, output_type='topic_words')['words'][2]
```
```no-highlight
['team',
 '2008',
 'game',
 '2009',
 '2007',
 '2010',
 'final',
 'player',
 '1',
 'players']
```

##### Evaluating topic models

A common quantitative way to evaluate topic models is to split each
document into a training set and a test set, learn a topic model on the
training portion of each document, and compute the probability of the
held out word counts under the model. A slight variation of this
probability is called "perplexity". Lower values are better. Estimates
of this quantity are provided during training. See
[turicreate.text_analytics.random_split](https://apple.github.io/turicreate/docs/api/generated/turicreate.text_analytics.random_split.html),
[turicreate.topic_model.perplexity](https://apple.github.io/turicreate/docs/api/generated/turicreate.topic_model.perplexity.html),
[TopicModel.evaluate](https://apple.github.io/turicreate/docs/api/generated/turicreate.topic_model.TopicModel.evaluate.html)
for helper functions to do this sort of evaluation on trained models.

A common way to qualitatively evaluate topic models is to examine the
most probable words in each topic and count the number of words that do
not fit with the rest. If there are topics with words that do not
co-occur in your corpus, you may want to try:

* removing stop words and other words that are not interesting to your analysis
* changing the number of topics
* increasing the number of iterations

To learn more check out the [API
Reference](https://apple.github.io/turicreate/docs/api/generated/turicreate.topic_model.create.html).
