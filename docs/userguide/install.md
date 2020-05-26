# Getting Started

We recommend using virtualenv to use, install, or build Turi Create.  Be
sure to install virtualenv using your system pip.

```shell
pip install virtualenv
```

The method for installing *Turi Create* follows the [standard python
package installation steps](https://packaging.python.org/installing/).

```shell
cd ~
virtualenv venv

# Active your virtual environment
source pythonenv/bin/activate
```

To install Turi Create in the new virtual environment, do the following:

```shell
pip install -U turicreate
```

For more detailed installation instruction, please refer to the
[README.md](https://github.com/apple/turicreate/README.md)


Once installed, you will be able to read or run the example code and
datasets by copying code into your terminal. Throughout the User Guide
we assume you have imported the package via

```python
import turicreate
```

Sometimes it is useful shorthand to use `tc` instead of `turicreate`.
This can be done via

```python
import turicreate as tc
```
