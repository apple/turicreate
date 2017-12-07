/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmXMLParser.h"

#include "cm_expat.h"
#include "cmsys/FStream.hxx"
#include <ctype.h>
#include <iostream>
#include <sstream>
#include <string.h>

cmXMLParser::cmXMLParser()
{
  this->Parser = CM_NULLPTR;
  this->ParseError = 0;
  this->ReportCallback = CM_NULLPTR;
  this->ReportCallbackData = CM_NULLPTR;
}

cmXMLParser::~cmXMLParser()
{
  if (this->Parser) {
    this->CleanupParser();
  }
}

int cmXMLParser::Parse(const char* string)
{
  return (int)this->InitializeParser() &&
    this->ParseChunk(string, strlen(string)) && this->CleanupParser();
}

int cmXMLParser::ParseFile(const char* file)
{
  if (!file) {
    return 0;
  }

  cmsys::ifstream ifs(file);
  if (!ifs) {
    return 0;
  }

  std::ostringstream str;
  str << ifs.rdbuf();
  return this->Parse(str.str().c_str());
}

int cmXMLParser::InitializeParser()
{
  if (this->Parser) {
    std::cerr << "Parser already initialized" << std::endl;
    this->ParseError = 1;
    return 0;
  }

  // Create the expat XML parser.
  this->Parser = XML_ParserCreate(CM_NULLPTR);
  XML_SetElementHandler(static_cast<XML_Parser>(this->Parser),
                        &cmXMLParserStartElement, &cmXMLParserEndElement);
  XML_SetCharacterDataHandler(static_cast<XML_Parser>(this->Parser),
                              &cmXMLParserCharacterDataHandler);
  XML_SetUserData(static_cast<XML_Parser>(this->Parser), this);
  this->ParseError = 0;
  return 1;
}

int cmXMLParser::ParseChunk(const char* inputString,
                            std::string::size_type length)
{
  if (!this->Parser) {
    std::cerr << "Parser not initialized" << std::endl;
    this->ParseError = 1;
    return 0;
  }
  int res;
  res = this->ParseBuffer(inputString, length);
  if (res == 0) {
    this->ParseError = 1;
  }
  return res;
}

int cmXMLParser::CleanupParser()
{
  if (!this->Parser) {
    std::cerr << "Parser not initialized" << std::endl;
    this->ParseError = 1;
    return 0;
  }
  int result = !this->ParseError;
  if (result) {
    // Tell the expat XML parser about the end-of-input.
    if (!XML_Parse(static_cast<XML_Parser>(this->Parser), "", 0, 1)) {
      this->ReportXmlParseError();
      result = 0;
    }
  }

  // Clean up the parser.
  XML_ParserFree(static_cast<XML_Parser>(this->Parser));
  this->Parser = CM_NULLPTR;

  return result;
}

int cmXMLParser::ParseBuffer(const char* buffer, std::string::size_type count)
{
  // Pass the buffer to the expat XML parser.
  if (!XML_Parse(static_cast<XML_Parser>(this->Parser), buffer,
                 static_cast<int>(count), 0)) {
    this->ReportXmlParseError();
    return 0;
  }
  return 1;
}

int cmXMLParser::ParseBuffer(const char* buffer)
{
  return this->ParseBuffer(buffer, static_cast<int>(strlen(buffer)));
}

int cmXMLParser::ParsingComplete()
{
  // Default behavior is to parse to end of stream.
  return 0;
}

void cmXMLParser::StartElement(const std::string& name, const char** /*atts*/)
{
  std::cout << "Start element: " << name << std::endl;
}

void cmXMLParser::EndElement(const std::string& name)
{
  std::cout << "End element: " << name << std::endl;
}

void cmXMLParser::CharacterDataHandler(const char* /*inData*/,
                                       int /*inLength*/)
{
}

int cmXMLParser::IsSpace(char c)
{
  return isspace(c);
}

const char* cmXMLParser::FindAttribute(const char** atts,
                                       const char* attribute)
{
  if (atts && attribute) {
    for (const char** a = atts; *a && *(a + 1); a += 2) {
      if (strcmp(*a, attribute) == 0) {
        return *(a + 1);
      }
    }
  }
  return CM_NULLPTR;
}

void cmXMLParserStartElement(void* parser, const char* name, const char** atts)
{
  // Begin element handler that is registered with the XML_Parser.
  // This just casts the user data to a cmXMLParser and calls
  // StartElement.
  static_cast<cmXMLParser*>(parser)->StartElement(name, atts);
}

void cmXMLParserEndElement(void* parser, const char* name)
{
  // End element handler that is registered with the XML_Parser.  This
  // just casts the user data to a cmXMLParser and calls EndElement.
  static_cast<cmXMLParser*>(parser)->EndElement(name);
}

void cmXMLParserCharacterDataHandler(void* parser, const char* data,
                                     int length)
{
  // Character data handler that is registered with the XML_Parser.
  // This just casts the user data to a cmXMLParser and calls
  // CharacterDataHandler.
  static_cast<cmXMLParser*>(parser)->CharacterDataHandler(data, length);
}

void cmXMLParser::ReportXmlParseError()
{
  XML_Parser parser = static_cast<XML_Parser>(this->Parser);
  this->ReportError(XML_GetCurrentLineNumber(parser),
                    XML_GetCurrentColumnNumber(parser),
                    XML_ErrorString(XML_GetErrorCode(parser)));
}

void cmXMLParser::ReportError(int line, int /*unused*/, const char* msg)
{
  if (this->ReportCallback) {
    this->ReportCallback(line, msg, this->ReportCallbackData);
  } else {
    std::cerr << "Error parsing XML in stream at line " << line << ": " << msg
              << std::endl;
  }
}
