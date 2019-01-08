# Scenario Tests

The scenario test framework is minimal and only spans Operating Systems.
i.e. it only spans
 - OSX 10.11
 - Ubuntu 14.04

Important things to know:

### Test Driver
The test driver is pytest. All tests on the same operating system are run sequentially.

### We do not have test selectors
All tests run everywhere. It is up to the test to decide it is "skipping" an
operating system. (For instance, numpy integration tests should be "skipped" on
Windows because it is not supported).

If tests are skipped, they should generate a pytest "skip" output. i.e.
raise unittest.SkipTest(...) or one of the other pytest decorators.
It should not simply silently succeed.

# Writing a Test
All tests are written in Python. The Python tests are written as
[unittests](https://docs.python.org/2/library/unittest.html), and can use any
of the methods available to a Python unit test.

### Create a Subdirectory
Create a subdirectory in the scenario-tests directory.

Every python file in the sub directory will be executed against pytest.

Optionally, a setup.py file may exist inside the subdirectory in which case
setup.py is effectively "sourced" before any of the tests are run. This allows
setup.py to modify environment variables, install packages, etc which will be
picked up by the tests.

For instance, if the PATH variable is changed in setup.py a different python
environment may be used to run pytest

To teardown, a "teardown.py" may also exist to undo any changes made by
setup.p.y

### To run all the tests in the scenario-testing environment

    ./run_scenario_tests.sh path_to_test_egg [other named options]

### To run all the tests (in your own environment -- YMMV)

    python driver.py

## Important Notes on Test Writing
If your tests depend on directory layout and creation of new 
directories, some things are good to keep in mind.

 - setup.py is run with current working directory in tests/ . NOT in your test directory as you might expect.
 - your test should never create new directories in tests/ . That will cause issues as
   they may get picked up as test directories.
