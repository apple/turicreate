# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
'''
This package provides commonly used methods for dealing with file operation,
including working with network file system like S3, http, etc.
'''
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
import os
import sys
import time
import logging
import shutil
import tempfile
import subprocess
import re
from subprocess import PIPE
from .type_checks import _is_string
from . import _get_s3_endpoint, _get_aws_credentials

__logger__ = logging.getLogger(__name__)

__RETRY_TIMES = 5
__SLEEP_SECONDS_BETWEEN_RETRIES = 2

def get_protocol(path):
    '''Given a path, returns the protocol the path uses

    For example,
      's3://a/b/c/'  returns 's3'
      'http://a/b/c' returns 'http'
      'tmp/a/bc/'    returns ''

    '''
    pos = path.find('://')
    if pos < 0:
        return ''
    return path[0:pos].lower()

def is_path(string):
    if not isinstance(string, str):
        return False
    return is_local_path(string) or is_s3_path(string)

def expand_full_path(path):
    '''Expand a relative path to a full path

    For example,
      '~/tmp' may be expanded to '/Users/username/tmp'
      'abc/def' may be expanded to '/pwd/abc/def'
    '''
    return os.path.abspath(os.path.expanduser(path))

def mkdir(path):
    if is_local_path(path):
        os.makedirs(path)
    else:
        raise ValueError('Unsupported protocol %s' % path)

def exists(path, aws_credentials = {}):
    if is_local_path(path):
        return os.path.exists(path)
    else:
        raise ValueError('Unsupported protocol %s' % path)

def touch(path):
    if is_local_path(path):
        with open(path, 'a'):
            os.utime(path, None)
    else:
        raise ValueError('Unsupported protocol %s' % path)

def read(path):
    if is_local_path(path):
        return open(path).read()
    else:
        raise ValueError('Unsupported protocol %s' % path)

def is_local_path(path):
    '''Returns True if the path indicates a local path, otherwise False'''
    protocol = get_protocol(path)
    return protocol != 'hdfs' and protocol != 's3' and \
          protocol != 'http' and protocol != 'https'


def copy_from_local(localpath, remotepath, is_dir = False, silent = True):
    if is_local_path(remotepath):
        if is_dir:
            shutil.copytree(localpath, remotepath)
        else:
            shutil.copy(localpath, remotepath)
    else:
        raise ValueError('Unsupported protocol %s' % remotepath)
