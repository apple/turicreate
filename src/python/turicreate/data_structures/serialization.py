

# SFrames require a disk-backed file or directory to work with.  This directory 
# has to be present to allow for serialization or deserialization. 
__serialization_directory = None 


def enable_sframe_serialization(serialization_directory):
    """
    Enables pickling of sframes through the use of a user-set directory to 
    store the objects.  This directory must be present and set through 
    this method for deserialization to work, but may be a different directory.
    """

    import os

    global __serialization_directory
    if serialization_directory is None:
        __serialization_directory = None
        return 

    __serialization_directory = os.path.abspath(os.path.expanduser(serialization_directory))

    # Make sure the directory exists.
    if not os.path.exists(__serialization_directory):
        raise ValueError("Directory %s does not exist." % __serialization_directory)

    # Is it a directory? 
    if not os.path.isdir(__serialization_directory):
        raise ValueError("%s is not a directory." % __serialization_directory)



def serialization_directory():
    """
    Returns the current serialization directory if set, or None otherwise. 
    """
    global __serialization_directory

    return __serialization_directory

def _safe_serialization_directory():
    global __serialization_directory

    from pickle import PickleError

    if __serialization_directory is None:
        raise PickleError("Serialization directory not set to enable pickling or unpickling. "
            "Set using turicreate.data_structures.serialization.enable_sframe_serialization().")
        
    return __serialization_directory
