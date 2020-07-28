# Copyright (c) 2017, Apple Inc. All rights reserved.
#
# Use of this source code is governed by a BSD-3-clause license that can be
# found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause

from ... import SPECIFICATION_VERSION
from ..._deps import _HAS_LIBSVM
from coremltools import __version__ as ct_version
from coremltools.models import _METADATA_VERSION, _METADATA_SOURCE
from six import string_types as _string_types


def _infer_min_num_features(model):
    # find the largest index of all the support vectors
    max_index = 0
    for i in range(model.l):
        j = 0
        while model.SV[i][j].index != -1:
            cur_last_index = model.SV[i][j].index
            j += 1
        if cur_last_index > max_index:
            max_index = cur_last_index
    return max_index


def convert(libsvm_model, feature_names, target, input_length, probability):
    """Convert a svm model to the protobuf spec.

    This currently supports:
      * C-SVC
      * nu-SVC
      * Epsilon-SVR
      * nu-SVR

    Parameters
    ----------
    model_path: libsvm_model
       Libsvm representation of the model.

    feature_names : [str] | str
        Names of each of the features.

    target: str
        Name of the predicted class column.

    probability: str
        Name of the class probability column. Only used for C-SVC and nu-SVC.

    Returns
    -------
    model_spec: An object of type Model_pb.
        Protobuf representation of the model
    """
    if not (_HAS_LIBSVM):
        raise RuntimeError("libsvm not found. libsvm conversion API is disabled.")

    from libsvm import svm as _svm
    from ...proto import SVM_pb2
    from ...proto import Model_pb2
    from ...proto import FeatureTypes_pb2
    from ...models import MLModel

    svm_type_enum = libsvm_model.param.svm_type

    # Create the spec
    export_spec = Model_pb2.Model()
    export_spec.specificationVersion = SPECIFICATION_VERSION

    if svm_type_enum == _svm.EPSILON_SVR or svm_type_enum == _svm.NU_SVR:
        svm = export_spec.supportVectorRegressor
    else:
        svm = export_spec.supportVectorClassifier

    # Set the features names
    inferred_length = _infer_min_num_features(libsvm_model)
    if isinstance(feature_names, _string_types):
        # input will be a single array
        if input_length == "auto":
            print(
                "[WARNING] Infering an input length of %d. If this is not correct,"
                " use the 'input_length' parameter." % inferred_length
            )
            input_length = inferred_length
        elif inferred_length > input_length:
            raise ValueError(
                "An input length of %d was given, but the model requires an"
                " input of at least %d." % (input_length, inferred_length)
            )

        input = export_spec.description.input.add()
        input.name = feature_names
        input.type.multiArrayType.shape.append(input_length)
        input.type.multiArrayType.dataType = Model_pb2.ArrayFeatureType.DOUBLE

    else:
        # input will be a series of doubles
        if inferred_length > len(feature_names):
            raise ValueError(
                "%d feature names were given, but the model requires at"
                " least %d features." % (len(feature_names), inferred_length)
            )
        for cur_input_name in feature_names:
            input = export_spec.description.input.add()
            input.name = cur_input_name
            input.type.doubleType.MergeFromString(b"")

    # Set target
    output = export_spec.description.output.add()
    output.name = target

    # Set the interface types
    if svm_type_enum == _svm.EPSILON_SVR or svm_type_enum == _svm.NU_SVR:
        export_spec.description.predictedFeatureName = target
        output.type.doubleType.MergeFromString(b"")
        nr_class = 2

    elif svm_type_enum == _svm.C_SVC or svm_type_enum == _svm.NU_SVC:
        export_spec.description.predictedFeatureName = target
        output.type.int64Type.MergeFromString(b"")

        nr_class = len(libsvm_model.get_labels())

        for i in range(nr_class):
            svm.numberOfSupportVectorsPerClass.append(libsvm_model.nSV[i])
            svm.int64ClassLabels.vector.append(libsvm_model.label[i])

        if probability and bool(libsvm_model.probA):
            output = export_spec.description.output.add()
            output.name = probability
            output.type.dictionaryType.MergeFromString(b"")
            output.type.dictionaryType.int64KeyType.MergeFromString(b"")
            export_spec.description.predictedProbabilitiesName = probability

    else:
        raise ValueError(
            "Only the following SVM types are supported: C_SVC, NU_SVC, EPSILON_SVR, NU_SVR"
        )

    if libsvm_model.param.kernel_type == _svm.LINEAR:
        svm.kernel.linearKernel.MergeFromString(
            b""
        )  # Hack to set kernel to an empty type
    elif libsvm_model.param.kernel_type == _svm.RBF:
        svm.kernel.rbfKernel.gamma = libsvm_model.param.gamma
    elif libsvm_model.param.kernel_type == _svm.POLY:
        svm.kernel.polyKernel.degree = libsvm_model.param.degree
        svm.kernel.polyKernel.c = libsvm_model.param.coef0
        svm.kernel.polyKernel.gamma = libsvm_model.param.gamma
    elif libsvm_model.param.kernel_type == _svm.SIGMOID:
        svm.kernel.sigmoidKernel.c = libsvm_model.param.coef0
        svm.kernel.sigmoidKernel.gamma = libsvm_model.param.gamma
    else:
        raise ValueError(
            "Unsupported kernel. The following kernel are supported: linear, RBF, polynomial and sigmoid."
        )

    # set rho
    # also set probA/ProbB only for SVC
    if svm_type_enum == _svm.C_SVC or svm_type_enum == _svm.NU_SVC:
        num_class_pairs = nr_class * (nr_class - 1) // 2
        for i in range(num_class_pairs):
            svm.rho.append(libsvm_model.rho[i])
        if bool(libsvm_model.probA) and bool(libsvm_model.probB):
            for i in range(num_class_pairs):
                svm.probA.append(libsvm_model.probA[i])
                svm.probB.append(libsvm_model.probB[i])
    else:
        svm.rho = libsvm_model.rho[0]

    # set coefficents
    if svm_type_enum == _svm.C_SVC or svm_type_enum == _svm.NU_SVC:
        for _ in range(nr_class - 1):
            svm.coefficients.add()
        for i in range(libsvm_model.l):
            for j in range(nr_class - 1):
                svm.coefficients[j].alpha.append(libsvm_model.sv_coef[j][i])
    else:
        for i in range(libsvm_model.l):
            svm.coefficients.alpha.append(libsvm_model.sv_coef[0][i])

    # set support vectors
    for i in range(libsvm_model.l):
        j = 0
        cur_support_vector = svm.sparseSupportVectors.vectors.add()
        while libsvm_model.SV[i][j].index != -1:
            cur_node = cur_support_vector.nodes.add()
            cur_node.index = libsvm_model.SV[i][j].index
            cur_node.value = libsvm_model.SV[i][j].value
            j += 1

    model = MLModel(export_spec)

    from libsvm import __version__ as libsvm_version

    libsvm_version = "libsvm=={0}".format(libsvm_version)
    model.user_defined_metadata[_METADATA_VERSION] = ct_version
    model.user_defined_metadata[_METADATA_SOURCE] = libsvm_version

    return model
