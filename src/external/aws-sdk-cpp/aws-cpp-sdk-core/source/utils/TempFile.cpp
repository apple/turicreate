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

#include <aws/core/utils/FileSystemUtils.h>

#include <aws/core/platform/FileSystem.h>

namespace Aws
{
    namespace Utils
    {
        static Aws::String ComputeTempFileName(const char* prefix, const char* suffix)
        {
            Aws::String prefixStr;

            if (prefix)
            {
                prefixStr = prefix;
            }

            Aws::String suffixStr;

            if (suffix)
            {
                suffixStr = suffix;
            }

            return prefixStr + Aws::FileSystem::CreateTempFilePath() + suffixStr;
        }

        TempFile::TempFile(const char* prefix, const char* suffix, std::ios_base::openmode openFlags) :
            FStreamWithFileName(ComputeTempFileName(prefix, suffix).c_str(), openFlags)
        {
        }

        TempFile::TempFile(const char* prefix, std::ios_base::openmode openFlags) :
            FStreamWithFileName(ComputeTempFileName(prefix, nullptr).c_str(), openFlags)
        {
        }

        TempFile::TempFile(std::ios_base::openmode openFlags) :
            FStreamWithFileName(ComputeTempFileName(nullptr, nullptr).c_str(), openFlags)
        {
        }


        TempFile::~TempFile()
        {
            Aws::FileSystem::RemoveFileIfExists(m_fileName.c_str());
        }
    }
}