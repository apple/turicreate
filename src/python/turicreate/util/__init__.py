# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _

import os as _os
import urllib as _urllib
import re as _re
import itertools as _itertools
import uuid as _uuid
import datetime as _datetime
import sys as _sys

from ._sframe_generation import generate_random_sframe
from ._sframe_generation import generate_random_regression_sframe
from ._sframe_generation import generate_random_classification_sframe
from ._type_checks import _raise_error_if_not_of_type
from ._type_checks import _is_non_string_iterable
from ._progress_table_printer import ProgressTablePrinter as _ProgressTablePrinter

try:
    import configparser as _ConfigParser
except ImportError:
    import ConfigParser as _ConfigParser


def _convert_slashes(path):
    """
    Converts all windows-style slashes to unix-style slashes
    """
    return path.replace("\\", "/")


def _get_s3_endpoint():
    """
    Returns the current S3 Endpoint"
    """
    import turicreate

    return turicreate.config.get_runtime_config()["TURI_S3_ENDPOINT"]


def _get_aws_credentials():
    """
    Returns the values stored in the AWS credential environment variables.
    Returns the value stored in the AWS_ACCESS_KEY_ID environment variable and
    the value stored in the AWS_SECRET_ACCESS_KEY environment variable.

    Returns
    -------
    out : tuple [string]
        The first string of the tuple is the value of the AWS_ACCESS_KEY_ID
        environment variable. The second string of the tuple is the value of the
        AWS_SECRET_ACCESS_KEY environment variable.

    Examples
    --------
    >>> turicreate.aws.get_credentials()
    ('RBZH792CTQPP7T435BGQ', '7x2hMqplWsLpU/qQCN6xAPKcmWo46TlPJXYTvKcv')
    """

    if not "AWS_ACCESS_KEY_ID" in _os.environ:
        raise KeyError(
            "No access key found. Please set the environment variable AWS_ACCESS_KEY_ID."
        )
    if not "AWS_SECRET_ACCESS_KEY" in _os.environ:
        raise KeyError(
            "No secret key found. Please set the environment variable AWS_SECRET_ACCESS_KEY."
        )
    return (_os.environ["AWS_ACCESS_KEY_ID"], _os.environ["AWS_SECRET_ACCESS_KEY"])


def _try_inject_s3_credentials(url):
    """
    Inject aws credentials into s3 url as s3://[aws_id]:[aws_key]:[bucket/][objectkey]

    If s3 url already contains secret key/id pairs, just return as is.
    """
    assert url.startswith("s3://")
    path = url[5:]
    # Check if the path already contains credentials
    tokens = path.split(":")
    # If there are two ':', its possible that we have already injected credentials
    if len(tokens) == 3:
        # Edge case: there are exactly two ':'s in the object key which is a false alarm.
        # We prevent this by checking that '/' is not in the assumed key and id.
        if ("/" not in tokens[0]) and ("/" not in tokens[1]):
            return url

    # S3 url does not contain secret key/id pair, query the environment variables
    (k, v) = _get_aws_credentials()
    return "s3://" + k + ":" + v + ":" + path


def _make_internal_url(url):
    """
    Process user input url string with proper normalization
    For all urls:
      Expands ~ to $HOME
    For S3 urls:
      Returns the s3 URL with credentials filled in using turicreate.aws.get_aws_credential().
      For example: "s3://mybucket/foo" -> "s3://$AWS_ACCESS_KEY_ID:$AWS_SECRET_ACCESS_KEY:mybucket/foo".
    For hdfs urls:
      Error if hadoop classpath is not set
    For local file urls:
      convert slashes for windows sanity

    Parameters
    ----------
    string
        A URL (as described above).

    Raises
    ------
    ValueError
        If a bad url is provided.
    """
    if not url:
        raise ValueError("Invalid url: %s" % url)

    from .. import _sys_util
    from . import _file_util

    # Convert Windows paths to Unix-style slashes
    url = _convert_slashes(url)

    # Try to split the url into (protocol, path).
    protocol = _file_util.get_protocol(url)
    is_local = False
    if protocol in ["http", "https"]:
        pass
    elif protocol == "hdfs":
        if not _sys_util.get_hadoop_class_path():
            raise ValueError(
                "HDFS URL is not supported because Hadoop not found. Please make hadoop available from PATH or set the environment variable HADOOP_HOME and try again."
            )
    elif protocol == "s3":
        return _try_inject_s3_credentials(url)
    elif protocol == "":
        is_local = True
    elif protocol == "local" or protocol == "remote":
        # local and remote are legacy protocol for separate server process
        is_local = True
        # This code assumes local and remote are same machine
        url = _re.sub(protocol + "://", "", url, count=1)
    else:
        raise ValueError(
            "Invalid url protocol %s. Supported url protocols are: local, s3://, https:// and hdfs://"
            % protocol
        )

    if is_local:
        url = _os.path.abspath(_os.path.expanduser(url))
    return url


def is_directory_archive(path):
    """
    Utility function that returns True if the path provided is a directory that has an SFrame or SGraph in it.

    SFrames are written to disk as a directory archive, this function identifies if a given directory is an archive
    for an SFrame.

    Parameters
    ----------
    path : string
        Directory to evaluate.

    Returns
    -------
    True if path provided is an archive location, False otherwise
    """
    if path is None:
        return False

    if not _os.path.isdir(path):
        return False

    ini_path = "/".join([_convert_slashes(path), "dir_archive.ini"])

    if not _os.path.exists(ini_path):
        return False

    if _os.path.isfile(ini_path):
        return True
    return False


def get_archive_type(path):
    """
    Returns the contents type for the provided archive path.

    Parameters
    ----------
    path : string
        Directory to evaluate.

    Returns
    -------
    Returns a string of: sframe, sgraph, raises TypeError for anything else
    """
    if not is_directory_archive(path):
        raise TypeError("Unable to determine the type of archive at path: %s" % path)

    try:
        ini_path = "/".join([_convert_slashes(path), "dir_archive.ini"])
        parser = _ConfigParser.SafeConfigParser()
        parser.read(ini_path)

        contents = parser.get("metadata", "contents")
        return contents
    except Exception as e:
        raise TypeError("Unable to determine type of archive for path: %s" % path, e)


def crossproduct(d):
    """
    Create an SFrame containing the crossproduct of all provided options.

    Parameters
    ----------
    d : dict
        Each key is the name of an option, and each value is a list
        of the possible values for that option.

    Returns
    -------
    out : SFrame
        There will be a column for each key in the provided dictionary,
        and a row for each unique combination of all values.

    Example
    -------
    settings = {'argument_1':[0, 1],
                'argument_2':['a', 'b', 'c']}
    print crossproduct(settings)
    +------------+------------+
    | argument_2 | argument_1 |
    +------------+------------+
    |     a      |     0      |
    |     a      |     1      |
    |     b      |     0      |
    |     b      |     1      |
    |     c      |     0      |
    |     c      |     1      |
    +------------+------------+
    [6 rows x 2 columns]
    """

    from .. import SArray

    d = [list(zip(list(d.keys()), x)) for x in _itertools.product(*list(d.values()))]
    sa = [{k: v for (k, v) in x} for x in d]
    return SArray(sa).unpack(column_name_prefix="")


def get_turicreate_object_type(url):
    """
    Given url where a Turi Create object is persisted, return the Turi
    Create object type: 'model', 'graph', 'sframe', or 'sarray'
    """
    from .._connect import main as _glconnect

    ret = _glconnect.get_unity().get_turicreate_object_type(_make_internal_url(url))

    # to be consistent, we use sgraph instead of graph here
    if ret == "graph":
        ret = "sgraph"
    return ret


def _assert_sframe_equal(
    sf1,
    sf2,
    check_column_names=True,
    check_column_order=True,
    check_row_order=True,
    float_column_delta=None,
):
    """
    Assert the two SFrames are equal.

    The default behavior of this function uses the strictest possible
    definition of equality, where all columns must be in the same order, with
    the same names and have the same data in the same order.  Each of these
    stipulations can be relaxed individually and in concert with another, with
    the exception of `check_column_order` and `check_column_names`, we must use
    one of these to determine which columns to compare with one another.

    Parameters
    ----------
    sf1 : SFrame

    sf2 : SFrame

    check_column_names : bool
        If true, assert if the data values in two columns are the same, but
        they have different names.  If False, column order is used to determine
        which columns to compare.

    check_column_order : bool
        If true, assert if the data values in two columns are the same, but are
        not in the same column position (one is the i-th column and the other
        is the j-th column, i != j).  If False, column names are used to
        determine which columns to compare.

    check_row_order : bool
        If true, assert if all rows in the first SFrame exist in the second
        SFrame, but they are not in the same order.

    float_column_delta : float
        The acceptable delta that two float values can be and still be
        considered "equal". When this is None, only exact equality is accepted.
        This is the default behavior since columns of all Nones are often of
        float type. Applies to all float columns.
    """
    from .. import SFrame as _SFrame

    if (type(sf1) is not _SFrame) or (type(sf2) is not _SFrame):
        raise TypeError("Cannot function on types other than SFrames.")

    if not check_column_order and not check_column_names:
        raise ValueError("Cannot ignore both column order and column names.")

    sf1.materialize()
    sf2.materialize()

    if sf1.num_columns() != sf2.num_columns():
        raise AssertionError(
            "Number of columns mismatched: "
            + str(sf1.num_columns())
            + " != "
            + str(sf2.num_columns())
        )

    s1_names = sf1.column_names()
    s2_names = sf2.column_names()

    sorted_s1_names = sorted(s1_names)
    sorted_s2_names = sorted(s2_names)

    if check_column_names:
        if (check_column_order and (s1_names != s2_names)) or (
            sorted_s1_names != sorted_s2_names
        ):
            raise AssertionError(
                "SFrame does not have same column names: "
                + str(sf1.column_names())
                + " != "
                + str(sf2.column_names())
            )

    if sf1.num_rows() != sf2.num_rows():
        raise AssertionError(
            "Number of rows mismatched: "
            + str(sf1.num_rows())
            + " != "
            + str(sf2.num_rows())
        )

    if not check_row_order and (sf1.num_rows() > 1):
        sf1 = sf1.sort(s1_names)
        sf2 = sf2.sort(s2_names)

    names_to_check = None
    if check_column_names:
        names_to_check = list(zip(sorted_s1_names, sorted_s2_names))
    else:
        names_to_check = list(zip(s1_names, s2_names))
    for i in names_to_check:
        col1 = sf1[i[0]]
        col2 = sf2[i[1]]
        if col1.dtype != col2.dtype:
            raise AssertionError("Columns " + str(i) + " types mismatched.")

        compare_ary = None
        if col1.dtype == float and float_column_delta is not None:
            dt = float_column_delta
            compare_ary = (col1 > col2 - dt) & (col1 < col2 + dt)
        else:
            compare_ary = sf1[i[0]] == sf2[i[1]]
        if not compare_ary.all():
            count = 0
            for j in compare_ary:
                if not j:
                    first_row = count
                    break
                count += 1
            raise AssertionError(
                "Columns "
                + str(i)
                + " are not equal! First differing element is at row "
                + str(first_row)
                + ": "
                + str((col1[first_row], col2[first_row]))
            )


def _get_temp_file_location():
    """
    Returns user specified temporary file location.
    The temporary location is specified through:

    >>> turicreate.config.set_runtime_config('TURI_CACHE_FILE_LOCATIONS', ...)

    """
    from .._connect import main as _glconnect

    unity = _glconnect.get_unity()
    cache_dir = _convert_slashes(unity.get_current_cache_file_location())
    if not _os.path.exists(cache_dir):
        _os.makedirs(cache_dir)
    return cache_dir


def _make_temp_directory(prefix):
    """
    Generate a temporary directory that would not live beyond the lifetime of
    unity_server.

    Caller is expected to clean up the temp file as soon as the directory is no
    longer needed. But the directory will be cleaned as unity_server restarts
    """
    temp_dir = _make_temp_filename(prefix=str(prefix))
    _os.makedirs(temp_dir)
    return temp_dir


def _make_temp_filename(prefix):
    """
    Generate a temporary file that would not live beyond the lifetime of
    unity_server.

    Caller is expected to clean up the temp file as soon as the file is no
    longer needed. But temp files created using this method will be cleaned up
    when unity_server restarts
    """
    temp_location = _get_temp_file_location()
    temp_file_name = "/".join([temp_location, str(prefix) + str(_uuid.uuid4())])
    return temp_file_name


def _pickle_to_temp_location_or_memory(obj):
    """
        If obj can be serialized directly into memory (via cloudpickle) this
        will return the serialized bytes.
        Otherwise, gl_pickle is attempted and it will then
        generates a temporary directory serializes an object into it, returning
        the directory name. This directory will not have lifespan greater than
        that of unity_server.
        """
    from . import _cloudpickle as cloudpickle

    try:
        # try cloudpickle first and see if that works
        lambda_str = cloudpickle.dumps(obj)
        return lambda_str
    except:
        pass

    # nope. that does not work! lets try again with gl pickle
    filename = _make_temp_filename("pickle")
    from .. import _gl_pickle

    pickler = _gl_pickle.GLPickler(filename)
    pickler.dump(obj)
    pickler.close()
    return filename


def _get_module_from_object(obj):
    mod_str = obj.__class__.__module__.split(".")[0]
    return _sys.modules[mod_str]


def _infer_dbapi2_types(cursor, mod_info):
    desc = cursor.description
    result_set_types = [i[1] for i in desc]
    dbapi2_to_python = [  # a type code can match more than one, so ordered by
        # preference (loop short-circuits when it finds a match
        (mod_info["DATETIME"], _datetime.datetime),
        (mod_info["ROWID"], int),
        (mod_info["NUMBER"], float),
    ]
    ret_types = []

    # Ugly nested loop because the standard only guarantees that a type code
    # will compare equal to the module-defined types
    for i in result_set_types:
        type_found = False
        for j in dbapi2_to_python:
            if i is None or j[0] is None:
                break
            elif i == j[0]:
                ret_types.append(j[1])
                type_found = True
                break
        if not type_found:
            ret_types.append(str)

    return ret_types


def _pytype_to_printf(in_type):
    if in_type == int:
        return "d"
    elif in_type == float:
        return "f"
    else:
        return "s"


# Automatic GPU detection
def _get_cuda_gpus():
    """
    Returns a list of dictionaries, with the following keys:
    - index (integer, device index of the GPU)
    - name (str, GPU name)
    - memory_free (float, free memory in MiB)
    - memory_total (float, total memory in MiB)
    """
    import subprocess

    try:
        output = subprocess.check_output(
            [
                "nvidia-smi",
                "--query-gpu=index,gpu_name,memory.free,memory.total",
                "--format=csv,noheader,nounits",
            ],
            universal_newlines=True,
        )
    except OSError:
        return []
    except subprocess.CalledProcessError:
        return []

    gpus = []
    for gpu_line in output.split("\n"):
        if gpu_line:
            index, gpu_name, memory_free, memory_total = gpu_line.split(", ")
            index = int(index)
            memory_free = float(memory_free)
            memory_total = float(memory_total)
            gpus.append(
                {
                    "index": index,
                    "name": gpu_name,
                    "memory_free": memory_free,
                    "memory_total": memory_total,
                }
            )
    return gpus


_CUDA_GPUS = _get_cuda_gpus()


def _num_available_cuda_gpus():
    return len(_CUDA_GPUS)


def _num_available_gpus():
    num_cuda = _num_available_cuda_gpus()
    if num_cuda > 0:
        return num_cuda

    from turicreate.toolkits._mps_utils import has_fast_mps_support

    if has_fast_mps_support():
        return 1

    return 0
