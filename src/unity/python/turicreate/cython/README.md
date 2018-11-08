# Cython 

### Table of Contents
- The Unity Server
- The Extensions Mechanism

## The Extensions Mechanism

#### Motivation

The TuriCreate Framework uses Cython as the glue between the Python and the C++ library. Cython users, for example can create C++ interfaces to header files to call classes, and functions. A secondary Proxy, or wrapper class is used to expose functionality to Python. As you can imagine, this can be a very cumbersome process. Every time a new C++ class or function is created, a Cython interface must be written to make it’s functionality accessible in Python.

Fortunately the extensions mechanism is a unique solution to this problem. The extensions mechanism is a high level abstraction on top of Cython, that provides a simple way of exposing C++ functions, and classes to python without explicitly having to write any Cython glue code to do so.

#### How do I use it?

// TODO

#### How does it work?

There are a number of different pieces that enable the extensions mechanism to work. Let’s start on the Python side and plumb our way through to the underlying registration code. The code snippet we are interested in is shown below:

```python
class _extensions_wrapper(object):
  def __init__(self, wrapped):
    self._wrapped = wrapped
    self.__doc__ = wrapped.__doc__

  def __getattr__(self, name):
    try:
        return getattr(self._wrapped, name)
    except:
        pass
    turicreate.connect.main.get_unity()
    return getattr(self._wrapped, name)
```
file: `src/unity/python/turicreate/__init__.py`

This code defines an `extensions_wrapper` class in python. There's an initialization method which takes in a parameter and a built in function named `__getattr__`. This function allows python to call methods, or attributes on an object. For example, the two statements are equivalent.

```python
class Example:
    value = 23

example = Example()

# 
print(getattr(example, "value"))
print(example.value)
```

This is the first piece of the system that allows TuriCreate to define attributes dynamically.

The second piece is to use the `_extensions_wrapper` to rewrite the `turicreate.extensions` import using `sys.modules`. `sys.modules` is a dictionary that maps module names to modules which have already been loaded, and can be manipulated to force reloading of modules. The code for this is shown below:


```python
import sys as _sys
_sys.modules["turicreate.extensions"] = _extensions_wrapper(_sys.modules["turicreate.extensions"])
# rewrite the import
extensions = _sys.modules["turicreate.extensions"]
```
file: `src/unity/python/turicreate/__init__.py`

Initially when both code snippets are run, the extensions aren't populated. For this the unity server must be started. The unity server is a C++ server that dynamically registers classes, and functions. It loads shared libraries and registers symbols. Read more about it in the Unity Server section here.

The third piece that allows the extensions mechanism to work is:

```python
_launch()
```
file: `src/unity/python/turicreate/__init__.py`

This starts the Unity Server; to see what `_launch` does we have to go to the code snippet below:

```python
def launch(server_log=None):
    global __UNITY_GLOBAL_PROXY__
    global __SERVER__

    server = EmbeddedServer(server_log)
    server.start()
    server.set_log_progress(True)

   	...


```
file: `src/unity/python/turicreate/connect/main.py`

There's a lot going on in this powerful function. First an `EmbeddedServer` is started. This is a Cython import, and the class is defined in the file `src/unity/python/turicreate/cython/cy_server.pyx`. Once instantiated, the `start()` method is called:

```python
def start(self): 
...
    # Try starting the server
    start_server(server_opts)
...
```
file: `src/unity/python/turicreate/cython/cy_server.pyx`

In this start method, a couple of options are initialized and the `start_server` function is called. This function can be found in `src/unity/server/unity_server_control.hpp`

```cpp
static unity_server* SERVER = nullptr;

EXPORT void start_server(const unity_server_options& server_options,
                         const unity_server_initializer& server_initializer) {
...
  SERVER = new unity_server(server_options);
  SERVER->start(server_initializer);
}

```
file: `src/unity/server/unity_server_control.hpp`

A new `unity_server` object is instantiated and the start method on this object is called. The constructor of this object is defined below:

```cpp
unity_server::unity_server(unity_server_options options) : options(options) {
  toolkit_functions = new toolkit_function_registry();
  toolkit_classes = new toolkit_class_registry();
}
```
file: `src/unity/server/unity_server.cpp`

The `toolkit_function_registry` takes care of the registration of the functions while the `toolkit_class_registry` takes care of the registration of the classes.

The `toolkit_function_registry` is defined in `src/unity/lib/toolkit_function_registry.hpp`
The `toolkit_class_registry` is defined in `src/unity/lib/toolkit_class_registry.hpp`

Let's take a look at the bit of the `toolkit_function_registry`:

```cpp
class toolkit_function_registry {
 private:
  std::map<std::string, toolkit_function_specification> registry;
	
	...

};
```
file: `src/unity/lib/toolkit_function_registry.cpp`

It contains a member variable which is a map from a string or name to a `toolkit_function_specification`. This toolkit function specification is the struct that holds the definition for a toolkit function. When we look at it's definition in `src/unity/lib/toolkit_function_specification.hpp` we see that it contains:

```cpp
struct toolkit_function_specification {
  std::string name;
  ...
  std::function<toolkit_function_response_type(toolkit_function_invocation&)> toolkit_execute_function;
  ...
};
```
file: `src/unity/lib/toolkit_function_specification.hpp`

These two member variables store the name of the function and an `std::function` polymorphic function wrapper respectively. A polymorphic function wrapper can store, copy, and invoke any Callable target -- functions, lambda expressions, bind expressions, or other function objects, as well as pointers to member functions and pointers to data members. This is how functions are store in the TuriCreate Extensions Mechanism.

Going back to `src/unity/server/unity_server.cpp` though. The constructor is called and both the `toolkit_functions` and the `toolkit_classes` variables aren't populated. They don't contain any function or class definitions yet. This is where the start method comes in.


```cpp
void unity_server::start(const unity_server_initializer& server_initializer) {
...
  server_initializer.init_toolkits(*toolkit_functions);
  server_initializer.init_models(*toolkit_classes);
...
}
```
file: `src/unity/server/unity_server.cpp`

The start method calls the `init_toolkits` and `init_models` functions on the `unity_server_initializer` class instance. These methods indirectly populate the `toolkit_functions` and the `toolkit_classes` variables respectively.

```cpp
void unity_server_initializer::init_toolkits(toolkit_function_registry& registry) const {
  register_functions(registry);
};

void unity_server_initializer::init_models(toolkit_class_registry& registry) const {
  register_models(registry);
};
```
file: `src/unity/server/unity_server_init.cpp`

These's methods call the `register_functions` and `register_models` functions respectively. And these functions are defined in `src/unity/server/registration.cpp`. For example `register_functions` registers the toolkit functions as shown below. Let's go through the supervised_learning toolkit function registration as an example of how this works.


```cpp
void register_functions(toolkit_function_registry& registry) {
	...
	registry.register_toolkit_function(turi::supervised::get_toolkit_function_registration());
	...
}
```
file: `src/unity/server/registration.cpp`

In the `turi::supervised` namespace a `get_toolkit_function_registration` function is defined in the file: `src/unity/toolkits/supervised_learning/class_registrations.hpp`.

When we look at the implementation we see a macro:

```cpp
BEGIN_FUNCTION_REGISTRATION
...
REGISTER_FUNCTION(create_automatic_classifier_model, "data", "target", "validation_data", "options");
...
END_FUNCTION_REGISTRATION
```
file: `src/unity/toolkits/supervised_learning/class_registrations.cpp`

The first macro `BEGIN_FUNCTION_REGISTRATION` is defined in `src/unity/lib/toolkit_function_macros.hpp`

```cpp
#define BEGIN_FUNCTION_REGISTRATION                         \
  __attribute__((visibility("default")))                  \
      std::vector<::turi::toolkit_function_specification> \
      get_toolkit_function_registration() {               \
     std::vector<::turi::toolkit_function_specification> specs;
```
file: `src/unity/lib/toolkit_function_macros.hpp`

As you can see, it just is the beginning of the implementation of the `get_toolkit_function_registration` function we placed in the header file. It also defines an `std::vector` with the type `toolkit_function_specification`. The same struct we saw previously.

The last macro `END_FUNCTION_REGISTRATION` is also defined in `src/unity/lib/toolkit_function_macros.hpp`


```cpp
#define END_FUNCTION_REGISTRATION  \
     return specs; \
   }

```
file: `src/unity/lib/toolkit_function_macros.hpp`

As you can see, it contains a return statement and a closing brace for the function implementation.

The macros in between the `BEGIN_FUNCTION_REGISTRATION` and `END_FUNCTION_REGISTRATION` actually register the functions as the name `REGISTER_FUNCTION` implies. This means populating the `std::vector` we defined in `BEGIN_FUNCTION_REGISTRATION` with `toolkit_function_specification` instances.

```cpp
#define REGISTER_FUNCTION(function, ...) \
     specs.push_back(toolkit_function_wrapper_impl::make_spec_indirect(function, #function, \
                                                                      ##__VA_ARGS__));
```
file: `src/unity/lib/toolkit_function_macros.hpp`

The `make_spec_indirect` is a template function that generates the `toolkit_function_specification`. It's defined in `toolkit_function_wrapper_impl.hpp`.

```cpp
template <typename Function, typename... Args>
toolkit_function_specification make_spec_indirect(Function fn, std::string fnname,
                                         Args... args) {
  // Get a function traits object from the function type
  typedef boost::function_traits<typename std::remove_pointer<Function>::type> fntraits;
  static_assert(fntraits::arity == sizeof...(Args), 
                "Incorrect number input parameter names specified.");
  return make_spec<sizeof...(Args)>(fn, fnname, {args...});
}
```
file: `src/unity/lib/toolkit_function_wrapper_impl.hpp`

As you can see this function is a thin wrapper around another template function `make_spec` which actually generates the spec. This function is shown below:

```cpp
template <size_t NumInArgs, typename Function>
toolkit_function_specification make_spec(Function fn, std::string fnname,
                                std::vector<std::string> inargnames) {
  // the actual spec object we are returning
  toolkit_function_specification spec;
  auto fnwrapper = generate_function_wrapper<NumInArgs, Function>(fn, inargnames);
  auto native_fn_wrapper = generate_native_function_wrapper<NumInArgs, Function>(fn);
  
  auto invoke_fn = [fnwrapper,inargnames](turi::toolkit_function_invocation& invoke)->turi::toolkit_function_response_type {
        turi::toolkit_function_response_type ret;
        // we are inside the actual toolkit call now.
        try {
          variant_type val = fnwrapper(invoke.params);
          ret.params["return_value"] = val;
          ret.success = true;
          // final bit of error handling in the case of exceptions
        } catch (std::string err) {
          ret.message = err;
          ret.success = false;
        } catch (const char* err) {
          ret.message = err;
          ret.success = false;
        } catch (std::exception& e) {
          ret.message = e.what();
          ret.success = false;
        } catch (...) {
          ret.message = "Unknown Exception";
          ret.success = false;
        }
        return ret;
      };

  // complete the function registration.
  // get the python exposed function name. Remove any namespacing is any.
  auto last_colon = fnname.find_last_of(":");
  if (last_colon == std::string::npos) spec.name = fnname;
  else spec.name = fnname.substr(last_colon + 1);

  // store the function 
  spec.toolkit_execute_function = invoke_fn; 
  spec.native_execute_function = native_fn_wrapper;
  spec.description["arguments"] = 
      flexible_type_converter<decltype(inargnames)>().set(inargnames);
  spec.description["_raw_fn_pointer_"] = reinterpret_cast<size_t>(fn);

  return spec;
}
```
file: `src/unity/lib/toolkit_function_wrapper_impl.hpp`

This is the base function that creates the `toolkit_function_specification`. The functions are now registered in the `unity_server`. A similar registration mechanism is used to register the classes in `unity_server` as well.

Now that the server has launched and the classes and functions have been registered. A `UnityGlobalProxy` class is instantiated. It contains pointers to the `toolkit_function_registry` and `toolkit_class_registry` as well as lambda workers and various other extensions that TuriCreate uses.

```python
def launch(server_log=None):
   `...
    __UNITY_GLOBAL_PROXY__ = UnityGlobalProxy()
    __SERVER__ = server
    ...
```
file: `src/unity/python/turicreate/connect/main.py`

Finally publishing the registered extensions back into the Python `turicreate.extensions` module requires a call to `_publish()`

```python
def launch(server_log=None):
   `...
    from ..extensions import _publish
    ...
    _publish()
    ...

``` 
file: `src/unity/python/turicreate/connect/main.py`

This function is located in `src/unity/python/turicreate/extensions.py`

```python
def _publish():
	...
    fnlist = unity.list_toolkit_functions()
    
    for fn in fnlist:
       	...
        newfunc = _make_injected_function(fn, arguments)
        ...
        mod = _thismodule
        ...
        _setattr_wrapper(mod, modpath[-1], newfunc)
```
file: `src/unity/python/turicreate/extensions.py`

We first get a list of all of the toolkit functions. Then we make an injected function using the `_make_injected_function` and that returns a lambda function with `_run_toolkit_function`. 

```python

def _make_injected_function(fn, arguments):
    return lambda *args, **kwargs: _run_toolkit_function(fn, arguments, args, kwargs)
```
file: `src/unity/python/turicreate/extensions.py`


The `_run_toolkit_function` puts all of the arguments in the proper format and calls the Cython to actually run the `std::function`.

```python
def _run_toolkit_function(fnname, arguments, args, kwargs):
	...
    argument_dict = {}
    for i in range(len(args)):
        argument_dict[arguments[i]] = args[i]
    ...
    with cython_context():
        ret = _get_unity().run_toolkit(fnname, argument_dict)

    ...
    return ret
```
file: `src/unity/python/turicreate/extensions.py`

Lastly we call the `_setattr_wrapper` to add the extensions function to the Python `turicreate.extensions` module, using the `setattr` function in python. Instead of getting an attribute, variable or method like the `getattr` the `setattr` sets this value.

```python
def _setattr_wrapper(mod, key, value):
    setattr(mod, key, value)
    if mod == _thismodule:
        setattr(_sys.modules[__name__], key, value)
```
file: `src/unity/python/turicreate/extensions.py`

A series of Cython and Python files, registration mechanisms, template functions, structs and classes allow you to easily register your classes and functions using simple macros. Hopefully this deep dive has allowed you to understand how this complex, yet very useful mechanism works.
