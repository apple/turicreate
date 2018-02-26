# -*- coding: utf-8 -*-
# Copyright © 2017 Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can
# be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
from __future__ import print_function as _
from __future__ import division as _
from __future__ import absolute_import as _
from . import util as _util, toolkits as _toolkits, SFrame as _SFrame, SArray as _SArray, \
    SGraph as _SGraph, load_sgraph as _load_graph

from .util import _get_aws_credentials as _util_get_aws_credentials, \
    cloudpickle as _cloudpickle

import pickle as _pickle
import uuid as _uuid
import os as _os
import zipfile as _zipfile
import shutil as _shutil
import atexit as _atexit
import glob as _glob

def _get_aws_credentials():
    (key, secret) = _util_get_aws_credentials()
    return {'aws_access_key_id': key, 'aws_secret_access_key': secret}

def _get_temp_filename():
    return _util._make_temp_filename(prefix='gl_pickle_')

def _get_tmp_file_location():
    return _util._make_temp_directory(prefix='gl_pickle_')

def _is_not_pickle_safe_gl_model_class(obj_class):
    """
    Check if a Turi create model is pickle safe.

    The function does it by checking that _CustomModel is the base class.

    Parameters
    ----------
    obj_class    : Class to be checked.

    Returns
    ----------
    True if the GLC class is a model and is pickle safe.

    """
    if issubclass(obj_class, _toolkits._model.CustomModel):
        return not obj_class._is_gl_pickle_safe()
    return False

def _is_not_pickle_safe_gl_class(obj_class):
    """
    Check if class is a Turi create model.

    The function does it by checking the method resolution order (MRO) of the
    class and verifies that _Model is the base class.

    Parameters
    ----------
    obj_class    : Class to be checked.

    Returns
    ----------
    True if the class is a GLC Model.

    """
    gl_ds = [_SFrame, _SArray, _SGraph]

    # Object is GLC-DS or GLC-Model
    return (obj_class in gl_ds) or _is_not_pickle_safe_gl_model_class(obj_class)

def _get_gl_class_type(obj_class):
    """
    Internal util to get the type of the GLC class. The pickle file stores
    this name so that it knows how to construct the object on unpickling.

    Parameters
    ----------
    obj_class    : Class which has to be categorized.

    Returns
    ----------
    A class type for the pickle file to save.

    """

    if obj_class == _SFrame:
        return "SFrame"
    elif obj_class == _SGraph:
        return "SGraph"
    elif obj_class == _SArray:
        return "SArray"
    elif _is_not_pickle_safe_gl_model_class(obj_class):
        return "Model"
    else:
        return None

def _get_gl_object_from_persistent_id(type_tag, gl_archive_abs_path):
    """
    Internal util to get a GLC object from a persistent ID in the pickle file.

    Parameters
    ----------
    type_tag : The name of the glc class as saved in the GLC pickler.

    gl_archive_abs_path: An absolute path to the GLC archive where the
                          object was saved.

    Returns
    ----------
    The GLC object.

    """
    if type_tag == "SFrame":
        obj = _SFrame(gl_archive_abs_path)
    elif type_tag == "SGraph":
        obj = _load_graph(gl_archive_abs_path)
    elif type_tag == "SArray":
        obj = _SArray(gl_archive_abs_path)
    elif type_tag == "Model":
        from . import load_model as _load_model
        obj = _load_model(gl_archive_abs_path)
    else:
        raise _pickle.UnpicklingError("Turi pickling Error: Unsupported object."
              " Only SFrames, SGraphs, SArrays, and Models are supported.")
    return obj

class GLPickler(_cloudpickle.CloudPickler):

    def _to_abs_path_set(self, l):
        return set([_os.path.abspath(x) for x in l])

    """

    # GLC pickle works with:
    #
    # (1) Regular python objects
    # (2) SArray
    # (3) SFrame
    # (4) SGraph
    # (5) Models
    # (6) Any combination of (1) - (5)

    Examples
    --------

    To pickle a collection of objects into a single file:

    .. sourcecode:: python

        from turicreate.util import gl_pickle
        import turicreate as tc

        obj = {'foo': tc.SFrame([1,2,3]),
               'bar': tc.SArray([1,2,3]),
               'foo-bar': ['foo-and-bar', tc.SFrame()]}

        # Setup the GLC pickler
        pickler = gl_pickle.GLPickler(filename = 'foo-bar')
        pickler.dump(obj)

        # The pickler has to be closed to make sure the files get closed.
        pickler.close()

    To unpickle the collection of objects:

    .. sourcecode:: python

        unpickler = gl_pickle.GLUnpickler(filename = 'foo-bar')
        obj = unpickler.load()
        unpickler.close()
        print obj

    The GLC pickler needs a temporary working directory to manage GLC objects.
    This temporary working path must be a local path to the file system. It
    can also be a relative path in the FS.

    .. sourcecode:: python

        unpickler = gl_pickle.GLUnpickler('foo-bar')
        obj = unpickler.load()
        unpickler.close()
        print obj


    Notes
    --------

    The GLC pickler saves the files into single zip archive with the following
    file layout.

    pickle_file_name: Name of the file in the archive that contains
                      the name of the pickle file.
                      The comment in the ZipFile contains the version number
                      of the GLC pickler used.

    "pickle_file": The pickle file that stores all python objects. For GLC objects
                   the pickle file contains a tuple with (ClassName, relative_path)
                   which stores the name of the GLC object type and a relative
                   path (in the zip archive) which points to the GLC archive
                   root directory.

    "gl_archive_dir_1" : A directory which is the GLC archive for a single
                          object.

     ....

    "gl_archive_dir_N"



    """
    def __init__(self, filename, protocol = -1, min_bytes_to_save = 0):
        """

        Construct a  GLC pickler.

        Parameters
        ----------
        filename  : Name of the file to write to. This file is all you need to pickle
                    all objects (including GLC objects).

        protocol  : Pickle protocol (see pickle docs). Note that all pickle protocols
                    may not be compatible with GLC objects.

        min_bytes_to_save : Cloud pickle option (see cloud pickle docs).

        Returns
        ----------
        GLC pickler.

        """
        # Zipfile
        # --------
        # Version 1: GLC 1.2.1
        #
        # Directory:
        # ----------
        # Version 1: GLC 1.4: 1

        self.archive_filename = None
        self.gl_temp_storage_path = _get_tmp_file_location()
        self.gl_object_memo = set()
        self.mark_for_delete = set()

        # Make sure the directory exists.
        filename = _os.path.abspath(
                     _os.path.expanduser(
                       _os.path.expandvars(filename)))
        if not _os.path.exists(filename):
            _os.makedirs(filename)
        elif _os.path.isdir(filename):
            self.mark_for_delete = self._to_abs_path_set(
                         _glob.glob(_os.path.join(filename, "*")))
            self.mark_for_delete -= self._to_abs_path_set(
                    [_os.path.join(filename, 'pickle_archive'),
                     _os.path.join(filename, 'version')])

        elif _os.path.isfile(filename):
           _os.remove(filename)
           _os.makedirs(filename)

        # Create a new directory.
        self.gl_temp_storage_path = filename

        # The pickle file where all the Python objects are saved.
        relative_pickle_filename = "pickle_archive"
        pickle_filename = _os.path.join(self.gl_temp_storage_path,
                                        relative_pickle_filename)

        try:
            # Initialize the pickle file with cloud _pickle. Note, cloud pickle
            # takes a file handle for initialization.
            self.file = open(pickle_filename, 'wb')
            _cloudpickle.CloudPickler.__init__(self, self.file, protocol)
        except IOError as err:
            print("Turi create pickling error: %s" % err)

        # Write the version number.
        with open(_os.path.join(self.gl_temp_storage_path, 'version'), 'w') as f:
            f.write("1.0")

    def dump(self, obj):
        _cloudpickle.CloudPickler.dump(self, obj)

    def persistent_id(self, obj):
        """
        Provide a persistent ID for "saving" GLC objects by reference. Return
        None for all non GLC objects.

        Parameters
        ----------

        obj: Name of the object whose persistent ID is extracted.

        Returns
        --------
        None if the object is not a GLC object. (ClassName, relative path)
        if the object is a GLC object.

        Notes
        -----

        Borrowed from pickle docs (https://docs.python.org/2/library/_pickle.html)

        For the benefit of object persistence, the pickle module supports the
        notion of a reference to an object outside the pickled data stream.

        To pickle objects that have an external persistent id, the pickler must
        have a custom persistent_id() method that takes an object as an argument and
        returns either None or the persistent id for that object.

        For GLC objects, the persistent_id is merely a relative file path (within
        the ZIP archive) to the GLC archive where the GLC object is saved. For
        example:

            (SFrame, 'sframe-save-path')
            (SGraph, 'sgraph-save-path')
            (Model, 'model-save-path')

        """

        # Get the class of the object (if it can be done)
        obj_class = None if not hasattr(obj, '__class__') else obj.__class__
        if obj_class is None:
            return None

        # If the object is a GLC class.
        if _is_not_pickle_safe_gl_class(obj_class):
            if (id(obj) in self.gl_object_memo):
                # has already been pickled
                return (None, None, id(obj))
            else:
                # Save the location of the GLC object's archive to the pickle file.
                relative_filename = str(_uuid.uuid4())
                filename = _os.path.join(self.gl_temp_storage_path, relative_filename)
                self.mark_for_delete -= set([filename])

                # Save the GLC object
                obj.save(filename)

                # Memoize.
                self.gl_object_memo.add(id(obj))

                # Return the tuple (class_name, relative_filename) in archive.
                return (_get_gl_class_type(obj.__class__), relative_filename, id(obj))

        # Not a GLC object. Default to cloud pickle
        else:
            return None

    def close(self):
        """
        Close the pickle file, and the zip archive file. The single zip archive
        file can now be shipped around to be loaded by the unpickler.
        """
        if self.file is None:
            return

        # Close the pickle file.
        self.file.close()
        self.file = None

        for f in self.mark_for_delete:
            error = [False]

            def register_error(*args):
                error[0] = True

            _shutil.rmtree(f, onerror = register_error)

            if error[0]:
                _atexit.register(_shutil.rmtree, f, ignore_errors=True)

    def __del__(self):
        self.close()

class GLUnpickler(_pickle.Unpickler):
    """
    # GLC unpickler works with a GLC pickler archive or a regular pickle
    # archive.
    #
    # Works with
    # (1) GLPickler archive
    # (2) Cloudpickle archive
    # (3) Python pickle archive

    Examples
    --------
    To unpickle the collection of objects:

    .. sourcecode:: python

        unpickler = gl_pickle.GLUnpickler('foo-bar')
        obj = unpickler.load()
        print obj

    """

    def __init__(self, filename):
        """
        Construct a GLC unpickler.

        Parameters
        ----------
        filename  : Name of the file to read from. The file can be a GLC pickle
                    file, a cloud pickle file, or a python pickle file.
        Returns
        ----------
        GLC unpickler.
        """
        self.gl_object_memo = {}
        self.pickle_filename = None
        self.tmp_file = None
        self.file = None
        self.gl_temp_storage_path = _get_tmp_file_location()

        # GLC 1.3 used Zipfiles for storing the objects.
        self.directory_mode = True
        filename = _os.path.abspath(
                     _os.path.expanduser(
                       _os.path.expandvars(filename)))
        if not _os.path.exists(filename):
            raise IOError('%s is not a valid file name.' % filename)

        # GLC 1.3 Pickle file
        if _zipfile.is_zipfile(filename):
            self.directory_mode = False
            pickle_filename = None

            # Get the pickle file name.
            zf = _zipfile.ZipFile(filename, allowZip64=True)
            for info in zf.infolist():
                if info.filename == 'pickle_file':
                    pickle_filename = zf.read(info.filename).decode()
            if pickle_filename is None:
                raise IOError(("Cannot pickle file of the given format. File"
                        " must be one of (a) GLPickler archive, "
                        "(b) Cloudpickle archive, or (c) python pickle archive."))

            # Extract the zip file.
            try:
                outpath = self.gl_temp_storage_path
                zf.extractall(outpath)
            except IOError as err:
                print("Turi pickle extraction error: %s " % err)

            self.pickle_filename = _os.path.join(self.gl_temp_storage_path,
                                                 pickle_filename)

        # GLC Pickle directory mode.
        elif _os.path.isdir(filename):
            self.directory_mode = True
            pickle_filename = _os.path.join(filename, "pickle_archive")
            if not _os.path.exists(pickle_filename):
                raise IOError("Corrupted archive: Missing pickle file %s." % pickle_filename)
            if not _os.path.exists(_os.path.join(filename, "version")):
                raise IOError("Corrupted archive: Missing version file.")
            self.pickle_filename = pickle_filename
            self.gl_temp_storage_path = _os.path.abspath(filename)

        # Pure pickle file.
        else:
            self.directory_mode = False
            self.pickle_filename = filename

        self.file = open(self.pickle_filename, 'rb')
        _pickle.Unpickler.__init__(self, self.file)


    def persistent_load(self, pid):
        """
        Reconstruct a GLC object using the persistent ID.

        This method should not be used externally. It is required by the unpickler super class.

        Parameters
        ----------
        pid      : The persistent ID used in pickle file to save the GLC object.

        Returns
        ----------
        The GLC object.
        """
        if len(pid) == 2:
            # Pre GLC-1.3 release behavior, without memorization
            type_tag, filename = pid
            abs_path = _os.path.join(self.gl_temp_storage_path, filename)
            return  _get_gl_object_from_persistent_id(type_tag, abs_path)
        else:
            # Post GLC-1.3 release behavior, with memorization
            type_tag, filename, object_id = pid
            if object_id in self.gl_object_memo:
                return self.gl_object_memo[object_id]
            else:
                abs_path = _os.path.join(self.gl_temp_storage_path, filename)
                obj = _get_gl_object_from_persistent_id(type_tag, abs_path)
                self.gl_object_memo[object_id] = obj
                return obj

    def close(self):
        """
        Clean up files that were created.
        """
        if self.file:
            self.file.close()
            self.file = None

        # If temp_file is a folder, we do not remove it because we may
        # still need it after the unpickler is disposed
        if self.tmp_file and _os.path.isfile(self.tmp_file):
            _os.remove(self.tmp_file)
            self.tmp_file = None

    def __del__(self):
        """
        Clean up files that were created.
        """
        self.close()
