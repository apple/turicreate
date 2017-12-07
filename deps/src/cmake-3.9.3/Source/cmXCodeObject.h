/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmXCodeObject_h
#define cmXCodeObject_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <algorithm>
#include <iosfwd>
#include <map>
#include <string>
#include <utility>
#include <vector>

class cmGeneratorTarget;

class cmXCodeObject
{
public:
  enum Type
  {
    OBJECT_LIST,
    STRING,
    ATTRIBUTE_GROUP,
    OBJECT_REF,
    OBJECT
  };
  enum PBXType
  {
    PBXGroup,
    PBXBuildStyle,
    PBXProject,
    PBXHeadersBuildPhase,
    PBXSourcesBuildPhase,
    PBXFrameworksBuildPhase,
    PBXNativeTarget,
    PBXFileReference,
    PBXBuildFile,
    PBXContainerItemProxy,
    PBXTargetDependency,
    PBXShellScriptBuildPhase,
    PBXResourcesBuildPhase,
    PBXApplicationReference,
    PBXExecutableFileReference,
    PBXLibraryReference,
    PBXToolTarget,
    PBXLibraryTarget,
    PBXAggregateTarget,
    XCBuildConfiguration,
    XCConfigurationList,
    PBXCopyFilesBuildPhase,
    None
  };
  class StringVec : public std::vector<std::string>
  {
  };
  static const char* PBXTypeNames[];
  virtual ~cmXCodeObject();
  cmXCodeObject(PBXType ptype, Type type);
  Type GetType() const { return this->TypeValue; }
  PBXType GetIsA() const { return this->IsA; }

  bool IsEmpty() const;

  void SetString(const std::string& s);
  const std::string& GetString() const { return this->String; }

  void AddAttribute(const std::string& name, cmXCodeObject* value)
  {
    this->ObjectAttributes[name] = value;
  }

  void AddAttributeIfNotEmpty(const std::string& name, cmXCodeObject* value)
  {
    if (value && !value->IsEmpty()) {
      AddAttribute(name, value);
    }
  }

  void SetObject(cmXCodeObject* value) { this->Object = value; }
  cmXCodeObject* GetObject() { return this->Object; }
  void AddObject(cmXCodeObject* value) { this->List.push_back(value); }
  bool HasObject(cmXCodeObject* o) const
  {
    return !(std::find(this->List.begin(), this->List.end(), o) ==
             this->List.end());
  }
  void AddUniqueObject(cmXCodeObject* value)
  {
    if (std::find(this->List.begin(), this->List.end(), value) ==
        this->List.end()) {
      this->List.push_back(value);
    }
  }
  static void Indent(int level, std::ostream& out);
  void Print(std::ostream& out);
  void PrintAttribute(std::ostream& out, const int level,
                      const std::string separator, const int factor,
                      const std::string& name, const cmXCodeObject* object,
                      const cmXCodeObject* parent);
  virtual void PrintComment(std::ostream&) {}

  static void PrintList(std::vector<cmXCodeObject*> const&, std::ostream& out);
  const std::string& GetId() const { return this->Id; }
  void SetId(const std::string& id) { this->Id = id; }
  cmGeneratorTarget* GetTarget() const { return this->Target; }
  void SetTarget(cmGeneratorTarget* t) { this->Target = t; }
  const std::string& GetComment() const { return this->Comment; }
  bool HasComment() const { return (!this->Comment.empty()); }
  cmXCodeObject* GetObject(const char* name) const
  {
    std::map<std::string, cmXCodeObject*>::const_iterator i =
      this->ObjectAttributes.find(name);
    if (i != this->ObjectAttributes.end()) {
      return i->second;
    }
    return 0;
  }
  // search the attribute list for an object of the specified type
  cmXCodeObject* GetObject(cmXCodeObject::PBXType t) const
  {
    for (std::vector<cmXCodeObject*>::const_iterator i = this->List.begin();
         i != this->List.end(); ++i) {
      cmXCodeObject* o = *i;
      if (o->IsA == t) {
        return o;
      }
    }
    return 0;
  }

  void CopyAttributes(cmXCodeObject*);

  void AddDependLibrary(const std::string& configName, const std::string& l)
  {
    this->DependLibraries[configName].push_back(l);
  }
  std::map<std::string, StringVec> const& GetDependLibraries() const
  {
    return this->DependLibraries;
  }
  void AddDependTarget(const std::string& configName, const std::string& tName)
  {
    this->DependTargets[configName].push_back(tName);
  }
  std::map<std::string, StringVec> const& GetDependTargets() const
  {
    return this->DependTargets;
  }
  std::vector<cmXCodeObject*> const& GetObjectList() const
  {
    return this->List;
  }
  void SetComment(const std::string& c) { this->Comment = c; }
  static void PrintString(std::ostream& os, std::string String);

protected:
  void PrintString(std::ostream& os) const;

  cmGeneratorTarget* Target;
  Type TypeValue;
  std::string Id;
  PBXType IsA;
  int Version;
  std::string Comment;
  std::string String;
  cmXCodeObject* Object;
  std::vector<cmXCodeObject*> List;
  std::map<std::string, StringVec> DependLibraries;
  std::map<std::string, StringVec> DependTargets;
  std::map<std::string, cmXCodeObject*> ObjectAttributes;
};
#endif
