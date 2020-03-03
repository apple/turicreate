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

#include <aws/core/platform/OSVersionInfo.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <iomanip>

#pragma warning(disable: 4996)
#include <windows.h>
#include <stdio.h>
namespace Aws
{
namespace OSVersionInfo
{

Aws::String GetSysCommandOutput(const char* command)
{
    Aws::String outputStr;
    FILE* outputStream;
    const int maxBufferSize = 256;
    char outputBuffer[maxBufferSize];

    outputStream = _popen(command, "r");

    if (outputStream)
    {
        while (!feof(outputStream))
        {
            if (fgets(outputBuffer, maxBufferSize, outputStream) != nullptr)
            {
                outputStr.append(outputBuffer);
            }
        }

        _pclose(outputStream);

        return Aws::Utils::StringUtils::Trim(outputStr.c_str());
    }

    return {};
}

Aws::String ComputeOSVersionString() 
{
    // With the release of Windows 8.1, the behavior of the GetVersionEx API has changed in the value it will return for the operating system version.
    // The value returned by the GetVersionEx function now depends on how the application is manifested.
    // https://msdn.microsoft.com/en-us/library/windows/desktop/ms724451%28v=vs.85%29.aspx?f=255&MSPPError=-2147217396
    //
    // This only works when the application is manifested for Windows 8.1 or 10, which we don't actually care about.
    // Also, this will cause build headaches for folks not building with VS2015, and is overall an unusable API for us.
    // The following is the least painful but most reliable hack I can come up with.
    //
    // From this article: https://msdn.microsoft.com/en-us/library/windows/desktop/ms724429.aspx
    // we will do the following:
    //
    // To obtain the full version number for the operating system, call the GetFileVersionInfo function on one of the system DLLs, such as Kernel32.dll, 
    // then call VerQueryValue to obtain the \\StringFileInfo\\<lang><codepage>\\ProductVersion subblock of the file version information.
    // 
    Aws::StringStream ss;
    ss << "Windows/";

    DWORD uselessParameter(0);
    static const char* FILE_TO_CHECK = "Kernel32.dll";
    DWORD fileVersionSize = GetFileVersionInfoSizeA(FILE_TO_CHECK, &uselessParameter);
    void* blob = Aws::Malloc("OSVersionInfo", static_cast<size_t>(fileVersionSize));
    bool versionFound(false);

    if (GetFileVersionInfoA(FILE_TO_CHECK, 0, fileVersionSize, blob))
    {
        struct LANGANDCODEPAGE {
            WORD wLanguage;
            WORD wCodePage;
        } *lpTranslate;

        UINT sizeOfCodePage(0);

        if (VerQueryValueA(blob, "\\VarFileInfo\\Translation", (LPVOID*)&lpTranslate, &sizeOfCodePage))
        {
            //we don't actually care which language pack we get, they should all have the same windows version attached.
            Aws::StringStream codePageSS;
            codePageSS << "\\StringFileInfo\\";
            codePageSS << std::setfill('0') << std::setw(4) << std::nouppercase << std::hex << lpTranslate[0].wLanguage;
            codePageSS << std::setfill('0') << std::setw(4) << std::nouppercase << std::hex << lpTranslate[0].wCodePage;
            codePageSS << "\\ProductVersion";

            void* subBlock(nullptr);
            UINT subBlockSize(0);

            if (VerQueryValueA(blob, codePageSS.str().c_str(), &subBlock, &subBlockSize))
            {
                ss << static_cast<const char*>(subBlock);
                versionFound = true;
            }
        }
    }

    Aws::Free(blob);

    if (!versionFound)
    {
        ss << "Unknown Version";
    }



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
