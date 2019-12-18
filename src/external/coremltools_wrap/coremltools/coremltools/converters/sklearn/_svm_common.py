# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

"""
Common stuff for SVMs
"""


def _set_kernel(model, spec):
    """
    Takes the sklearn SVM model and returns the spec with the protobuf kernel for that model.
    """
    def gamma_value(model):
        if(model.gamma == 'auto' or model.gamma == 'auto_deprecated'):
            # auto gamma value is 1/num_features
            return 1/float(len(model.support_vectors_[0]))
        else:
            return model.gamma


    result = None
    if(model.kernel == 'linear'):
        spec.kernel.linearKernel.MergeFromString(b'')  # hack to set kernel to an empty type
    elif(model.kernel == 'rbf'):
            spec.kernel.rbfKernel.gamma = gamma_value(model)
    elif(model.kernel == 'poly'):
        spec.kernel.polyKernel.gamma = gamma_value(model)
        spec.kernel.polyKernel.c = model.coef0
        spec.kernel.polyKernel.degree = model.degree
    elif(model.kernel == 'sigmoid'):
        spec.kernel.sigmoidKernel.gamma = gamma_value(model)
        spec.kernel.sigmoidKernel.c = model.coef0
    else:
        raise ValueError('Unsupported kernel. The following kernel are supported: linear, RBF, polynomial and sigmoid.')
    return result

