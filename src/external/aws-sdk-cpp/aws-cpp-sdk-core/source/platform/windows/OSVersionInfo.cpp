/*
  * Copyright 2010-2015 Amazon.com, Inc. or its affiliates. All Rights Reserved.
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

#include <aws/core/platform/OSVersionInfo.h>

#include <aws/core/utils/memory/stl/AWSStringStream.h>

#pragma warning(disable: 4996)
#include <windows.h>

namespace Aws
{
namespace OSVersionInfo
{

Aws::String ComputeOSVersionString() 
{
    OSVERSIONINFOA versionInfo;
    ZeroMemory(&versionInfo, sizeof(OSVERSIONINFOA));
    versionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOA);
    GetVersionExA(&versionInfo);
    Aws::StringStream ss;
    ss << "Windows/" << versionInfo.dwMajorVersion << "." << versionInfo.dwMinorVersion << "." << versionInfo.dwBuildNumber << "-" << versionInfo.szCSDVersion;

    SYSTEM_INFO sysInfo;
    ZeroMemory(&sysInfo, sizeof(SYSTEM_INFO));
    GetSystemInfo(&sysInfo);

    switch (sysInfo.wProcessorArchitecture)
    {
        //PROCESSOR_ARCHIECTURE_AMD64
        case 0x09:
            ss << " AMD64";
            break;
        //PROCESSOR_ARCHITECTURE_IA64
        case 0x06:
            ss << " IA64";
            break;
        //PROCESSOR_ARCHITECTURE_INTEL
        case 0x00:
            ss << " x86";
            break;
        default:
            ss << " Unknown Processor Architecture";
            break;
        }

    return ss.str();
}


} // namespace OSVersionInfo
} // namespace Aws
