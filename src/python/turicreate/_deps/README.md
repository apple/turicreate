# Lazy import layout

---

## raitionale

Most times, people tend to perform only one task at one time. There's no need to eagerly import all toolkits when `import turicreate`. When needed, the toolkit will be loaded in runtime

---

## code design

### design of LazyModuleLoader

`LazyModuleLoader` is a thin wrapper delegate the requests to real module object. Instead of loading module directly, the `LazyModuleLoader` will defer the load until `__getattr__`, `__setattr__` or `__delattr__` is called on attributes not from `LazyModuleLoader` itself.

For debug purpose, people often use `reload` to reload python source. `reload` is not directly suported. You need to call `reload` member method of `LazyModuleLoader`. Other than that, it works as the same as `reload` builtin function call.

```python
# won't load numpy
numpy = LazyModuleLoader(numpy)
# load the numpy and moudle numpy will be registered in sys.modules
numpy.ndarray
```

`LazyModuleLoader` supports customized moudle initialization function. By default, the initialization function will do exacly same thing as `import` declarative.

### design of LazyCallable

`LazyModuleLoader` cannot defer a `from ... import ...` clause, even though it's an variant of `import` clause. Often the case, you want to bind function to current namespace or module scope, e.g.,

```python
# lazy load image_analysis module
image_analysis = LazyModuleLoader(turicreate.toolkits.image_analysis)

# bind load_image to current namespace for user convenience
from turicreate.toolkits.image_analysis import load_image
```

This will actually eagerly load the module `image_analysis`, which we want to defer.

`LazyCallable` is a thin wrapper to `LazyModuleLoader` or any `ModuleTypes` to defer the load of module by calling `__getattr__(function_name)` on `LazyModuleLoader` when it's called by `__call__`.

---

## how to orgranize the module with lazy import

At top package level `turicreate/__init__.py`, lazy module is exposed as package level global variables to be backwards-compatible with submodule invokation.

```python
# defineed turicreate/__init__.py
distances = _LazyModuleLoader('turicreate.toolkits.distances'

# user call
import turicreate
turicreate.distances
```

Usually the sub-package (toolkit suite) is located under directory `turicreate/toolkits`.

```bash
├── activity_classifier
├── classifier
├── clustering
├── distances
├── graph_analytics
├── image_analysis
├── image_classifier
├── image_similarity
├── nearest_neighbors
├── recommender
├── regression
├── sound_classifier
├── style_transfer
├── text_analytics
├── text_classifier
└── topic_model
```

### standalone submodule lazy import

For a package containing exactly one standalone functional unit (tookit), it's easy to make it as a lazy module by only editing the `turicreate/__init__.py`. Take `style_transfer` as an example:

```python
# in turicreate/__init__.py
# module style_transfer is located at turicreate/toolkits/style_transfer
style_transfer = _LazyModuleLoader('turicreate.toolkits.style_transfer'
```

No need to modify the `__init__.py` of the submodule.

### submodule with more than one functional unit

Usually a package consists of many different functional units and it works as a hub to aggregate all of funtional units sharing similar traits. `audio_analytics` and `sound_classifier` are outliers. For example,

```bash
# recommender category layout
.
├── __init__.py
├── factorization_recommender.py
├── item_content_recommender.py
├── item_similarity_recommender.py
├── popularity_recommender.py
├── ranking_factorization_recommender.py
```

Recommder tookit contains 4 different recommender functional units and they are exposed through `__init__.py` at sub-package level or through `.py` submodule level.

In this case, we need to do extra work to bind all member funtional units (toolkits) to global package namespace `turicreate/__init__.py`. We need to make each unit lazily imported under local sub-package level, i.e., `turicreate/toolkits/recommender/__init__/py`.

```python
# turicreate/toolkits/recommender/__init__/py
_mod_par = 'turicreate.toolkits.recommender.'

popularity_recommender = _LazyModuleLoader(_mod_par + 'popularity_recommender')
factorization_recommender = _LazyModuleLoader(_mod_par + 'factorization_recommender')
ranking_factorization_recommender = _LazyModuleLoader(_mod_par + 'ranking_factorization_recommender')
item_similarity_recommender = _LazyModuleLoader(_mod_par + 'item_similarity_recommender')
item_content_recommender = _LazyModuleLoader(_mod_par + 'item_content_recommender')

util = _LazyModuleLoader(_mod_par + 'util')
create = _LazyCallable(util, '_create')
```

in `turicreate/__init__.py`, we do import as normal, except all components as substituted as `LazyModuleLoader` objects instead of eagerly loaded module objects.

```python
import turicreate.toolkits.recommender as recommender
from turicreate.toolkits.recommender import popularity_recommender
from turicreate.toolkits.recommender import item_similarity_recommender
from turicreate.toolkits.recommender import ranking_factorization_recommender
from turicreate.toolkits.recommender import item_content_recommender
from turicreate.toolkits.recommender import factorization_recommender
```
