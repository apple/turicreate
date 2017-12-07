/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmXCodeObject.h"

#include <CoreFoundation/CoreFoundation.h>
#include <ostream>

#include "cmSystemTools.h"

const char* cmXCodeObject::PBXTypeNames[] = {
  /* clang-format needs this comment to break after the opening brace */
  "PBXGroup",
  "PBXBuildStyle",
  "PBXProject",
  "PBXHeadersBuildPhase",
  "PBXSourcesBuildPhase",
  "PBXFrameworksBuildPhase",
  "PBXNativeTarget",
  "PBXFileReference",
  "PBXBuildFile",
  "PBXContainerItemProxy",
  "PBXTargetDependency",
  "PBXShellScriptBuildPhase",
  "PBXResourcesBuildPhase",
  "PBXApplicationReference",
  "PBXExecutableFileReference",
  "PBXLibraryReference",
  "PBXToolTarget",
  "PBXLibraryTarget",
  "PBXAggregateTarget",
  "XCBuildConfiguration",
  "XCConfigurationList",
  "PBXCopyFilesBuildPhase",
  "None"
};

cmXCodeObject::~cmXCodeObject()
{
  this->Version = 15;
}

cmXCodeObject::cmXCodeObject(PBXType ptype, Type type)
{
  this->Version = 15;
  this->Target = 0;
  this->Object = 0;

  this->IsA = ptype;

  if (type == OBJECT) {
    // Set the Id of an Xcode object to a unique string for each instance.
    // However the Xcode user file references certain Ids: for those cases,
    // override the generated Id using SetId().
    //
    char cUuid[40] = { 0 };
    CFUUIDRef uuid = CFUUIDCreate(kCFAllocatorDefault);
    CFStringRef s = CFUUIDCreateString(kCFAllocatorDefault, uuid);
    CFStringGetCString(s, cUuid, sizeof(cUuid), kCFStringEncodingUTF8);
    this->Id = cUuid;
    CFRelease(s);
    CFRelease(uuid);
  } else {
    this->Id =
      "Temporary cmake object, should not be referred to in Xcode file";
  }

  cmSystemTools::ReplaceString(this->Id, "-", "");
  if (this->Id.size() > 24) {
    this->Id = this->Id.substr(0, 24);
  }

  this->TypeValue = type;
  if (this->TypeValue == OBJECT) {
    this->AddAttribute("isa", 0);
  }
}

bool cmXCodeObject::IsEmpty() const
{
  switch (this->TypeValue) {
    case OBJECT_LIST:
      return this->List.empty();
    case STRING:
      return this->String.empty();
    case ATTRIBUTE_GROUP:
      return this->ObjectAttributes.empty();
    case OBJECT_REF:
    case OBJECT:
      return this->Object == 0;
  }
  return true; // unreachable, but quiets warnings
}

void cmXCodeObject::Indent(int level, std::ostream& out)
{
  while (level) {
    out << "\t";
    level--;
  }
}

void cmXCodeObject::Print(std::ostream& out)
{
  std::string separator = "\n";
  int indentFactor = 1;
  cmXCodeObject::Indent(2 * indentFactor, out);
  if (this->Version > 15 &&
      (this->IsA == PBXFileReference || this->IsA == PBXBuildFile)) {
    separator = " ";
    indentFactor = 0;
  }
  out << this->Id;
  this->PrintComment(out);
  out << " = {";
  if (separator == "\n") {
    out << separator;
  }
  std::map<std::string, cmXCodeObject*>::iterator i;
  cmXCodeObject::Indent(3 * indentFactor, out);
  out << "isa = " << PBXTypeNames[this->IsA] << ";" << separator;
  for (i = this->ObjectAttributes.begin(); i != this->ObjectAttributes.end();
       ++i) {
    if (i->first == "isa")
      continue;

    PrintAttribute(out, 3, separator, indentFactor, i->first, i->second, this);
  }
  cmXCodeObject::Indent(2 * indentFactor, out);
  out << "};\n";
}

void cmXCodeObject::PrintAttribute(std::ostream& out, const int level,
                                   const std::string separator,
                                   const int factor, const std::string& name,
                                   const cmXCodeObject* object,
                                   const cmXCodeObject* parent)
{
  cmXCodeObject::Indent(level * factor, out);
  switch (object->TypeValue) {
    case OBJECT_LIST: {
      out << name << " = (";
      if (parent->TypeValue != ATTRIBUTE_GROUP) {
        out << separator;
      }
      for (unsigned int i = 0; i < object->List.size(); ++i) {
        if (object->List[i]->TypeValue == STRING) {
          object->List[i]->PrintString(out);
          if (i + 1 < object->List.size()) {
            out << ",";
          }
        } else {
          cmXCodeObject::Indent((level + 1) * factor, out);
          out << object->List[i]->Id;
          object->List[i]->PrintComment(out);
          out << "," << separator;
        }
      }
      if (parent->TypeValue != ATTRIBUTE_GROUP) {
        cmXCodeObject::Indent(level * factor, out);
      }
      out << ");" << separator;
    } break;

    case ATTRIBUTE_GROUP: {
      out << name << " = {";
      if (separator == "\n") {
        out << separator;
      }
      std::map<std::string, cmXCodeObject*>::const_iterator i;
      for (i = object->ObjectAttributes.begin();
           i != object->ObjectAttributes.end(); ++i) {
        PrintAttribute(out, (level + 1) * factor, separator, factor, i->first,
                       i->second, object);
      }
      cmXCodeObject::Indent(level * factor, out);
      out << "};" << separator;
    } break;

    case OBJECT_REF: {
      cmXCodeObject::PrintString(out, name);
      out << " = " << object->Object->Id;
      if (object->Object->HasComment() && name != "remoteGlobalIDString") {
        object->Object->PrintComment(out);
      }
      out << ";" << separator;
    } break;

    case STRING: {
      cmXCodeObject::PrintString(out, name);
      out << " = ";
      object->PrintString(out);
      out << ";" << separator;
    } break;

    default: {
      break;
    }
  }
}

void cmXCodeObject::PrintList(std::vector<cmXCodeObject*> const& objs,
                              std::ostream& out)
{
  cmXCodeObject::Indent(1, out);
  out << "objects = {\n";
  for (unsigned int i = 0; i < objs.size(); ++i) {
    if (objs[i]->TypeValue == OBJECT) {
      objs[i]->Print(out);
    }
  }
  cmXCodeObject::Indent(1, out);
  out << "};\n";
}

void cmXCodeObject::CopyAttributes(cmXCodeObject* copy)
{
  this->ObjectAttributes = copy->ObjectAttributes;
  this->List = copy->List;
  this->String = copy->String;
  this->Object = copy->Object;
}

void cmXCodeObject::PrintString(std::ostream& os, std::string String)
{
  // The string needs to be quoted if it contains any characters
  // considered special by the Xcode project file parser.
  bool needQuote = (String.empty() || String.find("//") != std::string::npos ||
                    String.find_first_not_of("ABCDEFGHIJKLMNOPQRSTUVWXYZ"
                                             "abcdefghijklmnopqrstuvwxyz"
                                             "0123456789"
                                             "$_./") != std::string::npos);
  const char* quote = needQuote ? "\"" : "";

  // Print the string, quoted and escaped as necessary.
  os << quote;
  for (std::string::const_iterator i = String.begin(); i != String.end();
       ++i) {
    if (*i == '"' || *i == '\\') {
      // Escape double-quotes and backslashes.
      os << '\\';
    }
    os << *i;
  }
  os << quote;
}

void cmXCodeObject::PrintString(std::ostream& os) const
{
  cmXCodeObject::PrintString(os, this->String);
}

void cmXCodeObject::SetString(const std::string& s)
{
  this->String = s;
}
