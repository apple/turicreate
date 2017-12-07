/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmGraphVizWriter.h"

#include "cmConfigure.h"
#include <iostream>
#include <sstream>
#include <utility>

#include "cmGeneratedFileStream.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmStateSnapshot.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cm_auto_ptr.hxx"
#include "cmake.h"

static const char* getShapeForTarget(const cmGeneratorTarget* target)
{
  if (!target) {
    return "ellipse";
  }

  switch (target->GetType()) {
    case cmStateEnums::EXECUTABLE:
      return "house";
    case cmStateEnums::STATIC_LIBRARY:
      return "diamond";
    case cmStateEnums::SHARED_LIBRARY:
      return "polygon";
    case cmStateEnums::MODULE_LIBRARY:
      return "octagon";
    default:
      break;
  }

  return "box";
}

cmGraphVizWriter::cmGraphVizWriter(
  const std::vector<cmLocalGenerator*>& localGenerators)
  : GraphType("digraph")
  , GraphName("GG")
  , GraphHeader("node [\n  fontsize = \"12\"\n];")
  , GraphNodePrefix("node")
  , LocalGenerators(localGenerators)
  , GenerateForExecutables(true)
  , GenerateForStaticLibs(true)
  , GenerateForSharedLibs(true)
  , GenerateForModuleLibs(true)
  , GenerateForExternals(true)
  , GeneratePerTarget(true)
  , GenerateDependers(true)
  , HaveTargetsAndLibs(false)
{
}

void cmGraphVizWriter::ReadSettings(const char* settingsFileName,
                                    const char* fallbackSettingsFileName)
{
  cmake cm(cmake::RoleScript);
  cm.SetHomeDirectory("");
  cm.SetHomeOutputDirectory("");
  cm.GetCurrentSnapshot().SetDefaultDefinitions();
  cmGlobalGenerator ggi(&cm);
  CM_AUTO_PTR<cmMakefile> mf(new cmMakefile(&ggi, cm.GetCurrentSnapshot()));
  CM_AUTO_PTR<cmLocalGenerator> lg(ggi.CreateLocalGenerator(mf.get()));

  const char* inFileName = settingsFileName;

  if (!cmSystemTools::FileExists(inFileName)) {
    inFileName = fallbackSettingsFileName;
    if (!cmSystemTools::FileExists(inFileName)) {
      return;
    }
  }

  if (!mf->ReadListFile(inFileName)) {
    cmSystemTools::Error("Problem opening GraphViz options file: ",
                         inFileName);
    return;
  }

  std::cout << "Reading GraphViz options file: " << inFileName << std::endl;

#define __set_if_set(var, cmakeDefinition)                                    \
  {                                                                           \
    const char* value = mf->GetDefinition(cmakeDefinition);                   \
    if (value) {                                                              \
      (var) = value;                                                          \
    }                                                                         \
  }

  __set_if_set(this->GraphType, "GRAPHVIZ_GRAPH_TYPE");
  __set_if_set(this->GraphName, "GRAPHVIZ_GRAPH_NAME");
  __set_if_set(this->GraphHeader, "GRAPHVIZ_GRAPH_HEADER");
  __set_if_set(this->GraphNodePrefix, "GRAPHVIZ_NODE_PREFIX");

#define __set_bool_if_set(var, cmakeDefinition)                               \
  {                                                                           \
    const char* value = mf->GetDefinition(cmakeDefinition);                   \
    if (value) {                                                              \
      (var) = mf->IsOn(cmakeDefinition);                                      \
    }                                                                         \
  }

  __set_bool_if_set(this->GenerateForExecutables, "GRAPHVIZ_EXECUTABLES");
  __set_bool_if_set(this->GenerateForStaticLibs, "GRAPHVIZ_STATIC_LIBS");
  __set_bool_if_set(this->GenerateForSharedLibs, "GRAPHVIZ_SHARED_LIBS");
  __set_bool_if_set(this->GenerateForModuleLibs, "GRAPHVIZ_MODULE_LIBS");
  __set_bool_if_set(this->GenerateForExternals, "GRAPHVIZ_EXTERNAL_LIBS");
  __set_bool_if_set(this->GeneratePerTarget, "GRAPHVIZ_GENERATE_PER_TARGET");
  __set_bool_if_set(this->GenerateDependers, "GRAPHVIZ_GENERATE_DEPENDERS");

  std::string ignoreTargetsRegexes;
  __set_if_set(ignoreTargetsRegexes, "GRAPHVIZ_IGNORE_TARGETS");

  this->TargetsToIgnoreRegex.clear();
  if (!ignoreTargetsRegexes.empty()) {
    std::vector<std::string> ignoreTargetsRegExVector;
    cmSystemTools::ExpandListArgument(ignoreTargetsRegexes,
                                      ignoreTargetsRegExVector);
    for (std::vector<std::string>::const_iterator itvIt =
           ignoreTargetsRegExVector.begin();
         itvIt != ignoreTargetsRegExVector.end(); ++itvIt) {
      std::string currentRegexString(*itvIt);
      cmsys::RegularExpression currentRegex;
      if (!currentRegex.compile(currentRegexString.c_str())) {
        std::cerr << "Could not compile bad regex \"" << currentRegexString
                  << "\"" << std::endl;
      }
      this->TargetsToIgnoreRegex.push_back(currentRegex);
    }
  }
}

// Iterate over all targets and write for each one a graph which shows
// which other targets depend on it.
void cmGraphVizWriter::WriteTargetDependersFiles(const char* fileName)
{
  if (!this->GenerateDependers) {
    return;
  }

  this->CollectTargetsAndLibs();

  for (std::map<std::string, const cmGeneratorTarget*>::const_iterator ptrIt =
         this->TargetPtrs.begin();
       ptrIt != this->TargetPtrs.end(); ++ptrIt) {
    if (ptrIt->second == CM_NULLPTR) {
      continue;
    }

    if (!this->GenerateForTargetType(ptrIt->second->GetType())) {
      continue;
    }

    std::string currentFilename = fileName;
    currentFilename += ".";
    currentFilename += ptrIt->first;
    currentFilename += ".dependers";

    cmGeneratedFileStream str(currentFilename.c_str());
    if (!str) {
      return;
    }

    std::set<std::string> insertedConnections;
    std::set<std::string> insertedNodes;

    std::cout << "Writing " << currentFilename << "..." << std::endl;
    this->WriteHeader(str);

    this->WriteDependerConnections(ptrIt->first, insertedNodes,
                                   insertedConnections, str);

    this->WriteFooter(str);
  }
}

// Iterate over all targets and write for each one a graph which shows
// on which targets it depends.
void cmGraphVizWriter::WritePerTargetFiles(const char* fileName)
{
  if (!this->GeneratePerTarget) {
    return;
  }

  this->CollectTargetsAndLibs();

  for (std::map<std::string, const cmGeneratorTarget*>::const_iterator ptrIt =
         this->TargetPtrs.begin();
       ptrIt != this->TargetPtrs.end(); ++ptrIt) {
    if (ptrIt->second == CM_NULLPTR) {
      continue;
    }

    if (!this->GenerateForTargetType(ptrIt->second->GetType())) {
      continue;
    }

    std::set<std::string> insertedConnections;
    std::set<std::string> insertedNodes;

    std::string currentFilename = fileName;
    currentFilename += ".";
    currentFilename += ptrIt->first;
    cmGeneratedFileStream str(currentFilename.c_str());
    if (!str) {
      return;
    }

    std::cout << "Writing " << currentFilename << "..." << std::endl;
    this->WriteHeader(str);

    this->WriteConnections(ptrIt->first, insertedNodes, insertedConnections,
                           str);
    this->WriteFooter(str);
  }
}

void cmGraphVizWriter::WriteGlobalFile(const char* fileName)
{
  this->CollectTargetsAndLibs();

  cmGeneratedFileStream str(fileName);
  if (!str) {
    return;
  }
  this->WriteHeader(str);

  std::cout << "Writing " << fileName << "..." << std::endl;

  std::set<std::string> insertedConnections;
  std::set<std::string> insertedNodes;

  for (std::map<std::string, const cmGeneratorTarget*>::const_iterator ptrIt =
         this->TargetPtrs.begin();
       ptrIt != this->TargetPtrs.end(); ++ptrIt) {
    if (ptrIt->second == CM_NULLPTR) {
      continue;
    }

    if (!this->GenerateForTargetType(ptrIt->second->GetType())) {
      continue;
    }

    this->WriteConnections(ptrIt->first, insertedNodes, insertedConnections,
                           str);
  }
  this->WriteFooter(str);
}

void cmGraphVizWriter::WriteHeader(cmGeneratedFileStream& str) const
{
  str << this->GraphType << " \"" << this->GraphName << "\" {" << std::endl;
  str << this->GraphHeader << std::endl;
}

void cmGraphVizWriter::WriteFooter(cmGeneratedFileStream& str) const
{
  str << "}" << std::endl;
}

void cmGraphVizWriter::WriteConnections(
  const std::string& targetName, std::set<std::string>& insertedNodes,
  std::set<std::string>& insertedConnections, cmGeneratedFileStream& str) const
{
  std::map<std::string, const cmGeneratorTarget*>::const_iterator targetPtrIt =
    this->TargetPtrs.find(targetName);

  if (targetPtrIt == this->TargetPtrs.end()) // not found at all
  {
    return;
  }

  this->WriteNode(targetName, targetPtrIt->second, insertedNodes, str);

  if (targetPtrIt->second == CM_NULLPTR) // it's an external library
  {
    return;
  }

  std::string myNodeName = this->TargetNamesNodes.find(targetName)->second;

  const cmTarget::LinkLibraryVectorType* ll =
    &(targetPtrIt->second->Target->GetOriginalLinkLibraries());

  for (cmTarget::LinkLibraryVectorType::const_iterator llit = ll->begin();
       llit != ll->end(); ++llit) {
    const char* libName = llit->first.c_str();
    std::map<std::string, std::string>::const_iterator libNameIt =
      this->TargetNamesNodes.find(libName);

    // can happen e.g. if GRAPHVIZ_TARGET_IGNORE_REGEX is used
    if (libNameIt == this->TargetNamesNodes.end()) {
      continue;
    }

    std::string connectionName = myNodeName;
    connectionName += "-";
    connectionName += libNameIt->second;
    if (insertedConnections.find(connectionName) ==
        insertedConnections.end()) {
      insertedConnections.insert(connectionName);
      this->WriteNode(libName, this->TargetPtrs.find(libName)->second,
                      insertedNodes, str);

      str << "    \"" << myNodeName << "\" -> \"" << libNameIt->second << "\"";
      str << " // " << targetName << " -> " << libName << std::endl;
      this->WriteConnections(libName, insertedNodes, insertedConnections, str);
    }
  }
}

void cmGraphVizWriter::WriteDependerConnections(
  const std::string& targetName, std::set<std::string>& insertedNodes,
  std::set<std::string>& insertedConnections, cmGeneratedFileStream& str) const
{
  std::map<std::string, const cmGeneratorTarget*>::const_iterator targetPtrIt =
    this->TargetPtrs.find(targetName);

  if (targetPtrIt == this->TargetPtrs.end()) // not found at all
  {
    return;
  }

  this->WriteNode(targetName, targetPtrIt->second, insertedNodes, str);

  if (targetPtrIt->second == CM_NULLPTR) // it's an external library
  {
    return;
  }

  std::string myNodeName = this->TargetNamesNodes.find(targetName)->second;

  // now search who links against me
  for (std::map<std::string, const cmGeneratorTarget*>::const_iterator
         dependerIt = this->TargetPtrs.begin();
       dependerIt != this->TargetPtrs.end(); ++dependerIt) {
    if (dependerIt->second == CM_NULLPTR) {
      continue;
    }

    if (!this->GenerateForTargetType(dependerIt->second->GetType())) {
      continue;
    }

    // Now we have a target, check whether it links against targetName.
    // If so, draw a connection, and then continue with dependers on that one.
    const cmTarget::LinkLibraryVectorType* ll =
      &(dependerIt->second->Target->GetOriginalLinkLibraries());

    for (cmTarget::LinkLibraryVectorType::const_iterator llit = ll->begin();
         llit != ll->end(); ++llit) {
      std::string libName = llit->first;
      if (libName == targetName) {
        // So this target links against targetName.
        std::map<std::string, std::string>::const_iterator dependerNodeNameIt =
          this->TargetNamesNodes.find(dependerIt->first);

        if (dependerNodeNameIt != this->TargetNamesNodes.end()) {
          std::string connectionName = dependerNodeNameIt->second;
          connectionName += "-";
          connectionName += myNodeName;

          if (insertedConnections.find(connectionName) ==
              insertedConnections.end()) {
            insertedConnections.insert(connectionName);
            this->WriteNode(dependerIt->first, dependerIt->second,
                            insertedNodes, str);

            str << "    \"" << dependerNodeNameIt->second << "\" -> \""
                << myNodeName << "\"";
            str << " // " << targetName << " -> " << dependerIt->first
                << std::endl;
            this->WriteDependerConnections(dependerIt->first, insertedNodes,
                                           insertedConnections, str);
          }
        }
        break;
      }
    }
  }
}

void cmGraphVizWriter::WriteNode(const std::string& targetName,
                                 const cmGeneratorTarget* target,
                                 std::set<std::string>& insertedNodes,
                                 cmGeneratedFileStream& str) const
{
  if (insertedNodes.find(targetName) == insertedNodes.end()) {
    insertedNodes.insert(targetName);
    std::map<std::string, std::string>::const_iterator nameIt =
      this->TargetNamesNodes.find(targetName);

    str << "    \"" << nameIt->second << "\" [ label=\"" << targetName
        << "\" shape=\"" << getShapeForTarget(target) << "\"];" << std::endl;
  }
}

void cmGraphVizWriter::CollectTargetsAndLibs()
{
  if (!this->HaveTargetsAndLibs) {
    this->HaveTargetsAndLibs = true;
    int cnt = this->CollectAllTargets();
    if (this->GenerateForExternals) {
      this->CollectAllExternalLibs(cnt);
    }
  }
}

int cmGraphVizWriter::CollectAllTargets()
{
  int cnt = 0;
  // First pass get the list of all cmake targets
  for (std::vector<cmLocalGenerator*>::const_iterator lit =
         this->LocalGenerators.begin();
       lit != this->LocalGenerators.end(); ++lit) {
    std::vector<cmGeneratorTarget*> targets = (*lit)->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::const_iterator it = targets.begin();
         it != targets.end(); ++it) {
      const char* realTargetName = (*it)->GetName().c_str();
      if (this->IgnoreThisTarget(realTargetName)) {
        // Skip ignored targets
        continue;
      }
      // std::cout << "Found target: " << tit->first << std::endl;
      std::ostringstream ostr;
      ostr << this->GraphNodePrefix << cnt++;
      this->TargetNamesNodes[realTargetName] = ostr.str();
      this->TargetPtrs[realTargetName] = *it;
    }
  }

  return cnt;
}

int cmGraphVizWriter::CollectAllExternalLibs(int cnt)
{
  // Ok, now find all the stuff we link to that is not in cmake
  for (std::vector<cmLocalGenerator*>::const_iterator lit =
         this->LocalGenerators.begin();
       lit != this->LocalGenerators.end(); ++lit) {
    std::vector<cmGeneratorTarget*> targets = (*lit)->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::const_iterator it = targets.begin();
         it != targets.end(); ++it) {
      const char* realTargetName = (*it)->GetName().c_str();
      if (this->IgnoreThisTarget(realTargetName)) {
        // Skip ignored targets
        continue;
      }
      const cmTarget::LinkLibraryVectorType* ll =
        &((*it)->Target->GetOriginalLinkLibraries());
      for (cmTarget::LinkLibraryVectorType::const_iterator llit = ll->begin();
           llit != ll->end(); ++llit) {
        const char* libName = llit->first.c_str();
        if (this->IgnoreThisTarget(libName)) {
          // Skip ignored targets
          continue;
        }

        std::map<std::string, const cmGeneratorTarget*>::const_iterator tarIt =
          this->TargetPtrs.find(libName);
        if (tarIt == this->TargetPtrs.end()) {
          std::ostringstream ostr;
          ostr << this->GraphNodePrefix << cnt++;
          this->TargetNamesNodes[libName] = ostr.str();
          this->TargetPtrs[libName] = CM_NULLPTR;
          // str << "    \"" << ostr << "\" [ label=\"" << libName
          // <<  "\" shape=\"ellipse\"];" << std::endl;
        }
      }
    }
  }
  return cnt;
}

bool cmGraphVizWriter::IgnoreThisTarget(const std::string& name)
{
  for (std::vector<cmsys::RegularExpression>::iterator itvIt =
         this->TargetsToIgnoreRegex.begin();
       itvIt != this->TargetsToIgnoreRegex.end(); ++itvIt) {
    cmsys::RegularExpression& regEx = *itvIt;
    if (regEx.is_valid()) {
      if (regEx.find(name)) {
        return true;
      }
    }
  }

  return false;
}

bool cmGraphVizWriter::GenerateForTargetType(
  cmStateEnums::TargetType targetType) const
{
  switch (targetType) {
    case cmStateEnums::EXECUTABLE:
      return this->GenerateForExecutables;
    case cmStateEnums::STATIC_LIBRARY:
      return this->GenerateForStaticLibs;
    case cmStateEnums::SHARED_LIBRARY:
      return this->GenerateForSharedLibs;
    case cmStateEnums::MODULE_LIBRARY:
      return this->GenerateForModuleLibs;
    default:
      break;
  }
  return false;
}
