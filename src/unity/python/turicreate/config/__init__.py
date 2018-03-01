# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import sys as _sys
import tempfile as _tempfile
import os as _os
import time as _time
import logging as _logging
import re as _re

# Return the root package name
_root_package_name = __import__(__name__.split('.')[0]).__name__
_client_log_file = _os.path.join(_tempfile.gettempdir(),
                                _root_package_name +
                                '_client_%d_%d.log' % (_time.time(), _os.getpid()))

def _get_log_location():
    from ..connect import main as _glconnect
    server = _glconnect.get_server()
    if hasattr(server, 'unity_log'):
        return server.unity_log
    else:
        return None

def _i_am_a_lambda_worker():
    if _re.match(".*lambda_worker.*", _sys.argv[0]) is not None:
        return True
    return False

def init_logger():
    """
    Initialize the logging configuration for the turicreate/sframe package.

    This does not affect the logging config of root or other modules outside of
    turicreate/sframe.
    """
    # Package level logger
    _logging.config.dictConfig({
        'version': 1,
        'disable_existing_loggers': False,
        'formatters': {
            'standard': {
                'format': '%(asctime)s [%(levelname)s] %(name)s, %(lineno)s: %(message)s'
            },
            'brief': {
                'format': '[%(levelname)s] %(name)s: %(message)s'
            }
        },
        'handlers': {
            'default': {
                'class': 'logging.StreamHandler',
                'formatter': 'brief'
            },
            'file': {
                'class': 'logging.FileHandler',
                'formatter': 'standard',
                'filename': _client_log_file,
                'encoding': 'UTF-8',
                'delay': 'False',
            }
        },
        'loggers': {
            _root_package_name: {
                'handlers': ['default', 'file'],
                'propagate': 'True'
            }
        }
    })

    # Set module specific log levels
    _logging.getLogger('requests').setLevel(_logging.CRITICAL)
    if _i_am_a_lambda_worker():
        _logging.getLogger(_root_package_name).setLevel(_logging.WARNING)
    else:
        _logging.getLogger(_root_package_name).setLevel(_logging.INFO)


# Let's call init_logger on import
init_logger()

def get_client_log_location():
    """
    Get the location of client logs
    """
    return _client_log_file

def get_server_log_location():
    """
    Get the locations of server logs
    """
    return _get_log_location()

def set_num_gpus(num_gpus):
    """
    Set the number of GPUs to use whenever applicable.

    This can also be set by adding the following line to your
    ``~/.turicreate/config`` file, e.g.:

    ```
    num_gpus: 1
    ```

    Calling this function will override whatever you define in your config
    file.

    Parameters
    ----------
    num_gpus : int
        Number of GPUs to use. To always use CPU, set to 0. To use all
        available GPUs, set to -1.

    See also
    --------
    get_num_gpus

    Examples
    --------
    .. sourcecode:: python

      >> turicreate.config.set_num_gpus(1)
      >> turicreate.image_classifier.create(data, target='label')
    """
    set_runtime_config('TURI_NUM_GPUS', num_gpus)


def get_num_gpus():
    """
    Get the current option for how many GPUs to use when applicable.

    See also
    --------
    set_num_gpus
    """
    return get_runtime_config()['TURI_NUM_GPUS']

def get_environment_config():
    """
    Returns all the Turi Create configuration variables that can only
    be set via environment variables.

    - *TURI_FILEIO_WRITER_BUFFER_SIZE*: The file write buffer size.
    - *TURI_FILEIO_READER_BUFFER_SIZE*: The file read buffer size.
    - *OMP_NUM_THREADS*: The maximum number of threads to use for parallel processing.

    Returns
    -------
    Returns a dictionary of {key:value,..}
    """
    from ..connect import main as _glconnect
    unity = _glconnect.get_unity()
    return unity.list_globals(False)

def set_log_level(level):
    """
    Sets the log level.
    Lower log levels log more.
    if level is 8, nothing is logged. If level is 0, everything is logged.
    """
    from ..connect import main as _glconnect
    unity = _glconnect.get_unity()
    return unity.set_log_level(level)


def get_runtime_config():
    """
    Returns all the Turi Create configuration variables that can be set
    at runtime. See :py:func:`turicreate.set_runtime_config()` to set these
    values and for documentation on the effect of each variable.

    Returns
    -------
    Returns a dictionary of {key:value,..}

    See Also
    --------
    set_runtime_config
    """
    from ..connect import main as _glconnect
    unity = _glconnect.get_unity()
    return unity.list_globals(True)

def set_runtime_config(name, value):
    """
    Configures system behavior at runtime. These configuration values are also
    read from environment variables at program startup if available. See
    :py:func:`turicreate.get_runtime_config()` to get the current values for
    each variable.

    Note that defaults may change across versions and the names
    of performance tuning constants may also change as improved algorithms
    are developed and implemented.

    Parameters
    ----------
    name : string
        A string referring to runtime configuration variable.

    value
        The value to set the variable to.

    Raises
    ------
    RuntimeError
        If the key does not exist, or if the value cannot be changed to the
        requested value.

    Notes
    -----
    The following section documents all the Turi Create environment variables
    that can be configured.

    **Basic Configuration Variables**

    - *TURI_NUM_GPUS*: Number of GPUs to use when applicable. Set to 0 to force
      CPU use in all situations.

    - *TURI_CACHE_FILE_LOCATIONS*: The directory in which intermediate
      SFrames/SArray are stored.  For instance "/var/tmp".  Multiple
      directories can be specified separated by a colon (ex: "/var/tmp:/tmp")
      in which case intermediate SFrames will be striped across both
      directories (useful for specifying multiple disks).  Defaults to /var/tmp
      if the directory exists, /tmp otherwise.

    - *TURI_FILEIO_MAXIMUM_CACHE_CAPACITY*: The maximum amount of memory which
      will be occupied by *all* intermediate SFrames/SArrays. Once this limit
      is exceeded, SFrames/SArrays will be flushed out to temporary storage (as
      specified by `TURI_CACHE_FILE_LOCATIONS`). On large systems increasing
      this as well as `TURI_FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE` can improve
      performance significantly. Defaults to 2147483648 bytes (2GB).

    - *TURI_FILEIO_MAXIMUM_CACHE_CAPACITY_PER_FILE*: The maximum amount of
      memory which will be occupied by any individual intermediate
      SFrame/SArray.  Once this limit is exceeded, the SFrame/SArray will be
      flushed out to temporary storage (as specified by
      `TURI_CACHE_FILE_LOCATIONS`). On large systems, increasing this as well
      as `TURI_FILEIO_MAXIMUM_CACHE_CAPACITY` can improve performance
      significantly for large SFrames. Defaults to 134217728 bytes (128MB).

    **S3 Configuration**

    - *TURI_S3_ENDPOINT*: The S3 Endpoint to connect to. If not specified AWS
      S3 is assumed.

    **SSL Configuration**

    - *TURI_FILEIO_ALTERNATIVE_SSL_CERT_FILE*: The location of an SSL
      certificate file used to validate HTTPS / S3 connections. Defaults to the
      the Python certifi package certificates.

    - *TURI_FILEIO_ALTERNATIVE_SSL_CERT_DIR*: The location of an SSL
      certificate directory used to validate HTTPS / S3 connections. Defaults
      to the operating system certificates.

    - *TURI_FILEIO_INSECURE_SSL_CERTIFICATE_CHECKS*: If set to a non-zero
      value, disables all SSL certificate validation.  Defaults to False.

    **Sort Performance Configuration**

    - *TURI_SFRAME_SORT_PIVOT_ESTIMATION_SAMPLE_SIZE*: The number of random
      rows to sample from the SFrame to estimate the sort pivots used to
      partition the sort. Defaults to 2000000.

    - *TURI_SFRAME_SORT_BUFFER_SIZE*: The maximum estimated memory consumption
      sort is allowed to use. Increasing this will increase the size of each
      sort partition, and will increase performance with increased memory
      consumption.  Defaults to 2GB.

    **Join Performance Configuration**

    - *TURI_SFRAME_JOIN_BUFFER_NUM_CELLS*: The maximum number of cells to
      buffer in memory. Increasing this will increase the size of each join
      partition and will increase performance with increased memory
      consumption.  If you have very large cells (very long strings for
      instance), decreasing this value will help decrease memory consumption.
      Defaults to 52428800.

    **Groupby Aggregate Performance Configuration**

    - *TURI_SFRAME_GROUPBY_BUFFER_NUM_ROWS*: The number of groupby keys cached
      in memory. Increasing this will increase performance with increased
      memory consumption. Defaults to 1048576.

    **Advanced Configuration Variables**

    - *TURI_SFRAME_FILE_HANDLE_POOL_SIZE*: The maximum number of file handles
      to use when reading SFrames/SArrays.  Once this limit is exceeded, file
      handles will be recycled, reducing performance. This limit should be
      rarely approached by most SFrame/SArray operations. Large SGraphs however
      may create a large a number of SFrames in which case increasing this
      limit may improve performance (You may also need to increase the system
      file handle limit with "ulimit -n").  Defaults to 128.
    """
    from ..connect import main as _glconnect
    unity = _glconnect.get_unity()
    ret = unity.set_global(name, value)
    if ret != "":
        raise RuntimeError(ret)
