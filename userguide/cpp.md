# Using the C++ API (Experimental)

Turi Create provides a C++ API to enable embedding into other applications or services, as a way to deploy models for inference, or to enable training in embedded scenarios where there is no Python interpreter. This consists of a set of headers; when those headers are consumed, the resulting library or application can be linked against the `libunity` provided by the `turicreate` package.

The [API documentation for the C++ API](https://apple.github.io/turicreate/docs/cpp/) provides a more complete reference of available functions, classes, and methods. Note that this API is not stable and may change at any time; plan to pin a particular version of Turi Create to your application or service to make use of this API.

##### Caveats

* This technique is experimental and depends on internal APIs within Turi Create that may change over time. The C++ API is not API-stable.
* In order to link properly, some build flags must be shared between Turi Create and your project.
* Not all functionality in the Turi Create Python package is available in the C++ API.

##### Setup

Using the C++ API requires building Turi Create from source, starting with a clone of the [repository](git@github.com:apple/turicreate.git). You should consider checking out a particular [release](https://github.com/apple/turicreate/releases) in order to keep API stability as you proceed (the API may change in master). Before proceeding to use the C++ API, run `./configure` to set up the build system for the project, and run `make -j8` from within `release/src/unity` to build the library artifacts that your application will depend on.

Currently, the C++ API is not packaged for easy consumption outside of the Turi Create Python package. As a result, some care is needed when compiling and linking against Turi Create. This may cause compatibility issues since those compilation and linking flags may not be compatible with flags used elsewhere. Required compilation flags for your project include:

* `-std=c++11` - some headers exposed by Turi Create require the use of C++11 or later.
* `-stdlib=libc++` - may be needed on macOS (probably not on Linux).
* Assuming <PATH_TO_TC> refers to the location where you cloned the Turi Create repository, the following directories must be added to the include paths with `-I`, and to the link paths with `-L`:
  * `-I<PATH_TO_TC>/turicreate/src`
  * `-I<PATH_TO_TC>/turicreate/src/external`
  * `-I<PATH_TO_TC>/turicreate/deps/local/include`
  * `-I<PATH_TO_TC>/turicreate/src/external/armadillo/include`
  * `-L<PATH_TO_TC>/turicreate/release/src/unity`
* Depending on your compiler toolchain, the following defines may be needed (note: these are autodetected for Turi Create itself with `./configure`, but the values must be passed on to your own build process):
* `-DHASH_FOR_UINT128_DEFINED` - set this if your compiler toolchain supports `unsigned __int128`.
* `-DHASH_FOR_INT128_DEFINED` - set this if your compiler toolchain supports `__int128`.
* `-lunity_shared` to link against `libunity_shared.so`.
* `<PATH_TO_TC>/turicreate/release/src/unity/libunity.a` to statically link libunity.a into your project. Note that this is needed in addition to the dynamic library listed above.

##### Example: make predictions in C++ from a model trained in Python

Let's assume we're starting with the model from the introductory example in the [boosted trees classifier](https://github.com/apple/turicreate/blob/3490286b27ff5d79cb90d09fe026d5671ce990c7/userguide/supervised-learning/boosted_trees_classifier.md#gradient-boosted-regression-trees) section of the user guide. The model is available as [mushroom.tcmodel.zip](https://github.com/apple/turicreate/files/1654991/mushroom.tcmodel.zip).

To make predictions from this model in Python, you could run:
```python
import turicreate as tc
data =  tc.SFrame.read_csv('turicreate/lfs/datasets/xgboost/mushroom.csv')
model = tc.load_model('mushroom.tcmodel')
data[0]
```
```
{'bruises?': 't',
 'cap-color': 'n',
 'cap-shape': 'x',
 'cap-surface': 's',
 'gill-attachment': 'f',
 'gill-color': 'k',
 'gill-size': 'n',
 'gill-spacing': 'c',
 'habitat': 'u',
 'label': 'c',
 'odor': 'p',
 'population': 's',
 'ring-number': 'o',
 'ring-type': 'p',
 'spore-print-color': 'k',
 'stalk-color-above-ring': 'w',
 'stalk-color-below-ring': 'w',
 'stalk-root': 'e',
 'stalk-shape': 'e',
 'stalk-surface-above-ring': 's',
 'stalk-surface-below-ring': 's',
 'veil-color': 'w',
 'veil-type': 'p'}
```
```python
model.predict_topk(data[0], k=2, output_type='probability')
```
```
Columns:
	id	int
	class	int
	probability	float

Rows: 2

Data:
+----+-------+---------------+
| id | class |  probability  |
+----+-------+---------------+
| 0  |   1   | 0.57558375597 |
| 0  |   0   | 0.42441624403 |
+----+-------+---------------+
[2 rows x 3 columns]
```

In C++, the equivalent code would be (minus the CSV parsing):
```cpp
// standard C++ includes
#include <iostream>

// Turi Create includes
#include <logger/assertions.hpp>
#include <logger/logger.hpp>
#include <unity/lib/unity_global.hpp>
#include <unity/server/registration.hpp>
#include <unity/toolkits/supervised_learning/boosted_trees.hpp>

int main(int argc, char **argv) {
  // set up registry of named functions and models
  // used for rehydrating saved models
  auto fn_reg = turi::toolkit_function_registry();
  turi::register_functions(fn_reg);
  auto class_reg = turi::toolkit_class_registry();
  turi::register_models(class_reg);

  // create a global context
  auto global = turi::unity_global(&fn_reg, &class_reg);

  // load the model from disk
  auto loaded_model = global.load_model("mushroom.tcmodel");
  std::string name = turi::variant_get_value<turi::flexible_type>(loaded_model["model_name"]);
  std::cout << "Loaded model: " << name << std::endl;
  auto model = std::dynamic_pointer_cast<turi::supervised::xgboost::boosted_trees_classifier>(
    turi::variant_get_value<std::shared_ptr<turi::model_base>>(loaded_model["model"])
  );

  // set up input
  // (note: can do batch prediction by adding more than one row to rows)
  std::vector<turi::flexible_type> rows;
  turi::flex_dict row;
  row.push_back(std::make_pair("bruises?", "t"));
  row.push_back(std::make_pair("cap-color", "n"));
  row.push_back(std::make_pair("cap-shape", "x"));
  row.push_back(std::make_pair("cap-surface", "s"));
  row.push_back(std::make_pair("gill-attachment", "f"));
  row.push_back(std::make_pair("gill-color", "k"));
  row.push_back(std::make_pair("gill-size", "n"));
  row.push_back(std::make_pair("gill-spacing", "c"));
  row.push_back(std::make_pair("habitat", "u"));
  row.push_back(std::make_pair("odor", "p"));
  row.push_back(std::make_pair("population", "s"));
  row.push_back(std::make_pair("ring-number", "o"));
  row.push_back(std::make_pair("ring-type", "p"));
  row.push_back(std::make_pair("spore-print-color", "k"));
  row.push_back(std::make_pair("stalk-color-above-ring", "w"));
  row.push_back(std::make_pair("stalk-color-below-ring", "w"));
  row.push_back(std::make_pair("stalk-root", "e"));
  row.push_back(std::make_pair("stalk-shape", "e"));
  row.push_back(std::make_pair("stalk-surface-above-ring", "s"));
  row.push_back(std::make_pair("stalk-surface-below-ring", "s"));
  row.push_back(std::make_pair("veil-color", "w"));
  row.push_back(std::make_pair("veil-type", "p"));
  rows.push_back(row);

  // make prediction
  turi::gl_sframe result = model->fast_predict_topk(
      // input data - 1 row per prediction
      rows,

      // output type - see https://github.com/apple/turicreate/blob/3490286b27ff5d79cb90d09fe026d5671ce990c7/src/unity/toolkits/supervised_learning/supervised_learning.hpp#L47
      "probability",

      // missing value action - see https://github.com/apple/turicreate/blob/3490286b27ff5d79cb90d09fe026d5671ce990c7/src/ml_data/ml_data_column_modes.hpp#L27
      "error",

      // k (returns top k predictions) -- must be <= the number of classes
      2
  );
  std::string row_str = rows[0].to<std::string>(); // turn first row of input into a string
  std::cout << "Input to model: " << row_str << std::endl;
  std::cout << "Predicted: " << result << std::endl;
}
```
And the output from C++ looks like:
```
Loaded model: boosted_trees_classifier
Input to model: {"bruises?":"t", "cap-color":"n", "cap-shape":"x", "cap-surface":"s", "gill-attachment":"f", "gill-color":"k", "gill-size":"n", "gill-spacing":"c", "habitat":"u", "odor":"p", "population":"s", "ring-number":"o", "ring-type":"p", "spore-print-color":"k", "stalk-color-above-ring":"w", "stalk-color-below-ring":"w", "stalk-root":"e", "stalk-shape":"e", "stalk-surface-above-ring":"s", "stalk-surface-below-ring":"s", "veil-color":"w", "veil-type":"p"}
Predicted: 
Columns:
    id	integer
    class	integer
    probability	float
Rows: 2
Data:
+----------------+----------------+----------------+
| id             | class          | probability    |
+----------------+----------------+----------------+
| 0              | 1              | 0.575584       |
| 0              | 0              | 0.424416       |
+----------------+----------------+----------------+
[2 rows x 3 columns]
```

To build this C++ program, you'll need to borrow the compilation flags from Turi Create as shown above.

```shell
# run the following in the turicreate repository root:
./configure
cd release/src/unity
make -j8

# And assuming predict.cpp is in a sibling directory of turicreate, run:
c++ -std=c++11 -stdlib=libc++ -I../turicreate/src -I../turicreate/src/external -I../turicreate/src/platform -I../turicreate/deps/local/include -I../turicreate/src/external/armadillo/include -DHASH_FOR_UINT128_DEFINED -DHASH_FOR_INT128_DEFINED -L../turicreate/release/src/unity -lunity_shared predict.cpp ../turicreate/release/src/unity/libunity.a
LD_LIBRARY_PATH=../turicreate/release/src/unity ./a.out
```

##### Additional Resources

Please refer to the [API documentation for the C++ API](https://apple.github.io/turicreate/docs/cpp/) for a more complete reference of available functions, classes, and methods.
