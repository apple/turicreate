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

using namespace Aws::Utils;

Aws::String PathUtils::GetFileNameFromPathWithoutExt(const Aws::String& path)
{
    Aws::String fileName = Aws::Utils::PathUtils::GetFileNameFromPathWithExt(path);
    size_t endPos = fileName.find_last_of('.');
    if (endPos == std::string::npos)
    {
        return fileName;
    } 
    if (endPos == 0) // fileName is "."
    {
        return {};
    }

    return fileName.substr(0, endPos);
}

Aws::String PathUtils::GetFileNameFromPathWithExt(const Aws::String& path)
{
    if (path.size() == 0) 
    {	
        return path;
    }

    size_t startPos = path.find_last_of(Aws::FileSystem::PATH_DELIM);
    if (startPos == path.size() - 1)
    {
        return {};
    }

    if (startPos == std::string::npos)
    {
        startPos = 0;
    }
    else 
    {
        startPos += 1;
    }

    size_t endPos = path.size() - 1;
    
    return path.substr(startPos, endPos - startPos + 1);
}
