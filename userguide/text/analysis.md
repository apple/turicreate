# Text Analysis

Suppose our text data is currently arranged into a single file, where
each line of that file contains all of the text in a single document.
Here we can use
[SFrame.read_csv](https://apple.github.io/turicreate/docs/api/generated/turicreate.SFrame.read_csv.html)
to parse the text data into a one-column SFrame.

```python
import turicreate
sf = turicreate.SFrame('wikipedia_data')
```
```no-highlight
Columns:
X1      str

Rows: 72269

Data:
+--------------------------------+
|               X1               |
+--------------------------------+
| alainconnes alain connes i ... |
| americannationalstandardsi ... |
| alberteinstein near the be ... |
| austriangerman as german i ... |
| arsenic arsenic is a metal ... |
| alps the alps alpen alpi a ... |
| alexiscarrel born in saint ... |
| adelaide adelaide is a coa ... |
| artist an artist is a pers ... |
| abdominalsurgery the three ... |
|              ...               |
+--------------------------------+
[72269 rows x 1 columns]
Note: Only the head of the SFrame is printed.
```

##### Text cleaning

We can easily remove all words do not occur at least twice in each
document using
[SArray.dict_trim_by_values](https://apple.github.io/turicreate/docs/api/generated/turicreate.SArray.dict_trim_by_values.html).

Turi Create also contains a helper function called
[stopwords](https://apple.github.io/turicreate/docs/api/generated/turicreate.text_analytics.stopwords.html?highlight=stopwords#turicreate.text_analytics.stopwords)
that returns a list of common words. We can use
[SArray.docs.dict_trim_by_keys](https://apple.github.io/turicreate/docs/api/generated/turicreate.SArray.dict_trim_by_keys.html)
to remove these words from the documents as a preprocessing step. NB:
Currently only English words are available.


```python
docs = docs.dict_trim_by_keys(turicreate.text_analytics.stopwords(), exclude=True)
```

To confirm that we have indeed removed common words, e.g. "and", "the",
etc, we can examine the first document.

```python
print(docs[0])
```
```no-highlight
{'academy': 5,
 'algebras': 2,
 'connes': 3,
 'differential': 2,
 'early': 2,
 'geometry': 2,
 'including': 2,
 'medal': 2,
 'operator': 2,
 'physics': 2,
 'sciences': 5,
 'theory': 2,
 'work': 2}
```
