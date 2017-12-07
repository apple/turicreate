/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFilePathChecksum_h
#define cmFilePathChecksum_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <stddef.h>
#include <string>
#include <utility>

class cmMakefile;

/** \class cmFilePathChecksum
 * @brief Generates a checksum for the parent directory of a file
 *
 * The checksum is calculated from the relative file path to the
 * closest known project directory. This guarantees reproducibility
 * when source and build directory differ e.g. for different project
 * build directories.
 */
class cmFilePathChecksum
{
public:
  /// Maximum number of characters to use from the path checksum
  static const size_t partLengthDefault = 10;

  /// @brief Parent directories are empty
  cmFilePathChecksum();

  /// @brief Initilizes the parent directories manually
  cmFilePathChecksum(const std::string& currentSrcDir,
                     const std::string& currentBinDir,
                     const std::string& projectSrcDir,
                     const std::string& projectBinDir);

  /// @brief Initilizes the parent directories from a makefile
  cmFilePathChecksum(cmMakefile* makefile);

  /// @brief Allows parent directories setup after construction
  ///
  void setupParentDirs(const std::string& currentSrcDir,
                       const std::string& currentBinDir,
                       const std::string& projectSrcDir,
                       const std::string& projectBinDir);

  /* @brief Calculates the path checksum for the parent directory of a file
   *
   */
  std::string get(const std::string& filePath) const;

  /* @brief Same as get() but returns only the first length characters
   *
   */
  std::string getPart(const std::string& filePath,
                      size_t length = partLengthDefault) const;

private:
  /// Size of the parent directory list
  static const size_t numParentDirs = 4;
  /// List of (directory name, seed name) pairs
  std::pair<std::string, std::string> parentDirs[numParentDirs];
};

#endif
