/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmLocalVisualStudio10Generator.h"

#include "cmGeneratorTarget.h"
#include "cmGlobalVisualStudio10Generator.h"
#include "cmMakefile.h"
#include "cmVisualStudio10TargetGenerator.h"
#include "cmXMLParser.h"

#include "cm_expat.h"

class cmVS10XMLParser : public cmXMLParser
{
public:
  virtual void EndElement(const std::string& /* name */) {}
  virtual void CharacterDataHandler(const char* data, int length)
  {
    if (this->DoGUID) {
      if (data[0] == '{') {
        // remove surrounding curly brackets
        this->GUID.assign(data + 1, length - 2);
      } else {
        this->GUID.assign(data, length);
      }
      this->DoGUID = false;
    }
  }
  virtual void StartElement(const std::string& name, const char**)
  {
    // once the GUID is found do nothing
    if (!this->GUID.empty()) {
      return;
    }
    if ("ProjectGUID" == name || "ProjectGuid" == name) {
      this->DoGUID = true;
    }
  }
  int InitializeParser()
  {
    this->DoGUID = false;
    int ret = cmXMLParser::InitializeParser();
    if (ret == 0) {
      return ret;
    }
    // visual studio projects have a strange encoding, but it is
    // really utf-8
    XML_SetEncoding(static_cast<XML_Parser>(this->Parser), "utf-8");
    return 1;
  }
  std::string GUID;
  bool DoGUID;
};

cmLocalVisualStudio10Generator::cmLocalVisualStudio10Generator(
  cmGlobalGenerator* gg, cmMakefile* mf)
  : cmLocalVisualStudio7Generator(gg, mf)
{
}

cmLocalVisualStudio10Generator::~cmLocalVisualStudio10Generator()
{
}

void cmLocalVisualStudio10Generator::GenerateTargetsDepthFirst(
  cmGeneratorTarget* target, std::vector<cmGeneratorTarget*>& remaining)
{
  if (target->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    return;
  }
  // Find this target in the list of remaining targets.
  auto it = std::find(remaining.begin(), remaining.end(), target);
  if (it == remaining.end()) {
    // This target was already handled.
    return;
  }
  // Remove this target from the list of remaining targets because
  // we are handling it now.
  *it = nullptr;
  auto& deps = this->GlobalGenerator->GetTargetDirectDepends(target);
  for (auto& d : deps) {
    // FIXME: Revise CreateSingleVCProj so we do not have to drop `const` here.
    auto dependee = const_cast<cmGeneratorTarget*>(&*d);
    GenerateTargetsDepthFirst(dependee, remaining);
    // Take the union of visited source files of custom commands
    auto visited = GetSourcesVisited(dependee);
    GetSourcesVisited(target).insert(visited.begin(), visited.end());
  }
  if (static_cast<cmGlobalVisualStudioGenerator*>(this->GlobalGenerator)
        ->TargetIsFortranOnly(target)) {
    this->CreateSingleVCProj(target->GetName(), target);
  } else {
    cmVisualStudio10TargetGenerator tg(
      target,
      static_cast<cmGlobalVisualStudio10Generator*>(
        this->GetGlobalGenerator()));
    tg.Generate();
  }
}

void cmLocalVisualStudio10Generator::Generate()
{
  std::vector<cmGeneratorTarget*> remaining = this->GetGeneratorTargets();
  for (auto& t : remaining) {
    if (t) {
      GenerateTargetsDepthFirst(t, remaining);
    }
  }
  this->WriteStampFiles();
}

void cmLocalVisualStudio10Generator::ReadAndStoreExternalGUID(
  const std::string& name, const char* path)
{
  cmVS10XMLParser parser;
  parser.ParseFile(path);

  // if we can not find a GUID then we will generate one later
  if (parser.GUID.empty()) {
    return;
  }

  std::string guidStoreName = name;
  guidStoreName += "_GUID_CMAKE";
  // save the GUID in the cache
  this->GlobalGenerator->GetCMakeInstance()->AddCacheEntry(
    guidStoreName.c_str(), parser.GUID.c_str(), "Stored GUID",
    cmStateEnums::INTERNAL);
}

const char* cmLocalVisualStudio10Generator::ReportErrorLabel() const
{
  return ":VCEnd";
}
