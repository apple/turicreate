# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ..._deps import HAS_LIBSVM

def load_model(model_path):
    """Load a libsvm model from a path on disk.

    This currently supports:
      * C-SVC
      * NU-SVC
      * Epsilon-SVR
      * NU-SVR

    Parameters
    ----------
    model_path: str
        Path on disk where the libsvm model representation is.

    Returns
    -------
    model: libsvm_model
        A model of the libsvm format.
    """
    if not(HAS_LIBSVM):
        raise RuntimeError('libsvm not found. libsvm conversion API is disabled.')
    
    from svmutil import svm_load_model # From libsvm
    import os
    if (not os.path.exists(model_path)):
        raise IOError("Expected a valid file path. %s does not exist" % model_path)
    return svm_load_model(model_path)
