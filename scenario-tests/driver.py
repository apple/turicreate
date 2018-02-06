# -*- coding: utf-8 -*-`
import os
import stat
import re


include_regex = os.environ.get('TEST_INCLUDE_REGEX', '')
exclude_regex = os.environ.get('TEST_EXCLUDE_REGEX', '')

# precompile the regexes if available
include_compile = None
exclude_compile = None
try:
    if len(include_regex) > 0:
        include_compile = re.compile(include_regex)
except:
    pass

try:
    if len(exclude_regex) > 0:
        exclude_compile = re.compile(exclude_regex)
except:
    pass

os.chdir(os.path.join(os.getcwd(), 'tests'))
test_path = os.getcwd()

# remove all ".xml" files
for f in os.listdir(test_path):
    if f.endswith(".xml"):
        try:
            os.remove(f)
        except:
            pass

# go through all the tests
exit_code = 0
for test in os.listdir(test_path):
    # skip if it is not a directory
    # skip things beginning with "." they are probably editor temp files
    # or .git, or something like that
    # tests starting with _ are disabled
    try:
        if test.startswith(".") or test.startswith("_") or not stat.S_ISDIR(os.stat(test).st_mode):
            continue

        if exclude_compile and exclude_compile.match(test):
            continue

        if include_compile and not include_compile.match(test):
            continue
    except:
        # if file does not exist, stat will raise an exception. which is kind of
        # annoying. one might expect S_ISDIR to just return False. But no.....
        continue
    print("============================================================================")
    print(test)
    print("============================================================================")
    cmd = 'python single_test_driver.py "{}" "{}"'.format(test, test)
    print(cmd)
    # trigger test in subdirectory
    subprocess_exit_code = os.system(cmd)

    # error handling -- fail parent process if a test subprocess fails
    if subprocess_exit_code != 0:
        print("****************************************************************************")
        print("Non-zero process return: %s" % test)
        print("****************************************************************************")

        exit_code = 1

exit(exit_code)
