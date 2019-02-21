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

cmFilePathChecksum::cmFilePathChecksum(std::string const& currentSrcDir,
                                       std::string const& currentBinDir,
                                       std::string const& projectSrcDir,
                                       std::string const& projectBinDir)
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

void cmFilePathChecksum::setupParentDirs(std::string const& currentSrcDir,
                                         std::string const& currentBinDir,
                                         std::string const& projectSrcDir,
                                         std::string const& projectBinDir)
{
  this->parentDirs[0].first = cmSystemTools::GetRealPath(currentSrcDir);
  this->parentDirs[1].first = cmSystemTools::GetRealPath(currentBinDir);
  this->parentDirs[2].first = cmSystemTools::GetRealPath(projectSrcDir);
  this->parentDirs[3].first = cmSystemTools::GetRealPath(projectBinDir);

  this->parentDirs[0].second = "CurrentSource";
  this->parentDirs[1].second = "CurrentBinary";
  this->parentDirs[2].second = "ProjectSource";
  this->parentDirs[3].second = "ProjectBinary";
}

std::string cmFilePathChecksum::get(std::string const& filePath) const
{
  std::string relPath;
  std::string relSeed;
  {
    std::string const fileReal = cmSystemTools::GetRealPath(filePath);
    std::string parentDir;
    // Find closest project parent directory
    for (auto const& pDir : this->parentDirs) {
      if (!pDir.first.empty() &&
          cmsys::SystemTools::IsSubDirectory(fileReal, pDir.first)) {
        parentDir = pDir.first;
        relSeed = pDir.second;
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
  return cmBase32Encoder().encodeString(&hashBytes.front(), hashBytes.size(),
                                        false);
}

std::string cmFilePathChecksum::getPart(std::string const& filePath,
                                        size_t length) const
{
  return get(filePath).substr(0, length);
}
