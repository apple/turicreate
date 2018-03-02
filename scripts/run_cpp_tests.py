#!/usr/bin/env python

import os
import time
import pickle
import fnmatch
import hashlib
import argparse
import subprocess

if __name__ == '__main__':
  parser = argparse.ArgumentParser(description='Run cxxtests that have been modified.')
  parser.add_argument('--cache-file', type=str, default='cache_for_cxxtests.pkl',
      help='Filename where the cache should be saved')
  parser.add_argument('-j', type=int, default=4,
      help='Number of processes to use for ctest command.')
  parser.add_argument('--dry-run', action='store_true',
      help='If present, the ctest command is printed rather than run.')

  args = parser.parse_args()

  # Get all cxxtests in directory
  matches = []
  for root, dirnames, filenames in os.walk('.'):
    for filename in fnmatch.filter(filenames, '*.cxxtest*'):
      matches.append(os.path.join(root, filename))
  print('Found {} tests.'.format(len(matches)))

  # Load the previous cache if it exists
  if os.path.exists(args.cache_file):
    cache = pickle.load(open(args.cache_file, 'rb'))
  else:
    cache = set()

  if type(cache) != set:
    print("Invalid cache contents. Resetting cache")
    cache = set()

  print('Found {} files in cache.'.format(len(cache)))

  # Hash each test binary.
  new_tests = {}
  start_time = time.time()
  for test_file in matches:
    hasher = hashlib.md5()
    with open(test_file, 'rb') as afile:
      buf = afile.read()
      hasher.update(buf)
    new_tests[test_file] = hasher.hexdigest()
  elapsed = time.time() - start_time
  print('Hashed {0} files in {1} seconds.'.format(len(new_tests), elapsed))

  # Make a list of tests whose hash does not appear in the cache
  tests = []
  for test_file in new_tests.keys():
      if new_tests[test_file] not in cache:
          tests.append(test_file)
  print('Ready to test {} files.'.format(len(tests)))

  # If there are no tests, pick a random one.
  if len(tests) == 0:
    print("For annoying reasons, we cannot handle running 0 tests because jenkins will complain. So we are running the first test")
    tests = [matches[0]]

  # Get basename and use .cxx rather than .cxxtest
  runtests = [test.split('/')[-1].replace('cxxtest','cxx') for test in tests]

  # Make the command to run
  cmd = 'ctest --output-on-failure -j {0} --schedule-random -R "({1})"'.format(args.j, '|'.join(runtests))
  if args.dry_run:
    print('Dry run requested! Proposed ctest command:', cmd)
    exit()

  exit_code = 0
  try: 
      ctest_output = subprocess.check_output(cmd, shell=True)
  except subprocess.CalledProcessError as e:
      ctest_output = e.output
      exit_code = e.returncode
  ctest_output = ctest_output.decode()
  print(ctest_output)
  # get all the lines which say "Passed"
  lines = [line for line in ctest_output.splitlines() if "Passed" in line]
  # go through all the tests and see if we have a "Passed" line matching it
  for i in range(len(tests)):
      for line in lines:
          if " " + runtests[i] + " " in line:
              # pass!
              cache.add(new_tests[tests[i]])

  # Save to cache
  pickle.dump(cache, open(args.cache_file, "wb" ))

  exit(exit_code)
