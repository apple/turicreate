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
#include <aws/core/platform/FileSystem.h>

#include <aws/core/platform/Environment.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/StringUtils.h>

#include <Userenv.h>

#pragma warning( disable : 4996)

namespace Aws
{
namespace FileSystem
{

static const char* FILE_SYSTEM_UTILS_LOG_TAG = "FileSystem";

Aws::String GetHomeDirectory()
{
    static const char* HOME_DIR_ENV_VAR = "USERPROFILE";

    AWS_LOGSTREAM_TRACE(FILE_SYSTEM_UTILS_LOG_TAG, "Checking " << HOME_DIR_ENV_VAR << " for the home directory.");
    Aws::String homeDir = Aws::Environment::GetEnv(HOME_DIR_ENV_VAR);
    AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "Environment value for variable " << HOME_DIR_ENV_VAR << " is " << homeDir);
    if(homeDir.empty())
    {
        AWS_LOG_WARN(FILE_SYSTEM_UTILS_LOG_TAG, "Home dir not stored in environment, trying to fetch manually from the OS.");
        HANDLE hToken;
    
        if (OpenProcessToken(GetCurrentProcess(), TOKEN_READ, &hToken))
        {
            DWORD len = MAX_PATH;
            CHAR path[MAX_PATH];
            if (GetUserProfileDirectoryA(hToken, path, &len))
            {                
                homeDir = path;
            }
            CloseHandle(hToken);
        }        

        AWS_LOGSTREAM_INFO(FILE_SYSTEM_UTILS_LOG_TAG, "Pulled " << homeDir << " as home directory from the OS.");
    }

    Aws::String retVal = (homeDir.size() > 0) ? Aws::Utils::StringUtils::Trim(homeDir.c_str()) : "";

    if (!retVal.empty())
    {
        if (retVal.at(retVal.length() - 1) != Aws::FileSystem::PATH_DELIM)
        {
            retVal += Aws::FileSystem::PATH_DELIM;
        }
    }
    
    return retVal;
}

bool CreateDirectoryIfNotExists(const char* path)
{
    AWS_LOGSTREAM_INFO(FILE_SYSTEM_UTILS_LOG_TAG, "Creating directory " << path);

    if (CreateDirectoryA(path, nullptr))
    {
        AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "Creation of directory " << path << " succeeded.")
        return true;
    }
    else
    {
        DWORD errorCode = GetLastError();
        AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, " Creation of directory " << path << " returned code: " << errorCode);
        return errorCode == ERROR_ALREADY_EXISTS;
    }
}

bool RemoveFileIfExists(const char* path)
{
    AWS_LOGSTREAM_INFO(FILE_SYSTEM_UTILS_LOG_TAG, "Deleting file: " << path);

    if(DeleteFileA(path))
    {
        AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "Successfully deleted file: " << path);
        return true;
    }
    else
    {
        DWORD errorCode = GetLastError();
        AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "Deletion of file: " << path << " Returned error code: " << errorCode);
        return errorCode == ERROR_FILE_NOT_FOUND;
    }
}

bool RelocateFileOrDirectory(const char* from, const char* to)
{
    AWS_LOGSTREAM_INFO(FILE_SYSTEM_UTILS_LOG_TAG, "Moving file at " << from << " to " << to);

    if(MoveFileA(from, to))
    {
        AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG,  "The moving operation of file at " << from << " to " << to << " Succeeded.");
        return true;
    }
    else
    {
        int errorCode = GetLastError();
        AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG,  "The moving operation of file at " << from << " to " << to << " Returned error code of " << errorCode);
        return false;
    }
}

Aws::String CreateTempFilePath()
{
#ifdef _MSC_VER
#pragma warning(disable: 4996) // _CRT_SECURE_NO_WARNINGS
#endif
    char s_tempName[L_tmpnam_s+1];

    /*
    Prior to VS 2014, tmpnam/tmpnam_s generated root level files ("\filename") which were not appropriate for our usage, so for the windows version, we prepended a '.' to make it a
    tempfile in the current directory.  Starting with VS2014, the behavior of tmpnam/tmpnam_s was changed to be a full, valid filepath based on the
    current user ("C:\Users\username\AppData\Local\Temp\...").

    See the tmpnam section in http://blogs.msdn.com/b/vcblog/archive/2014/06/18/crt-features-fixes-and-breaking-changes-in-visual-studio-14-ctp1.aspx
    for more details.
    */

#if _MSC_VER >= 1900
    tmpnam_s(s_tempName, L_tmpnam_s);
#else
    s_tempName[0] = '.';
    tmpnam_s(s_tempName + 1, L_tmpnam_s);
#endif // _MSC_VER


    return s_tempName;
}

} // namespace FileSystem
} // namespace Aws
