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
from os.path import split, abspath, join
from glob import glob
from itertools import chain

def get_main_dir():
    script_path = abspath(sys.modules[__name__].__file__)
    main_dir = split(split(script_path)[0])[0]

    return main_dir

def get_installation_flavor():

    module = split(get_main_dir())[1]

    if module == "sframe":
        return "sframe"
    elif module == "turicreate":
        return "turicreate"
    else:
        raise ImportError("Installation module does not appear to be sframe or turicreate; main dir = %s"
                          % get_main_dir())


def load_isolated_gl_module(subdir, name):

    if subdir:
        path = join(get_main_dir(), subdir)
    else:
        path = get_main_dir()

    fp, pathname, description = imp.find_module(name, [path])

    try:
        return imp.load_module(name, fp, pathname, description)
    finally:
        # Since we may exit via an exception, close fp explicitly.
        if fp:
            fp.close()


def setup_environment(info_log_function = None, error_log_function = None):

    def _write_log(s, error = False):
        if error:
            if error_log_function is None:
                print(s)
            else:
                try:
                    error_log_function(s)
                except Exception as e:
                    print("Error setting exception: repr(e)")
                    print("Error: %s" % str(s))
        else:
            if info_log_function is not None:
                try:
                    info_log_function(s)
                except Exception as e:
                    print("Error logging info: %s." % repr(e))
                    print("Message: %s" % str(s))

    ########################################
    # Set up the system path.

    system_path = os.environ.get("__GL_SYS_PATH__", "")

    del sys.path[:]
    sys.path.extend(p.strip() for p in system_path.split(os.pathsep) if p.strip())

    for i, p in enumerate(sys.path):
        _write_log("  sys.path[%d] = %s. " % (i, sys.path[i]))

    ########################################
    # Now, import thnigs

    main_dir = get_main_dir()

    _write_log("Main program directory: %s." % main_dir)

    ########################################
    # Finally, set the dll load path if we are on windows
    if sys.platform == 'win32':

        import ctypes
        import ctypes.wintypes as wintypes

        # Back up to the directory, then to the base directory as this is
        # in ./_scripts.
        lib_path = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))

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
        try:
            kernel32 = ctypes.WinDLL('kernel32', use_last_error=True)
            kernel32.SetDllDirectoryW.errcheck = errcheck_bool
            kernel32.SetDllDirectoryW.argtypes = (wintypes.LPCWSTR,)
            kernel32.SetDllDirectoryW(lib_path)
        except Exception as e:
            _write_log("Error setting DLL load orders: %s (things may still work).\n"
                       % str(e), error = True)

if __name__ == "__main__":

    ############################################################
    # Set up the logging
    
    if len(sys.argv) == 1:
        dry_run = True
    else:
        dry_run = False

    if dry_run or os.environ.get("TURI_LAMBDA_WORKER_DEBUG_MODE") == "1":
        _write_out = sys.stderr
    else:
        _write_out = None

    _write_out_file_name = os.environ.get("TURI_LAMBDA_WORKER_LOG_FILE", "")
    _write_out_file = None
    
    def _write_log(s, error = False):
        s = s + "\n"

        if error:
            try:
                sys.stderr.write(s)
                sys.stderr.flush()
            except Exception:
                # Can't do anything in this case, as it can be because
                # of a bad file descriptor passed on from a windows
                # subprocess
                pass
            
        elif _write_out is not None:
            try:
                _write_out.write(s)
                _write_out.flush()
            except Exception:
                pass

        if _write_out_file is not None:
            try:
                _write_out_file.write(s)
                _write_out_file.flush()
            except Exception:
                pass

    if _write_out_file_name != "":

        # Set this to an absolute location to make things worthwhile
        _write_out_file_name = abspath(_write_out_file_name)
        os.environ["TURI_LAMBDA_WORKER_LOG_FILE"] = _write_out_file_name

        _write_out_file_name = _write_out_file_name + "-init"
        _write_log("Logging initialization routines to %s." % _write_out_file_name)
        try:
            _write_out_file = open(_write_out_file_name, "w")
        except Exception as e:
            _write_log("Error opening '%s' for write: %s" % (_write_out_file_name, repr(e)))
            _write_out_file = None

    for s in sys.argv:
        _write_log("Lambda worker args: \n  %s" % ("\n  ".join(sys.argv)))
            
    if dry_run:
        print("PyLambda script called with no IPC information; entering diagnostic mode.")

    setup_environment(info_log_function = _write_log,
                      error_log_function = lambda s: _write_log(s, error=True))
        
    ############################################################
    # Load in the cython lambda workers.  On import, this will resolve
    # the proper symbols.
    
    if get_installation_flavor() == "sframe":
        from sframe.cython.cy_pylambda_workers import run_pylambda_worker
    else:
        from turicreate.cython.cy_pylambda_workers import run_pylambda_worker
    
    main_dir = get_main_dir()

    default_loglevel = 5  # 5: LOG_WARNING, 4: LOG_PROGRESS  3: LOG_EMPH  2: LOG_INFO  1: LOG_DEBUG
    dryrun_loglevel = 1  # 5: LOG_WARNING, 4: LOG_PROGRESS  3: LOG_EMPH  2: LOG_INFO  1: LOG_DEBUG
    
    if not dry_run:
        # This call only returns after the parent process is done.
        result = run_pylambda_worker(main_dir, sys.argv[1], default_loglevel)
    else:
        # This version will print out a bunch of diagnostic information and then exit.
        result = run_pylambda_worker(main_dir, "debug", dryrun_loglevel)

    _write_log("Lambda process exited with code %d." % result)
    sys.exit(0)
