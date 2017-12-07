/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmXMLWiter_h
#define cmXMLWiter_h

#include "cmConfigure.h"

#include "cmXMLSafe.h"

#include <ostream>
#include <stack>
#include <string>
#include <vector>

class cmXMLWriter
{
  CM_DISABLE_COPY(cmXMLWriter)

public:
  cmXMLWriter(std::ostream& output, std::size_t level = 0);
  ~cmXMLWriter();

  void StartDocument(const char* encoding = "UTF-8");
  void EndDocument();

  void StartElement(std::string const& name);
  void EndElement();

  void BreakAttributes();

  template <typename T>
  void Attribute(const char* name, T const& value)
  {
    this->PreAttribute();
    this->Output << name << "=\"" << SafeAttribute(value) << '"';
  }

  void Element(const char* name);

  template <typename T>
  void Element(std::string const& name, T const& value)
  {
    this->StartElement(name);
    this->Content(value);
    this->EndElement();
  }

  template <typename T>
  void Content(T const& content)
  {
    this->PreContent();
    this->Output << SafeContent(content);
  }

  void Comment(const char* comment);

  void CData(std::string const& data);

  void Doctype(const char* doctype);

  void ProcessingInstruction(const char* target, const char* data);

  void FragmentFile(const char* fname);

  void SetIndentationElement(std::string const& element);

private:
  void ConditionalLineBreak(bool condition, std::size_t indent);

  void PreAttribute();
  void PreContent();

  void CloseStartElement();

private:
  static cmXMLSafe SafeAttribute(const char* value)
  {
    return cmXMLSafe(value);
  }

  static cmXMLSafe SafeAttribute(std::string const& value)
  {
    return cmXMLSafe(value);
  }

  template <typename T>
  static T SafeAttribute(T value)
  {
    return value;
  }

  static cmXMLSafe SafeContent(const char* value)
  {
    return cmXMLSafe(value).Quotes(false);
  }

  static cmXMLSafe SafeContent(std::string const& value)
  {
    return cmXMLSafe(value).Quotes(false);
  }

  template <typename T>
  static T SafeContent(T value)
  {
    return value;
  }

private:
  std::ostream& Output;
  std::stack<std::string, std::vector<std::string> > Elements;
  std::string IndentationElement;
  std::size_t Level;
  bool ElementOpen;
  bool BreakAttrib;
  bool IsContent;
};

#endif
