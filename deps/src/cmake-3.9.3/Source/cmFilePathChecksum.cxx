/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmFilePathChecksum.h"

#include "cmBase32.h"
#include "cmCryptoHash.h"
#include "cmMakefile.h"
#include "cmSystemTools.h"

#include <vector>

cmFilePathChecksum::cmFilePathChecksum()
{
}

cmFilePathChecksum::cmFilePathChecksum(const std::string& currentSrcDir,
                                       const std::string& currentBinDir,
                                       const std::string& projectSrcDir,
                                       const std::string& projectBinDir)
{
  setupParentDirs(currentSrcDir, currentBinDir, projectSrcDir, projectBinDir);
}

cmFilePathChecksum::cmFilePathChecksum(cmMakefile* makefile)
{
  setupParentDirs(makefile->GetCurrentSourceDirectory(),
                  makefile->GetCurrentBinaryDirectory(),
                  makefile->GetHomeDirectory(),
                  makefile->GetHomeOutputDirectory());
}

void cmFilePathChecksum::setupParentDirs(const std::string& currentSrcDir,
                                         const std::string& currentBinDir,
                                         const std::string& projectSrcDir,
                                         const std::string& projectBinDir)
{
  parentDirs[0].first = cmsys::SystemTools::GetRealPath(currentSrcDir);
  parentDirs[1].first = cmsys::SystemTools::GetRealPath(currentBinDir);
  parentDirs[2].first = cmsys::SystemTools::GetRealPath(projectSrcDir);
  parentDirs[3].first = cmsys::SystemTools::GetRealPath(projectBinDir);

  parentDirs[0].second = "CurrentSource";
  parentDirs[1].second = "CurrentBinary";
  parentDirs[2].second = "ProjectSource";
  parentDirs[3].second = "ProjectBinary";
}

std::string cmFilePathChecksum::get(const std::string& filePath) const
{
  std::string relPath;
  std::string relSeed;
  {
    const std::string fileReal = cmsys::SystemTools::GetRealPath(filePath);
    std::string parentDir;
    // Find closest project parent directory
    for (size_t ii = 0; ii != numParentDirs; ++ii) {
      const std::string& pDir = parentDirs[ii].first;
      if (!pDir.empty() &&
          cmsys::SystemTools::IsSubDirectory(fileReal, pDir)) {
        relSeed = parentDirs[ii].second;
        parentDir = pDir;
        break;
      }
    }
    // Use file system root as fallback parent directory
    if (parentDir.empty()) {
      relSeed = "FileSystemRoot";
      cmsys::SystemTools::SplitPathRootComponent(fileReal, &parentDir);
    }
    // Calculate relative path from project parent directory
    relPath = cmsys::SystemTools::RelativePath(
      parentDir, cmsys::SystemTools::GetParentDirectory(fileReal));
  }

  // Calculate the file ( seed + relative path ) binary checksum
  std::vector<unsigned char> hashBytes =
    cmCryptoHash(cmCryptoHash::AlgoSHA256).ByteHashString(relSeed + relPath);

  // Convert binary checksum to string
  return cmBase32Encoder().encodeString(&hashBytes[0], hashBytes.size(),
                                        false);
}

std::string cmFilePathChecksum::getPart(const std::string& filePath,
                                        size_t length) const
{
  return get(filePath).substr(0, length);
}
