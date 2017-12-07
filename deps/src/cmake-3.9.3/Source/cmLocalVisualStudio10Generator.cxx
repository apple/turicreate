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
      this->GUID.assign(data + 1, length - 2);
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

void cmLocalVisualStudio10Generator::Generate()
{

  std::vector<cmGeneratorTarget*> tgts = this->GetGeneratorTargets();
  for (std::vector<cmGeneratorTarget*>::iterator l = tgts.begin();
       l != tgts.end(); ++l) {
    if ((*l)->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
      continue;
    }
    if (static_cast<cmGlobalVisualStudioGenerator*>(this->GlobalGenerator)
          ->TargetIsFortranOnly(*l)) {
      this->CreateSingleVCProj((*l)->GetName().c_str(), *l);
    } else {
      cmVisualStudio10TargetGenerator tg(
        *l, static_cast<cmGlobalVisualStudio10Generator*>(
              this->GetGlobalGenerator()));
      tg.Generate();
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
