# -*- coding: utf-8 -*-`
# system-wide imports
import os
import sys

# local imports
from setup_reporter import try_execfile

def print_help():
    print("""\
%s [sub_directory] [Optional output xunit xml path prefix]

This test runs the scenario test located in the sub directory.
Every python file in the sub directory will be executed against pytest.

Optionally, a setup.py file may exist inside the subdirectory in which case
setup.py is effectively "sourced" before any of the tests are run. This allows
setup.py to modify environment variables which will be picked up by the
tests.

For instance, if the PATH variable is changed in setup.py a different python
environment may be used to run pytest
""" % sys.argv[0])

# set PYTHONPATH to include `common` so that test files can include shared code.
common_dir = os.path.join(os.getcwd(), '..', 'common')
if 'PYTHONPATH' in os.environ:
    os.environ['PYTHONPATH'] = '%s:%s' % (os.environ['PYTHONPATH'], common_dir)
else:
    os.environ['PYTHONPATH'] = common_dir

sys.path.append(common_dir)

def process_directory(test_path, xml_prefix):
    """
    Recursively processes tests in the directory `test_path`, with the following
    logic:

    * Run setup.py
    * Run tests
    * Recurse into subdirectory
    * Run teardown.py

    Note that inner-directory setup/teardown are run nested in between outer-
    directory setup/teardown.
    """
    # cleanup sub-directory. drop pyc
    for f in os.listdir(test_path):
        if f.endswith(".pyc"):
            try:
                os.remove(f)
            except:
                pass

    # run the setup file if there is one
    try_execfile(test_path, 'setup.py')

    # go through all the tests
    exit_code = 0
    for sub_test in os.listdir(test_path):
        if sub_test != "setup.py" and sub_test != "teardown.py" and sub_test.endswith(".py") and not sub_test.startswith("."):
            xml_path = xml_prefix + ".{}.xml".format(sub_test)
            sub_test_py = os.path.join(test_path, sub_test)
            # we may have to something a little different here for windows
            cmd = "pytest -v -s --junit-xml=\"{}\" \"{}\"".format(xml_path, sub_test_py)
            print(cmd)
            test_exit_code = os.system(cmd)
            exit_code |= test_exit_code

    for d in os.listdir(test_path):
        d = os.path.join(test_path, d)
        if os.path.isdir(d):
            sub_dir_exit_code = process_directory(d, xml_prefix)
            exit_code |= sub_dir_exit_code

    # run the setup file if there is one
    try_execfile(test_path, 'teardown.py')

    return exit_code

def main():
    if len(sys.argv) < 2 or sys.argv[1] == '-h' or sys.argv[1] == '--help':
        print_help()
        exit(0)
    test_path = sys.argv[1]
    if len(sys.argv) < 3:
        xml_prefix = "tests"
    else:
        xml_prefix = sys.argv[2]

    exit_code = process_directory(test_path, xml_prefix)

    if exit_code != 0:
        exit(1)

if __name__ == '__main__':
    main()
