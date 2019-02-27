/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmFilePathChecksum_h
#define cmFilePathChecksum_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <array>
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

  /// @brief Initializes the parent directories manually
  cmFilePathChecksum(std::string const& currentSrcDir,
                     std::string const& currentBinDir,
                     std::string const& projectSrcDir,
                     std::string const& projectBinDir);

  /// @brief Initializes the parent directories from a makefile
  cmFilePathChecksum(cmMakefile* makefile);

  /// @brief Allows parent directories setup after construction
  ///
  void setupParentDirs(std::string const& currentSrcDir,
                       std::string const& currentBinDir,
                       std::string const& projectSrcDir,
                       std::string const& projectBinDir);

  /* @brief Calculates the path checksum for the parent directory of a file
   *
   */
  std::string get(std::string const& filePath) const;

  /* @brief Same as get() but returns only the first length characters
   *
   */
  std::string getPart(std::string const& filePath,
                      size_t length = partLengthDefault) const;

private:
  /// List of (directory name, seed name) pairs
  std::array<std::pair<std::string, std::string>, 4> parentDirs;
};

#endif
