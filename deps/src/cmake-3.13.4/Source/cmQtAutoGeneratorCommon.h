/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmQtAutoGeneratorCommon_h
#define cmQtAutoGeneratorCommon_h

#include "cmConfigure.h"

#include <string>
#include <vector>

class cmQtAutoGeneratorCommon
{
  // - Types and statics
public:
  static const char* listSep;

  enum GeneratorType
  {
    MOC,
    UIC,
    RCC
  };

public:
  /// @brief Returns a the string escaped and enclosed in quotes
  ///
  static std::string Quoted(const std::string& text);

  /// @brief Reads the resource files list from from a .qrc file
  /// @arg fileName Must be the absolute path of the .qrc file
  /// @return True if the rcc file was successfully parsed
  static bool RccListInputs(const std::string& qtMajorVersion,
                            const std::string& rccCommand,
                            const std::string& fileName,
                            std::vector<std::string>& files,
                            std::string* errorMessage = CM_NULLPTR);
};

#endif
