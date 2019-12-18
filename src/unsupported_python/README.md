An egg only needs to be uploaded to PyPI when we want to change the unsupported error message. If we're not changing what platforms or Python versions we support, we shouldn't upload an egg. 

To generate the egg:

* Update the value of `VERSION` in `setup.py`

* Run `python setup.py sdist`

The egg (a `.tar.gz` file) will be located in the `dist` folder.
