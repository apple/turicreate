/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmQtAutoGen_h
#define cmQtAutoGen_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

/** \class cmQtAutoGen
 * \brief Common base class for QtAutoGen classes
 */
class cmQtAutoGen
{
public:
  /// @brief Nested lists separator
  static std::string const ListSep;
  /// @brief Maximum number of parallel threads/processes in a generator
  static unsigned int const ParallelMax;

  /// @brief AutoGen generator type
  enum class GeneratorT
  {
    GEN, // General
    MOC,
    UIC,
    RCC
  };

  /// @brief Integer version
  struct IntegerVersion
  {
    unsigned int Major = 0;
    unsigned int Minor = 0;

    IntegerVersion() = default;
    IntegerVersion(unsigned int major, unsigned int minor)
      : Major(major)
      , Minor(minor)
    {
    }

    bool operator>(IntegerVersion const version)
    {
      return (this->Major > version.Major) ||
        ((this->Major == version.Major) && (this->Minor > version.Minor));
    }

    bool operator>=(IntegerVersion const version)
    {
      return (this->Major > version.Major) ||
        ((this->Major == version.Major) && (this->Minor >= version.Minor));
    }
  };

public:
  /// @brief Returns the generator name
  static std::string const& GeneratorName(GeneratorT genType);
  /// @brief Returns the generator name in upper case
  static std::string GeneratorNameUpper(GeneratorT genType);

  /// @brief Returns the string escaped and enclosed in quotes
  static std::string Quoted(std::string const& text);

  static std::string QuotedCommand(std::vector<std::string> const& command);

  /// @brief Returns the parent directory of the file with a "/" suffix
  static std::string SubDirPrefix(std::string const& filename);

  /// @brief Appends the suffix to the filename before the last dot
  static std::string AppendFilenameSuffix(std::string const& filename,
                                          std::string const& suffix);

  /// @brief Merges newOpts into baseOpts
  static void UicMergeOptions(std::vector<std::string>& baseOpts,
                              std::vector<std::string> const& newOpts,
                              bool isQt5);

  /// @brief Merges newOpts into baseOpts
  static void RccMergeOptions(std::vector<std::string>& baseOpts,
                              std::vector<std::string> const& newOpts,
                              bool isQt5);

  /// @brief Parses the content of a qrc file
  ///
  /// Use when rcc does not support the "--list" option
  static void RccListParseContent(std::string const& content,
                                  std::vector<std::string>& files);

  /// @brief Parses the output of the "rcc --list ..." command
  static bool RccListParseOutput(std::string const& rccStdOut,
                                 std::string const& rccStdErr,
                                 std::vector<std::string>& files,
                                 std::string& error);

  /// @brief Converts relative qrc entry paths to full paths
  static void RccListConvertFullPath(std::string const& qrcFileDir,
                                     std::vector<std::string>& files);
};

#endif
