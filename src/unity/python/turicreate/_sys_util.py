# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import sys
import os
import logging
from distutils.util import get_platform as _get_platform
import ctypes
import glob as _glob
import subprocess as _subprocess
from ._scripts import _pylambda_worker
from copy import copy
from .util import sys_info as _sys_info

if sys.version_info.major == 2:
    import ConfigParser as _ConfigParser
else:
    import configparser as _ConfigParser

def make_unity_server_env():
    """
    Returns the environment for unity_server.

    The environment is necessary to start the unity_server
    by setting the proper environments for shared libraries,
    hadoop classpath, and module search paths for python lambda workers.

    The environment has 3 components:
    1. CLASSPATH, contains hadoop class path
    2. __GL_PYTHON_EXECUTABLE__, path to the python executable
    3. __GL_PYLAMBDA_SCRIPT__, path to the lambda worker executable
    4. __GL_SYS_PATH__: contains the python sys.path of the interpreter
    """
    env = os.environ.copy()

    # Add hadoop class path
    classpath = get_hadoop_class_path()
    if ("CLASSPATH" in env):
        env["CLASSPATH"] = env['CLASSPATH'] + (os.path.pathsep + classpath if classpath != '' else '')
    else:
        env["CLASSPATH"] = classpath

    # Add python syspath
    env['__GL_SYS_PATH__'] = (os.path.pathsep).join(sys.path + [os.getcwd()])

    # Add the python executable to the runtime config
    env['__GL_PYTHON_EXECUTABLE__'] = os.path.abspath(sys.executable)

    # Add the pylambda execution script to the runtime config
    env['__GL_PYLAMBDA_SCRIPT__'] = os.path.abspath(_pylambda_worker.__file__)

    #### Remove PYTHONEXECUTABLE ####
    # Anaconda overwrites this environment variable
    # which forces all python sub-processes to use the same binary.
    # When using virtualenv with ipython (which is outside virtualenv),
    # all subprocess launched under unity_server will use the
    # conda binary outside of virtualenv, which lacks the access
    # to all packages installed inside virtualenv.
    if 'PYTHONEXECUTABLE' in env:
        del env['PYTHONEXECUTABLE']

    # Set mxnet envvars
    if 'MXNET_CPU_WORKER_NTHREADS' not in env:
        num_workers = min(2, int(env.get('OMP_NUM_THREADS', _sys_info.NUM_CPUS)))
        env['MXNET_CPU_WORKER_NTHREADS'] = str(num_workers)

    ## set local to be c standard so that unity_server will run ##
    env['LC_ALL']='C'
    # add certificate file
    if 'TURI_FILEIO_ALTERNATIVE_SSL_CERT_FILE' not in env and \
            'TURI_FILEIO_ALTERNATIVE_SSL_CERT_DIR' not in env:
        try:
            import certifi
            env['TURI_FILEIO_ALTERNATIVE_SSL_CERT_FILE'] = certifi.where()
            env['TURI_FILEIO_ALTERNATIVE_SSL_CERT_DIR'] = ""
        except:
            pass
    return env

def set_windows_dll_path():
    """
    Sets the dll load path so that things are resolved correctly.
    """

    lib_path = os.path.dirname(os.path.abspath(_pylambda_worker.__file__))
    lib_path = os.path.abspath(os.path.join(lib_path, os.pardir))

    def errcheck_bool(result, func, args):
        if not result:
            last_error = ctypes.get_last_error()
            if last_error != 0:
                raise ctypes.WinError(last_error)
            else:
                raise OSError
        return args

    # Also need to set the dll loading directory to the main
    # folder so windows attempts to load all DLLs from this
    # directory.
    import ctypes.wintypes as wintypes

    try:
        kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
        kernel32.SetDllDirectoryW.errcheck = errcheck_bool
        kernel32.SetDllDirectoryW.argtypes = (wintypes.LPCWSTR,)
        kernel32.SetDllDirectoryW(lib_path)
    except Exception as e:
        logging.getLogger(__name__).warning(
            "Error setting DLL load orders: %s (things should still work)." % str(e))


def get_current_platform_dll_extension():
    """
    Return the dynamic loading library extension for the current platform
    """
    if sys.platform == 'win32':
        return 'dll'
    elif sys.platform == 'darwin':
        return 'dylib'
    else:
        return 'so'


def test_pylambda_worker():
    """
    Tests the pylambda workers by spawning off a separate python
    process in order to print out additional diagnostic information
    in case there is an error.
    """

    import os

    environment = os.environ.copy()
    
    from os.path import join
    from os.path import exists
    import tempfile
    import subprocess
    import datetime
    import time
    import zipfile
    import sys
    # change the temp directory to /tmp.
    # Otherwise we get interesting zeromq "too long file name" issues.
    if sys.platform == 'darwin':
        if exists('/tmp'):
            tempfile.tempdir = '/tmp'

    temp_dir = tempfile.mkdtemp()

    temp_dir_sim = join(temp_dir, "simulated")
    os.mkdir(temp_dir_sim)    
    lambda_log_file_sym = join(temp_dir_sim, "lambda_log")

    # Dump the directory structure.
    print("\nGathering installation information.")
    dir_structure_file = join(temp_dir, "dir_structure.log")
    dir_structure_out = open(dir_structure_file, "w")
    dump_directory_structure(dir_structure_out)
    dir_structure_out.close()
    
    print("\nRunning simulation.")
    
    env=make_unity_server_env()
    env["TURI_LAMBDA_WORKER_DEBUG_MODE"] = "1"
    env["TURI_LAMBDA_WORKER_LOG_FILE"] = lambda_log_file_sym
    
    proc = subprocess.Popen(
            [sys.executable, os.path.abspath(_pylambda_worker.__file__)],
            env = env)
    
    proc.wait()

    ################################################################################

    # Write out the current system path.
    open(join(temp_dir, "sys_path_1.log"), "w").write(
        "\n".join("  sys.path[%d] = %s. " % (i, p) for i, p in enumerate(sys.path)))

    # Now run the program
    print("\nRunning full lambda worker process")

    trial_temp_dir = join(temp_dir, "full_run")
    os.mkdir(trial_temp_dir)
    lambda_log_file_run = join(trial_temp_dir, "lambda_log.log")
    
    run_temp_dir = join(trial_temp_dir, "run_temp_dir")
    os.mkdir(run_temp_dir)

    run_temp_dir_copy = join(temp_dir, "run_temp_dir_copy")
    
    run_info_dict = {
        "lambda_log" : lambda_log_file_run,
        "temp_dir" : trial_temp_dir,
        "run_temp_dir" : run_temp_dir,
        "preserved_temp_dir" : run_temp_dir_copy,
        "runtime_log" : join(trial_temp_dir, "runtime.log"),
        "sys_path_log" : join(trial_temp_dir, "sys_path_2.log")}
        
    run_script = r"""
import os
import traceback
import shutil
import sys
import glob
from os.path import join

def write_exception(e):
    ex_str = "\n\nException: \n"
    traceback_str = traceback.format_exc()

    try:
        ex_str += repr(e)
    except Exception as e:
        ex_str += "Error expressing exception as string."

    ex_str += ": \n" + traceback_str

    try:
        sys.stderr.write(ex_str + "\n")
        sys.stderr.flush()
    except:
        # Pretty much nothing we can do here.
        pass

# Set the system path.
system_path = os.environ.get("__GL_SYS_PATH__", "")
del sys.path[:]
sys.path.extend(p.strip() for p in system_path.split(os.pathsep) if p.strip())

try:
    open(r"%(sys_path_log)s", "w").write(
         "\n".join("  sys.path[%%d] = %%s. " %% (i, p)
         for i, p in enumerate(sys.path)))
except Exception as e:
    write_exception(e)


os.environ["TURI_LAMBDA_WORKER_DEBUG_MODE"] = "1"
os.environ["TURI_LAMBDA_WORKER_LOG_FILE"] = r"%(lambda_log)s"
os.environ["TURI_CACHE_FILE_LOCATIONS"] = r"%(run_temp_dir)s"
os.environ["OMP_NUM_THREADS"] = "1"

try:
    import sframe
except Exception as e:
    write_exception(e)

    try:
        import turicreate as sframe
    except Exception as e:
        write_exception(e)
        sys.exit(55)

log_file = open(r"%(runtime_log)s", "w")
for k, v in sframe.get_runtime_config().items():
    log_file.write("%%s : %%s\n" %% (str(k), str(v)))
log_file.close()

try:
    sa = sframe.SArray(range(1000))
except Exception as e:
    write_exception(e)
    
try:
    print("Sum = %%d" %% (sa.apply(lambda x: x).sum()))
except Exception as e:
    write_exception(e)

new_dirs = []
copy_files = []
for root, dirs, files in os.walk(r"%(run_temp_dir)s"):
    new_dirs += [join(root, d) for d in dirs]
    copy_files += [join(root, name) for name in files]

def translate_name(d):
    return os.path.abspath(join(r"%(preserved_temp_dir)s",
                                os.path.relpath(d, r"%(run_temp_dir)s")))

for d in new_dirs:
    try:
        os.makedirs(translate_name(d))
    except Exception as e:
        sys.stderr.write("Error with: " + d)
        write_exception(e)
    
for f in copy_files:
    try:
        shutil.copy(f, translate_name(f))
    except Exception as e:
        sys.stderr.write("Error with: " + f)
        write_exception(e)

server_logs = (glob.glob(sframe.util.get_server_log_location() + "*")
               + glob.glob(sframe.util.get_client_log_location() + "*"))

for f in server_logs:
    try:
        shutil.copy(f, join(r"%(temp_dir)s", os.path.split(f)[1]))
    except Exception as e:
        sys.stderr.write("Error with: " + f)
        write_exception(e)
               
    """ % run_info_dict

    run_script_file = join(temp_dir, "run_script.py")
    open(run_script_file, "w").write(run_script)

    log_file_stdout = join(trial_temp_dir, "stdout.log")
    log_file_stderr = join(trial_temp_dir, "stderr.log")

    env = os.environ.copy()
    env['__GL_SYS_PATH__'] = (os.path.pathsep).join(sys.path)

    proc = subprocess.Popen(
            [sys.executable, os.path.abspath(run_script_file)],
            stdout = open(log_file_stdout, "w"),
            stderr = open(log_file_stderr, "w"),
            env = env)
    
    proc.wait()

    # Now zip up the output data into a package we can access.
    timestamp = datetime.datetime.fromtimestamp(time.time()).strftime('%Y-%m-%d-%H-%M-%S')
    zipfile_name = join(temp_dir, "testing_logs-%d-%s.zip" % (os.getpid(), timestamp))

    print("Creating archive of log files in %s." % zipfile_name)

    save_files = []

    for root, dirs, files in os.walk(temp_dir):
        save_files += [join(root, name) for name in files]
    
    with zipfile.ZipFile(zipfile_name, 'w') as logzip:
        error_logs = []
        for f in save_files:
            try:
                logzip.write(f)
            except Exception as e:
                error_logs.append("%s: error = %s" % (f, repr(e)))

        if error_logs:
            error_log_file = join(temp_dir, "archive_errors.log")
            open(error_log_file, "w").write("\n\n".join(error_logs))
            logzip.write(error_log_file)

    print("################################################################################")
    print("#   ")
    print("#   Results of lambda test logged as %s." % zipfile_name)
    print("#   ")
    print("################################################################################")

    print("Cleaning up.")

    for f in save_files:
        try:
            os.remove(f)
        except Exception:
            pass
 

def dump_directory_structure(out = sys.stdout):
    """
    Dumps a detailed report of the turicreate/sframe directory structure
    and files, along with the output of os.lstat for each.  This is useful
    for debugging purposes.
    """

    "Dumping Installation Directory Structure for Debugging: "

    import sys, os
    from os.path import split, abspath, join
    from itertools import chain
    main_dir = split(abspath(sys.modules[__name__].__file__))[0]

    visited_files = []

    def on_error(err):
        visited_files.append( ("  ERROR", str(err)) )

    for path, dirs, files in os.walk(main_dir, onerror = on_error):
        for fn in chain(files, dirs):
            name = join(path, fn)
            try:
                visited_files.append( (name, repr(os.lstat(name))) )
            except:
                visited_files.append( (name, "ERROR calling os.lstat.") )

    def strip_name(n):
        if n[:len(main_dir)] == main_dir:
            return "<root>/" + n[len(main_dir):]
        else:
            return n

    out.write("\n".join( ("  %s: %s" % (strip_name(name), stats))
                     for name, stats in sorted(visited_files)))

    out.flush()

__hadoop_class_warned = False

def get_hadoop_class_path():
    # Try get the classpath directly from executing hadoop
    env = os.environ.copy()
    hadoop_exe_name = 'hadoop'
    if sys.platform == 'win32':
        hadoop_exe_name += '.cmd'
    output = None
    try:
        try:
            output = _subprocess.check_output([hadoop_exe_name, 'classpath']).decode()
        except:
            output = _subprocess.check_output(['/'.join([env['HADOOP_HOME'],'bin',hadoop_exe_name]), 'classpath']).decode()

        output = (os.path.pathsep).join(os.path.realpath(path) for path in output.split(os.path.pathsep))
        return _get_expanded_classpath(output)

    except Exception as e:
        global __hadoop_class_warned
        if not __hadoop_class_warned:
            __hadoop_class_warned = True
            logging.getLogger(__name__).debug("Exception trying to retrieve Hadoop classpath: %s" % e)

    logging.getLogger(__name__).debug("Hadoop not found. HDFS url is not supported. Please make hadoop available from PATH or set the environment variable HADOOP_HOME.")
    return ""


def _get_expanded_classpath(classpath):
    """
    Take a classpath of the form:
      /etc/hadoop/conf:/usr/lib/hadoop/lib/*:/usr/lib/hadoop/.//*: ...

    and return it expanded to all the JARs (and nothing else):
      /etc/hadoop/conf:/usr/lib/hadoop/lib/netty-3.6.2.Final.jar:/usr/lib/hadoop/lib/jaxb-api-2.2.2.jar: ...

    mentioned in the path
    """
    if classpath is None or classpath == '':
        return ''

    #  so this set comprehension takes paths that end with * to be globbed to find the jars, and then
    #  recombined back into a colon separated list of jar paths, removing dupes and using full file paths
    jars = (os.path.pathsep).join((os.path.pathsep).join([os.path.abspath(jarpath) for jarpath in _glob.glob(path)])
                    for path in classpath.split(os.path.pathsep))
    logging.getLogger(__name__).debug('classpath being used: %s' % jars)
    return jars

def get_library_name():
    """
    Returns either sframe or turicreate depending on which library
    this file is bundled with.
    """
    from os.path import split, abspath

    __lib_name = split(split(abspath(sys.modules[__name__].__file__))[0])[1]

    assert __lib_name in ["sframe", "turicreate"]

    return __lib_name


def get_config_file():
    """
    Returns the file name of the config file from which the environment
    variables are written.
    """
    import os
    from os.path import abspath, expanduser, join, exists

    __lib_name = get_library_name()

    assert __lib_name in ["sframe", "turicreate"]

    __default_config_path = join(expanduser("~"), ".%s" % __lib_name, "config")

    if "TURI_CONFIG_FILE" in os.environ:
        __default_config_path = abspath(expanduser(os.environ["TURI_CONFIG_FILE"]))

        if not exists(__default_config_path):
            print(("WARNING: Config file specified in environment variable "
                   "'TURI_CONFIG_FILE' as "
                   "'%s', but this path does not exist.") % __default_config_path)

    return __default_config_path


def setup_environment_from_config_file():
    """
    Imports the environmental configuration settings from the
    config file, if present, and sets the environment
    variables to test it.
    """

    from os.path import exists

    config_file = get_config_file()

    if not exists(config_file):
        return

    try:
        config = _ConfigParser.SafeConfigParser()
        config.read(config_file)

        __section = "Environment"

        if config.has_section(__section):
            items = config.items(__section)

            for k, v in items:
                try:
                    os.environ[k.upper()] = v
                except Exception as e:
                    print(("WARNING: Error setting environment variable "
                           "'%s = %s' from config file '%s': %s.")
                          % (k, str(v), config_file, str(e)) )
    except Exception as e:
        print("WARNING: Error reading config file '%s': %s." % (config_file, str(e)))


def write_config_file_value(key, value):
    """
    Writes an environment variable configuration to the current
    config file.  This will be read in on the next restart.
    The config file is created if not present.

    Note: The variables will not take effect until after restart.
    """

    filename = get_config_file()

    config = _ConfigParser.SafeConfigParser()
    config.read(filename)

    __section = "Environment"

    if not(config.has_section(__section)):
        config.add_section(__section)

    config.set(__section, key, value)

    with open(filename, 'w') as config_file:
        config.write(config_file)
