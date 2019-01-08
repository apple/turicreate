/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "mps_lstm_helper.h"

#define USE_DIAGONAL_PEEPHOLES  1

namespace turi {
namespace neural_net {

const std::string lstm_weight_names_mxnet_format[] = {
    "i2h_i_weight",
    "h2h_i_weight",
    "h2h_i_bias"  ,
    "i2h_f_weight",
    "h2h_f_weight",
    "h2h_f_bias"  ,
    "i2h_c_weight",
    "h2h_c_weight",
    "h2h_c_bias"  ,
    "i2h_o_weight",
    "h2h_o_weight",
    "h2h_o_bias"
};

static void getWMatDimension(MPSRNNMatrixId matrixId, int * nRowsOut, int * nColsOut, int nInputFeatures, int nOutputFeatures)
{
    int nCols = nOutputFeatures;
    int nRows = nOutputFeatures;
    switch (matrixId) {
        case MPSRNNMatrixIdSingleGateInputWeights:
        case MPSRNNMatrixIdLSTMInputGateInputWeights:
        case MPSRNNMatrixIdLSTMForgetGateInputWeights:
        case MPSRNNMatrixIdLSTMMemoryGateInputWeights:
        case MPSRNNMatrixIdLSTMOutputGateInputWeights:
        case MPSRNNMatrixIdGRUInputGateInputWeights:
        case MPSRNNMatrixIdGRURecurrentGateInputWeights:
        case MPSRNNMatrixIdGRUOutputGateInputWeights:
            nCols = nInputFeatures;
            break;
            
        case MPSRNNMatrixIdLSTMInputGateBiasTerms:
        case MPSRNNMatrixIdSingleGateBiasTerms:
        case MPSRNNMatrixIdLSTMForgetGateBiasTerms:
        case MPSRNNMatrixIdLSTMMemoryGateBiasTerms:
        case MPSRNNMatrixIdLSTMOutputGateBiasTerms:
        case MPSRNNMatrixIdGRUInputGateBiasTerms:
        case MPSRNNMatrixIdGRURecurrentGateBiasTerms:
        case MPSRNNMatrixIdGRUOutputGateBiasTerms:
            nRows = 1;
            break;
            
            
        case MPSRNNMatrixIdLSTMInputGateMemoryWeights:
        case MPSRNNMatrixIdLSTMForgetGateMemoryWeights:
        case MPSRNNMatrixIdLSTMMemoryGateMemoryWeights:
        case MPSRNNMatrixIdLSTMOutputGateMemoryWeights:
            if (USE_DIAGONAL_PEEPHOLES)
            {
                nRows = 1;
            }
            break;
        default:
            break;
    }
    *nRowsOut = nRows;
    *nColsOut = nCols;
}


API_AVAILABLE(macos(10.13))
static MPSMatrix * createMPSMatrix(id <MTLDevice> device, int sizeX, int sizeY, MPSDataType dataType, BOOL allowPadding = YES, void * data = NULL)
{
    assert(nil != device);
    assert(sizeX > 0);
    assert(sizeY > 0);
    assert(dataType == MPSDataTypeFloat32 || dataType == MPSDataTypeFloat16);
    @autoreleasepool
    {
        NSUInteger elemSize = (dataType & 63) >> 3;
        NSUInteger rowBytes = [MPSMatrixDescriptor rowBytesForColumns: sizeX dataType: dataType];
        if (NO == allowPadding)
            rowBytes = sizeX * elemSize;
        MPSMatrixDescriptor * mDesc = [MPSMatrixDescriptor matrixDescriptorWithRows: sizeY columns: sizeX rowBytes: rowBytes dataType: dataType];
        id <MTLBuffer> mBuf = [device newBufferWithLength: rowBytes * sizeY options: MTLResourceStorageModeManaged];
        if (data)
            memcpy(mBuf.contents, data, mBuf.length);
        else
            memset(mBuf.contents, 0, mBuf.length);
#if MPS_TARGET_MAC
        [mBuf didModifyRange: NSMakeRange(0, mBuf.length)];
#endif
        MPSMatrix * result = [[MPSMatrix alloc] initWithBuffer: mBuf descriptor: mDesc];
        return result;
    }
}

MPSMatrix * createWeightMatrix(id <MTLDevice> device, MPSRNNMatrixId wMatId, int inputFeatures, int outputFeatures)
{
    int nRows = 0;
    int nCols = 0;
    getWMatDimension(wMatId, &nRows, &nCols, inputFeatures, outputFeatures);
    return createMPSMatrix(device, nCols, nRows, MPSDataTypeFloat32, NO);
}

API_AVAILABLE(macos(10.14))
MPSRNNMatrixId MxnetNameToMatrixId(std::string mat_name) {
    static const std::map<std::string, MPSRNNMatrixId> MXnetNamesToMatId{
        {"i2h_i_weight", MPSRNNMatrixIdLSTMInputGateInputWeights},
        {"h2h_i_weight", MPSRNNMatrixIdLSTMInputGateRecurrentWeights},
        {"h2h_i_bias"  , MPSRNNMatrixIdLSTMInputGateBiasTerms},
        
        {"i2h_f_weight", MPSRNNMatrixIdLSTMForgetGateInputWeights},
        {"h2h_f_weight", MPSRNNMatrixIdLSTMForgetGateRecurrentWeights},
        {"h2h_f_bias"  , MPSRNNMatrixIdLSTMForgetGateBiasTerms},
        
        {"i2h_c_weight", MPSRNNMatrixIdLSTMMemoryGateInputWeights},
        {"h2h_c_weight", MPSRNNMatrixIdLSTMMemoryGateRecurrentWeights},
        {"h2h_c_bias"  , MPSRNNMatrixIdLSTMMemoryGateBiasTerms},
        
        {"i2h_o_weight", MPSRNNMatrixIdLSTMOutputGateInputWeights},
        {"h2h_o_weight", MPSRNNMatrixIdLSTMOutputGateRecurrentWeights},
        {"h2h_o_bias"  , MPSRNNMatrixIdLSTMOutputGateBiasTerms}
    };
    
    auto it = MXnetNamesToMatId.find(mat_name);
    assert(it != MXnetNamesToMatId.end() && "The key %s is not a valid lstm weights key");
    return it->second;
}

API_AVAILABLE(macos(10.13))
MPSVector * MPSMatrixToVector (MPSMatrix * matrix)
{
    MPSDataType dataType = matrix.dataType;
    NSUInteger elemSize = (NSUInteger)(dataType & 127) >> 3;
    NSUInteger nValues = matrix.rows * (matrix.rowBytes / elemSize);
    MPSVectorDescriptor * vDesc = [MPSVectorDescriptor vectorDescriptorWithLength: nValues dataType: dataType];
    MPSVector * result = [[MPSVector alloc] initWithBuffer: matrix.data descriptor: vDesc];
    return result;
}

void printMatrix(MPSMatrix * matrix, const char* name, NSUInteger byteOffset)
{
    printf("Matrix: %s%s\n", name, matrix == NULL ? "(null)" : "");
    if (!matrix)
        return;
    
    assert(matrix.dataType == MPSDataTypeFloat32 && "Sorry, printMatrix() works only with FP32 matrices.");
    assert((matrix.rowBytes & 3) == 0 && "Sorry, Now unaligned matrices.");
    
#if MPS_TARGET_MAC
    id <MTLCommandBuffer> cmdBuf = gCmdQueue.commandBuffer;
    id <MTLBlitCommandEncoder> blitEncoder = [cmdBuf blitCommandEncoder];
    [blitEncoder synchronizeResource: matrix.data];
    [blitEncoder endEncoding];
    [cmdBuf commit];
    [cmdBuf waitUntilCompleted];
#endif
    
    
    const float * data = (const float *)((char*)matrix.data.contents + byteOffset);
    int sizeY = (int)matrix.rows;
    int sizeX = (int)matrix.columns;
    int fStride = (int)matrix.rowBytes >> 2;
    
    for (int i = 0; i < sizeY; i++)
    {
        printf("Row %3d: [ %9.4f", i, data[ 0 + i * fStride]);
        for (int j = 1; j < sizeX; j++)
        {
            printf(", %9.4f", data[ j + i * fStride]);
        }
        printf("]\n");
    }
}

}  // namespace neural_net
}  // namespace turi
