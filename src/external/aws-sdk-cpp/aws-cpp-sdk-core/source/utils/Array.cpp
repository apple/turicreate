/*
  * Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
  *
  * Licensed under the Apache License, Version 2.0 (the "License").
  * You may not use this file except in compliance with the License.
  * A copy of the License is located at
  *
  *  http://aws.amazon.com/apache2.0
  *
  * or in the "license" file accompanying this file. This file is distributed
  * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
  * express or implied. See the License for the specific language governing
  * permissions and limitations under the License.
  */

#include <aws/core/utils/Array.h>

#include <aws/core/platform/Security.h>

namespace Aws
{
    namespace Utils
    {
            Array<CryptoBuffer> CryptoBuffer::Slice(size_t sizeOfSlice) const
            {
                assert(sizeOfSlice <= GetLength());

                size_t numberOfSlices = (GetLength() + sizeOfSlice - 1) / sizeOfSlice;
                size_t currentSliceIndex = 0;
                Array<CryptoBuffer> slices(numberOfSlices);

                for (size_t i = 0; i < numberOfSlices - 1; ++i)
                {
                    CryptoBuffer newArray(sizeOfSlice);
                    for (size_t cpyIdx = 0; cpyIdx < newArray.GetLength(); ++cpyIdx)
                    {
                        newArray[cpyIdx] = GetItem(cpyIdx + currentSliceIndex);
                    }
                    currentSliceIndex += sizeOfSlice;
                    slices[i] = std::move(newArray);
                }

                CryptoBuffer lastArray(GetLength() % sizeOfSlice == 0 ? sizeOfSlice : GetLength() % sizeOfSlice );
                for (size_t cpyIdx = 0; cpyIdx < lastArray.GetLength(); ++cpyIdx)
                {
                    lastArray[cpyIdx] = GetItem(cpyIdx + currentSliceIndex);
                }
                slices[slices.GetLength() - 1] = std::move(lastArray);

                return slices;
            }            

            CryptoBuffer& CryptoBuffer::operator^(const CryptoBuffer& operand)
            {
                size_t smallestSize = std::min<size_t>(GetLength(), operand.GetLength());
                for (size_t i = 0; i < smallestSize; ++i)
                {
                    (*this)[i] ^= operand[i];
                }

                return *this;
            }

            /**
            * Zero out the array securely
            */
            void CryptoBuffer::Zero()
            {
                if (GetUnderlyingData())
                {
                    Aws::Security::SecureMemClear(GetUnderlyingData(), GetLength());
                }
            }
    }
}