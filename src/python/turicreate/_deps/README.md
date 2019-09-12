# Lazy import layout

---

## Rationale

The elapsed import time of turicreate 5.7 is more than 2 seconds (wall time more than 1.21 seconds) on a commodity laptop, which is extremely slower than state-of-the-art tools' import speed. At the time of this writing, the elapsed import time of sklearn is around 560 ms (wall time around 110 ms) on a commodity laptop.

Most times, people tend to perform only one task at one time. There's no need to eagerly import all toolkits when `import turicreate`. When needed, the toolkit will be loaded in runtime

---

## Code design

### Design of LazyModuleLoader

`LazyModuleLoader` is a thin wrapper delegate the requests to the real module object. You can use it as a regular module except `reload`, which is mentioned later. Instead of loading module directly, the `LazyModuleLoader` will defer the load until `__getattr__`, `__setattr__` or `__delattr__` is called on attributes not from `LazyModuleLoader` itself.

For debug purposes, people often use `reload` to reload python source. `reload` is not directly supported. You need to call `reload` member method of `LazyModuleLoader`. Other than that, it works as the same as `reload` builtin function call.

```python
# won't load numpy
numpy = LazyModuleLoader(numpy)
# load the numpy and moudle numpy will be registered in sys.modules
numpy.ndarray
```

`LazyModuleLoader` supports customized module initialization function. By default, the initialization function will do exactly the same thing as `import` declarative.

### Design of LazyCallable

`LazyModuleLoader` cannot defer a `from ... import ...` clause, even though it's a variant of `import` clause. Often the case, you want to bind the function to a current namespace or module scope, e.g.,

```python
# lazy load image_analysis module
image_analysis = LazyModuleLoader(turicreate.toolkits.image_analysis)

# bind load_image to current namespace for user convenience
from turicreate.toolkits.image_analysis import load_image
```

This will eagerly load the module `image_analysis`, which we want to defer.

`LazyCallable` is a thin wrapper to `LazyModuleLoader` or any `ModuleTypes` to defer the load of module by calling `__getattr__(function_name)` on `LazyModuleLoader` when it's called by `__call__`.

---

## Principles of organizing your module with lazy import

### toolkit

At top package level `turicreate/__init__.py`, lazy module is exposed as package level global variables to be backward-compatible with submodule invokation.

```python
# defineed turicreate/__init__.py
distances = _LazyModuleLoader('turicreate.toolkits.distances'

# user call
import turicreate
turicreate.distances
```

Usually, the sub-package (toolkit suite) is located under directory `turicreate/toolkits`.

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

#### Whole package is a standalone functional unit

For a package containing exactly one standalone functional unit (tookit), it's easy to make it as a lazy module by only editing the `turicreate/__init__.py`. Take `style_transfer` as an example:

```python
# in turicreate/__init__.py
# module style_transfer is located at turicreate/toolkits/style_transfer
style_transfer = _LazyModuleLoader('turicreate.toolkits.style_transfer'
```

No need to modify the `__init__.py` of the submodule.

#### Package contains multiple submodules

Usually, a package consists of many different functional units and it works as a hub to aggregate all of functional units sharing similar traits. `audio_analytics` and `sound_classifier` are outliers. For example,

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

Recommender toolkit contains 4 different recommender functional units and they are exposed through `__init__.py` at the sub-package level or through `.py` submodule level.

In this case, we need to do extra work to bind all member functional units (toolkits) to global package namespace `turicreate/__init__.py`. We need to make each unit lazily imported under local sub-package level, i.e., `turicreate/toolkits/recommender/__init__/py`.

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

in `turicreate/__init__.py`, we do import as we do for regular modules, except all components as substituted as `LazyModuleLoader` objects instead of eagerly loaded module objects.

```python
import turicreate.toolkits.recommender as recommender
from turicreate.toolkits.recommender import popularity_recommender
from turicreate.toolkits.recommender import item_similarity_recommender
from turicreate.toolkits.recommender import ranking_factorization_recommender
from turicreate.toolkits.recommender import item_content_recommender
from turicreate.toolkits.recommender import factorization_recommender
```

### Dependency

It's a burden to remember to do pythonic lazy import for certain packages, such as numpy, pandas, and sklearn. In python, you can do like this,

```python
def foo():
    import pandas as pd
```

As mentioned above, module `pandas` will be imported if only if the function `foo` is invoked. However, the drawback of this approach is that you need to remember to lazy import certain packages all the time when you code. You may inadvertently forget to obey this rule, and then the package will be eagerly loaded, which will add more time to the total package import time. Like finding a memory leak, you may spend hours of finding module loading leaks.

To address this, in turicreate, we have a convention that we aggregate common and heavily relied modules into file `turicreate/_deps/__init__.py`. Taking `pandas` for example,

```python
HAS_PANDAS = __has_module('pandas')
PANDAS_MIN_VERSION = '0.13.0'

# called within import lock; don't lock inside
def _dynamic_load_pandas(name):
    global HAS_PANDAS
    if HAS_PANDAS:
        _ret = sys.modules.get(name, None)
        if _ret is None:
            assert name not in sys.modules.keys(), ("sys.modules[%s] cannot be None"
                                                    " during moudle loading" % name)
            import pandas as _ret
        if __get_version(_ret.__version__) < _StrictVersion(PANDAS_MIN_VERSION):
            HAS_PANDAS = False
            _logging.warn(('Pandas version %s is not supported. Minimum required version: %s. '
                           'Pandas support will be disabled.')
                          % (pandas.__version__, PANDAS_MIN_VERSION))
    if not HAS_PANDAS:
        from . import pandas_mock as _ret
    return _ret

pandas = LazyModuleLoader('pandas', _dynamic_load_pandas)
```

When you want to use `pandas`, in your python file, just do normal import but in a slightly different way:

```python
from turicreate._deps import pandas as pd

class YourCode():
    def __init__(self):
        pd.DataFrame({})
```

__caveat__: `import turicreate._deps.pandas as pd` won't work since `pandas` is not a module anymore but a global variable under package `turicreate._deps`'s namespace.
