#!/usr/bin/env python

import os
import sys
import time
import pickle
import fnmatch
import hashlib
import argparse
import subprocess

# The build image version that will be used for testing
SCRIPT_DIR=os.path.dirname(__file__)
WORKSPACE=os.path.abspath(os.path.join(SCRIPT_DIR, '..'))
TC_BUILD_IMAGE_1004=subprocess.check_output(['bash', os.path.join(WORKSPACE, 'scripts', 'get_docker_image.sh'), '--ubuntu=10.04']).strip()

def run_in_docker(cmd, workdir='/build'):
    if not(isinstance(cmd, list)):
      cmd = [cmd]
    subprocess.check_call(['docker', 'run', '--rm', '-m=4g',
        '--mount', 'type=bind,source=' + WORKSPACE + ',target=/build,consistency=delegated',
        '-w=%s' % workdir,
        TC_BUILD_IMAGE_1004] + cmd)

if __name__ == '__main__':
  parser = argparse.ArgumentParser(
    description='Run cxxtests (optionally caching those that have not been modified).')

  parser.add_argument('--cache', dest='cache', action='store_true')
  parser.add_argument('--no-cache', dest='cache', action='store_false')
  parser.set_defaults(cache=True)

  parser.add_argument('--schedule-random', dest='schedule_random', action='store_true')
  parser.add_argument('--no-schedule-random', dest='schedule_random', action='store_false')
  parser.set_defaults(schedule_random=True)

  parser.add_argument('--skip-expensive', default=False, action='store_true')
  parser.add_argument('--only-expensive', default=False, action='store_true')

  parser.add_argument('--cache-file', type=str, default='cache_for_cxxtests.pkl',
      help='Filename where the cache should be saved')
  parser.add_argument('-j', type=int, default=4,
      help='Number of processes to use for ctest command.')
  parser.add_argument('--dry-run', action='store_true',
      help='If present, the ctest command is printed rather than run.')
  parser.add_argument('--docker', action='store_true',
      help='Run the C++ tests inside of Docker on Ubuntu 10.04.')

  args = parser.parse_args()

  if args.docker:
    print('Docker run requested! Proceeding to run inside Docker.')

    # create docker images if needed
    subprocess.check_call(['bash', os.path.join(WORKSPACE, 'scripts/create_docker_images.sh')])

    # make tests if needed
    run_in_docker(['bash', 'configure']) # TODO use --no-python when it works again
    run_in_docker(['make', '-j%d' % args.j], '/build/release/test')

    # run tests
    # TODO pass through other arguments
    run_in_docker(['python', '/build/scripts/run_cpp_tests.py', '-j%d' % args.j], '/build/release/test')

    # exit if successful (if failed, it will have thrown above)
    sys.exit(0)

  expensive_tests = [
    'boosted_trees_classifier_tests.cxxtest',
    'worker_pool_test.cxxtest',
    'sframe_test.cxxtest',
    'shuffle_test.cxxtest',
    'sarray_file_format_v2_test.cxxtest',
    'parallel_sframe_iterator.cxxtest',
    'optimizations.cxxtest',
    'sorting_and_blocks.cxxtest',
    'test_brute_force_all_pairs.cxxtest',
    'gl_string.cxxtest',
    'process_launch_test.cxxtest',
  ]

  # Get all cxxtests in directory
  matches = []
  for root, dirnames, filenames in os.walk('.'):
    for filename in fnmatch.filter(filenames, '*.cxxtest*'):
      if args.skip_expensive and (filename in expensive_tests):
        continue
      if args.only_expensive and (filename not in expensive_tests):
        continue
      matches.append(os.path.join(root, filename))
  print('Found {} tests.'.format(len(matches)))

  if args.cache:
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
  else:
    tests = list(matches)

  print('Ready to test {} files.'.format(len(tests)))

  # If there are no tests, pick a random one.
  if len(tests) == 0:
    print("For annoying reasons, we cannot handle running 0 tests because jenkins will complain. So we are running the first test")
    tests = [matches[0]]

  # Get basename and use .cxx rather than .cxxtest
  runtests = [test.split('/')[-1].replace('cxxtest','cxx') for test in tests]

  # Make the command to run
  cmd = [
    'ctest',
    '--output-on-failure',
    '-j',
    str(args.j),
  ]

  if args.schedule_random:
    cmd.append('--schedule-random')

  cmd += [
    '-R',
    '({})'.format('|'.join(runtests)),
  ]

  if args.dry_run:
    print('Dry run requested! Proposed ctest command:', ' '.join(cmd))
    exit()

  exit_code = 0

  ctest_process = subprocess.Popen(cmd, stdout=subprocess.PIPE)

  lines = []
  while True:
    line = ctest_process.stdout.readline()
    if len(line) == 0:
      break
    sys.stdout.write(line)
    sys.stdout.flush()
    lines.append(line)

  out, err = ctest_process.communicate()

  if args.cache:
    # go through all the tests and see if we have a "Passed" line matching it
    for i in xrange(len(tests)):
      for line in lines:
        if ('Passed' in line) and ((" " + runtests[i] + " ") in line):
          # pass!
          cache.add(new_tests[tests[i]])

    # Save to cache
    pickle.dump(cache, open(args.cache_file, "wb"))

  exit(ctest_process.returncode)
