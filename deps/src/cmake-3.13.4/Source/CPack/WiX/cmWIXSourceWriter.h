/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmWIXSourceWriter_h
#define cmWIXSourceWriter_h

#include "cmCPackLog.h"

#include "cmsys/FStream.hxx"

#include <string>
#include <vector>

/** \class cmWIXSourceWriter
 * \brief Helper class to generate XML WiX source files
 */
class cmWIXSourceWriter
{
public:
  enum GuidType
  {
    WIX_GENERATED_GUID,
    CMAKE_GENERATED_GUID
  };

  enum RootElementType
  {
    WIX_ELEMENT_ROOT,
    INCLUDE_ELEMENT_ROOT
  };

  cmWIXSourceWriter(cmCPackLog* logger, std::string const& filename,
                    GuidType componentGuidType,
                    RootElementType rootElementType = WIX_ELEMENT_ROOT);

  ~cmWIXSourceWriter();

  void BeginElement(std::string const& name);

  void EndElement(std::string const& name);

  void AddTextNode(std::string const& text);

  void AddProcessingInstruction(std::string const& target,
                                std::string const& content);

  void AddAttribute(std::string const& key, std::string const& value);

  void AddAttributeUnlessEmpty(std::string const& key,
                               std::string const& value);

  std::string CreateGuidFromComponentId(std::string const& componentId);

protected:
  cmCPackLog* Logger;

private:
  enum State
  {
    DEFAULT,
    BEGIN
  };

  void WriteXMLDeclaration();

  void Indent(size_t count);

  static std::string EscapeAttributeValue(std::string const& value);

  cmsys::ofstream File;

  State State;

  std::vector<std::string> Elements;

  std::string SourceFilename;

  GuidType ComponentGuidType;
};

#endif
