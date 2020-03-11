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
#include <aws/core/platform/FileSystem.h>

#include <aws/core/platform/Android.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws/core/utils/StringUtils.h>

#include <unistd.h>
#include <sys/stat.h>
#include <cerrno>
#include <dirent.h>
#include <cassert>

#include <mutex>

namespace Aws
{
namespace FileSystem
{

static const char* FILE_SYSTEM_UTILS_LOG_TAG = "FileSystem";

    class AndroidDirectory : public Directory
    {
    public:
        AndroidDirectory(const Aws::String& path, const Aws::String& relativePath) : Directory(path, relativePath), m_dir(nullptr)
        {
            m_dir = opendir(m_directoryEntry.path.c_str());
            AWS_LOGSTREAM_TRACE(FILE_SYSTEM_UTILS_LOG_TAG, "Entering directory " << m_directoryEntry.path);

            if (m_dir)
            {
                AWS_LOGSTREAM_TRACE(FILE_SYSTEM_UTILS_LOG_TAG, "Successfully opened directory " << m_directoryEntry.path);
                m_directoryEntry.fileType = FileType::Directory;
            }
            else
            {
                AWS_LOGSTREAM_ERROR(FILE_SYSTEM_UTILS_LOG_TAG, "Could not load directory " << m_directoryEntry.path << " with error code " << errno);
            }
        }

        ~AndroidDirectory()
        {
            if (m_dir)
            {
                closedir(m_dir);
            }
        }

        operator bool() const override { return m_directoryEntry.operator bool() && m_dir != nullptr; }

        DirectoryEntry Next() override
        {
            assert(m_dir);
            DirectoryEntry entry;

            dirent* dirEntry;
            bool invalidEntry(true);

            while(invalidEntry)
            {
                if ((dirEntry = readdir(m_dir)))
                {
                    Aws::String entryName = dirEntry->d_name;
                    if(entryName != ".." && entryName != ".")
                    {
                        entry = ParseFileInfo(dirEntry, true);
                        invalidEntry = false;
                    }
                }
                else
                {
                    break;
                }
            }

            return entry;
        }

    private:
        DirectoryEntry ParseFileInfo(dirent* dirEnt, bool computePath)
        {
            DirectoryEntry entry;

            if(computePath)
            {
                Aws::StringStream ss;
                ss << m_directoryEntry.path << PATH_DELIM << dirEnt->d_name;
                entry.path = ss.str();

                ss.str("");
                if(m_directoryEntry.relativePath.empty())
                {
                    ss << dirEnt->d_name;
                }
                else
                {
                    ss << m_directoryEntry.relativePath << PATH_DELIM << dirEnt->d_name;
                }
                entry.relativePath = ss.str();
            }
            else
            {
                entry.path = m_directoryEntry.path;
                entry.relativePath = m_directoryEntry.relativePath;
            }

            AWS_LOGSTREAM_TRACE(FILE_SYSTEM_UTILS_LOG_TAG, "Calling stat on path " << entry.path);

            struct stat dirInfo;
            if(!lstat(entry.path.c_str(), &dirInfo))
            {
               if(S_ISDIR(dirInfo.st_mode))
               {
                   AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "type directory detected");
                   entry.fileType = FileType::Directory;
               }
               else if(S_ISLNK(dirInfo.st_mode))
               {
                   AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "type symlink detected");
                   entry.fileType = FileType::Symlink;
               }
               else if(S_ISREG(dirInfo.st_mode))
               {
                   AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "type file detected");
                   entry.fileType = FileType::File;
               }

               entry.fileSize = static_cast<int64_t>(dirInfo.st_size);
               AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "file size detected as " << entry.fileSize);
            }
            else
            {
                AWS_LOGSTREAM_ERROR(FILE_SYSTEM_UTILS_LOG_TAG, "Failed to stat file path " << entry.path << " with error code " << errno);
            }

            return entry;
        }

        DIR* m_dir;
    };

Aws::String GetHomeDirectory()
{
    return Aws::Platform::GetCacheDirectory();
}

bool CreateDirectoryIfNotExists(const char* path, bool createParentDirs)
{
    Aws::String directoryName = path;
    AWS_LOGSTREAM_INFO(FILE_SYSTEM_UTILS_LOG_TAG, "Creating directory " << directoryName);

    for (size_t i = createParentDirs ? 0 : directoryName.size() - 1; i < directoryName.size(); i++)
    {
        if (i != 0 && (directoryName[i] == FileSystem::PATH_DELIM || i == directoryName.size() - 1))
        {
            if (directoryName[i] == FileSystem::PATH_DELIM)
            {
                directoryName[i] = '\0';
            }
	    int errorCode = mkdir(directoryName.c_str(), S_IRWXU | S_IRWXG | S_IRWXO);
	    if (errorCode != 0 && errno != EEXIST)
	    {
                AWS_LOGSTREAM_ERROR(FILE_SYSTEM_UTILS_LOG_TAG, "Creation of directory " << directoryName.c_str() << " returned code: " << errno);
                return false;
	    }
            AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "Creation of directory " << directoryName.c_str() << " returned code: " << errno);
            directoryName[i] = FileSystem::PATH_DELIM;
        }
    }
    return true;
}

bool RemoveFileIfExists(const char* path)
{
    AWS_LOGSTREAM_INFO(FILE_SYSTEM_UTILS_LOG_TAG, "Deleting file: " << path);

    int errorCode = unlink(path);
    AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "Deletion of file: " << path << " Returned error code: " << errno);
    return errorCode == 0 || errno == ENOENT;
}

bool RemoveDirectoryIfExists(const char* path)
{
    AWS_LOGSTREAM_INFO(FILE_SYSTEM_UTILS_LOG_TAG, "Deleting directory: " << path);
    int errorCode = rmdir(path);
    AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "Deletion of directory: " << path << " Returned error code: " << errno);
    return errorCode == 0 || errno == ENOTDIR || errno == ENOENT;
}

bool RelocateFileOrDirectory(const char* from, const char* to)
{
    AWS_LOGSTREAM_INFO(FILE_SYSTEM_UTILS_LOG_TAG, "Moving file at " << from << " to " << to);

    int errorCode = rename(from, to);

    AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG,  "The moving operation of file at " << from << " to " << to << " Returned error code of " << errno);
    return errorCode == 0;
}

std::mutex tempFileMutex;
uint32_t tempFileIndex(0);

Aws::String CreateTempFilePath()
{
    std::lock_guard<std::mutex> tempFileLock(tempFileMutex);

    Aws::StringStream pathStream;
    pathStream << Aws::Platform::GetCacheDirectory();
    pathStream << PATH_DELIM << "temp" << tempFileIndex;
    ++tempFileIndex;

    AWS_LOGSTREAM_DEBUG(FILE_SYSTEM_UTILS_LOG_TAG, "CreateTempFilePath generated: " << pathStream.str());

    return pathStream.str();
}

Aws::String GetExecutableDirectory()
{
    char dest[PATH_MAX];
    size_t destSize = sizeof(dest);
    memset(dest, 0, destSize);

    if (readlink("/proc/self/exe", dest, destSize))
    {
        Aws::String executablePath(dest);
        auto lastSlash = executablePath.find_last_of('/');
        if (lastSlash != std::string::npos)
        {
            return executablePath.substr(0, lastSlash);
        }
    }

    return "./";
}

Aws::UniquePtr<Directory> OpenDirectory(const Aws::String& path, const Aws::String& relativePath)
{
    return Aws::MakeUnique<AndroidDirectory>(FILE_SYSTEM_UTILS_LOG_TAG, path, relativePath);
}

} // namespace FileSystem
} // namespace Aws

