/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmWIXPatchParser.h"

#include "cmCPackGenerator.h"

#include "cm_expat.h"

cmWIXPatchNode::Type cmWIXPatchText::type()
{
  return cmWIXPatchNode::TEXT;
}

cmWIXPatchNode::Type cmWIXPatchElement::type()
{
  return cmWIXPatchNode::ELEMENT;
}

cmWIXPatchNode::~cmWIXPatchNode()
{
}

cmWIXPatchElement::~cmWIXPatchElement()
{
  for (child_list_t::iterator i = children.begin(); i != children.end(); ++i) {
    delete *i;
  }
}

cmWIXPatchParser::cmWIXPatchParser(fragment_map_t& fragments,
                                   cmCPackLog* logger)
  : Logger(logger)
  , State(BEGIN_DOCUMENT)
  , Valid(true)
  , Fragments(fragments)
{
}

void cmWIXPatchParser::StartElement(const std::string& name, const char** atts)
{
  if (State == BEGIN_DOCUMENT) {
    if (name == "CPackWiXPatch") {
      State = BEGIN_FRAGMENTS;
    } else {
      ReportValidationError("Expected root element 'CPackWiXPatch'");
    }
  } else if (State == BEGIN_FRAGMENTS) {
    if (name == "CPackWiXFragment") {
      State = INSIDE_FRAGMENT;
      StartFragment(atts);
    } else {
      ReportValidationError("Expected 'CPackWixFragment' element");
    }
  } else if (State == INSIDE_FRAGMENT) {
    cmWIXPatchElement& parent = *ElementStack.back();

    cmWIXPatchElement* element = new cmWIXPatchElement;
    parent.children.push_back(element);

    element->name = name;

    for (size_t i = 0; atts[i]; i += 2) {
      std::string key = atts[i];
      std::string value = atts[i + 1];

      element->attributes[key] = value;
    }

    ElementStack.push_back(element);
  }
}

void cmWIXPatchParser::StartFragment(const char** attributes)
{
  cmWIXPatchElement* new_element = CM_NULLPTR;
  /* find the id of for fragment */
  for (size_t i = 0; attributes[i]; i += 2) {
    const std::string key = attributes[i];
    const std::string value = attributes[i + 1];

    if (key == "Id") {
      if (Fragments.find(value) != Fragments.end()) {
        std::ostringstream tmp;
        tmp << "Invalid reuse of 'CPackWixFragment' 'Id': " << value;
        ReportValidationError(tmp.str());
      }

      new_element = &Fragments[value];
      ElementStack.push_back(new_element);
    }
  }

  /* add any additional attributes for the fragement */
  if (!new_element) {
    ReportValidationError("No 'Id' specified for 'CPackWixFragment' element");
  } else {
    for (size_t i = 0; attributes[i]; i += 2) {
      const std::string key = attributes[i];
      const std::string value = attributes[i + 1];

      if (key != "Id") {
        new_element->attributes[key] = value;
      }
    }
  }
}

void cmWIXPatchParser::EndElement(const std::string& name)
{
  if (State == INSIDE_FRAGMENT) {
    if (name == "CPackWiXFragment") {
      State = BEGIN_FRAGMENTS;
      ElementStack.clear();
    } else {
      ElementStack.pop_back();
    }
  }
}

void cmWIXPatchParser::CharacterDataHandler(const char* data, int length)
{
  const char* whitespace = "\x20\x09\x0d\x0a";

  if (State == INSIDE_FRAGMENT) {
    cmWIXPatchElement& parent = *ElementStack.back();

    std::string text(data, length);

    std::string::size_type first = text.find_first_not_of(whitespace);
    std::string::size_type last = text.find_last_not_of(whitespace);

    if (first != std::string::npos && last != std::string::npos) {
      cmWIXPatchText* text_node = new cmWIXPatchText;
      text_node->text = text.substr(first, last - first + 1);

      parent.children.push_back(text_node);
    }
  }
}

void cmWIXPatchParser::ReportError(int line, int column, const char* msg)
{
  cmCPackLogger(cmCPackLog::LOG_ERROR,
                "Error while processing XML patch file at "
                  << line << ":" << column << ":  " << msg << std::endl);
  Valid = false;
}

void cmWIXPatchParser::ReportValidationError(std::string const& message)
{
  ReportError(
    XML_GetCurrentLineNumber(static_cast<XML_Parser>(this->Parser)),
    XML_GetCurrentColumnNumber(static_cast<XML_Parser>(this->Parser)),
    message.c_str());
}

bool cmWIXPatchParser::IsValid() const
{
  return Valid;
}
