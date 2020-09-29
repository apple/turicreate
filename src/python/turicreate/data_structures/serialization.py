
# -*- coding: utf-8 -*-
# Copyright © 2020 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

# SFrames require a disk-backed file or directory to work with.  This directory 
# has to be present to allow for serialization or deserialization. 
__serialization_directory = None 
from sys import version_info as _version_info


def enable_sframe_serialization(serialization_directory):
    """
    Enables pickling of SFrames through the use of a user-set directory to 
    store the objects.  This directory must be set through his method for 
    deserialization to work.  It may be a different directory for serialization and 
    unserialization. 
     
    When an SFrame is pickled, a copy of the SFrame is saved in this
    directory and a reference handle to a randomly generated subdirectory is saved in the 
    pickle.  As long as that reference handle is present in the set directory, then
    deserialization should work. 
    
    Note that the pickle files themselves do not contain the data -- both the directory contents
    and the pickle need to be present for deserialization to work. 
    """

    import os

    if _version_info[0] == 2:
        raise RuntimeError("SFrame serialization not supported in python 2.")
    global __serialization_directory
    if serialization_directory is None:
        __serialization_directory = None
        return 

    __serialization_directory = os.path.abspath(os.path.expanduser(serialization_directory))

    # Make sure the directory exists.
    if not os.path.exists(__serialization_directory):

        # Attempt to create it
        os.makedirs(__serialization_directory)

    # Is it a directory? 
    elif not os.path.isdir(__serialization_directory):
        raise ValueError("%s is not a directory." % __serialization_directory)



def get_serialization_directory():
    """
    Returns the current serialization directory if set, or None otherwise. 
    """
    global __serialization_directory

    return __serialization_directory

def _safe_serialization_directory():
    global __serialization_directory

    from pickle import PickleError

    if __serialization_directory is None:
        
        expected_error = TypeError if (_version_info[0] == 3) else PickleError

        raise expected_error("Serialization directory not set to enable pickling or unpickling. "
            "Set using turicreate.data_structures.serialization.enable_sframe_serialization().")
        
    return __serialization_directory
