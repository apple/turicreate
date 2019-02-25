/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmSourceGroup.h"

class cmSourceGroupInternals
{
public:
  std::vector<cmSourceGroup> GroupChildren;
};

cmSourceGroup::cmSourceGroup(const std::string& name, const char* regex,
                             const char* parentName)
  : Name(name)
{
  this->Internal = new cmSourceGroupInternals;
  this->SetGroupRegex(regex);
  if (parentName) {
    this->FullName = parentName;
    this->FullName += "\\";
  }
  this->FullName += this->Name;
}

cmSourceGroup::~cmSourceGroup()
{
  delete this->Internal;
}

cmSourceGroup::cmSourceGroup(cmSourceGroup const& r)
{
  this->Name = r.Name;
  this->FullName = r.FullName;
  this->GroupRegex = r.GroupRegex;
  this->GroupFiles = r.GroupFiles;
  this->SourceFiles = r.SourceFiles;
  this->Internal = new cmSourceGroupInternals(*r.Internal);
}

cmSourceGroup& cmSourceGroup::operator=(cmSourceGroup const& r)
{
  this->Name = r.Name;
  this->GroupRegex = r.GroupRegex;
  this->GroupFiles = r.GroupFiles;
  this->SourceFiles = r.SourceFiles;
  *(this->Internal) = *(r.Internal);
  return *this;
}

void cmSourceGroup::SetGroupRegex(const char* regex)
{
  if (regex) {
    this->GroupRegex.compile(regex);
  } else {
    this->GroupRegex.compile("^$");
  }
}

void cmSourceGroup::AddGroupFile(const std::string& name)
{
  this->GroupFiles.insert(name);
}

std::string const& cmSourceGroup::GetName() const
{
  return this->Name;
}

std::string const& cmSourceGroup::GetFullName() const
{
  return this->FullName;
}

bool cmSourceGroup::MatchesRegex(const std::string& name)
{
  return this->GroupRegex.find(name);
}

bool cmSourceGroup::MatchesFiles(const std::string& name) const
{
  return this->GroupFiles.find(name) != this->GroupFiles.cend();
}

void cmSourceGroup::AssignSource(const cmSourceFile* sf)
{
  this->SourceFiles.push_back(sf);
}

const std::vector<const cmSourceFile*>& cmSourceGroup::GetSourceFiles() const
{
  return this->SourceFiles;
}

void cmSourceGroup::AddChild(cmSourceGroup const& child)
{
  this->Internal->GroupChildren.push_back(child);
}

cmSourceGroup* cmSourceGroup::LookupChild(const std::string& name)
{
  for (cmSourceGroup& group : this->Internal->GroupChildren) {
    // look if descenened is the one were looking for
    if (group.GetName() == name) {
      return (&group); // if it so return it
    }
  }

  // if no child with this name was found return NULL
  return nullptr;
}

cmSourceGroup* cmSourceGroup::MatchChildrenFiles(const std::string& name)
{
  if (this->MatchesFiles(name)) {
    return this;
  }
  for (cmSourceGroup& group : this->Internal->GroupChildren) {
    cmSourceGroup* result = group.MatchChildrenFiles(name);
    if (result) {
      return result;
    }
  }
  return nullptr;
}

cmSourceGroup* cmSourceGroup::MatchChildrenRegex(const std::string& name)
{
  for (cmSourceGroup& group : this->Internal->GroupChildren) {
    cmSourceGroup* result = group.MatchChildrenRegex(name);
    if (result) {
      return result;
    }
  }
  if (this->MatchesRegex(name)) {
    return this;
  }

  return nullptr;
}

std::vector<cmSourceGroup> const& cmSourceGroup::GetGroupChildren() const
{
  return this->Internal->GroupChildren;
}
