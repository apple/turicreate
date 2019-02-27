/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef CMGRAPHVIZWRITER_H
#define CMGRAPHVIZWRITER_H

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmStateTypes.h"

#include "cmsys/RegularExpression.hxx"
#include <map>
#include <set>
#include <string>
#include <vector>

class cmGeneratedFileStream;
class cmGeneratorTarget;
class cmLocalGenerator;

/** This class implements writing files for graphviz (dot) for graphs
 * representing the dependencies between the targets in the project. */
class cmGraphVizWriter
{
public:
  cmGraphVizWriter(const std::vector<cmLocalGenerator*>& localGenerators);

  void ReadSettings(const char* settingsFileName,
                    const char* fallbackSettingsFileName);

  void WritePerTargetFiles(const char* fileName);
  void WriteTargetDependersFiles(const char* fileName);

  void WriteGlobalFile(const char* fileName);

protected:
  void CollectTargetsAndLibs();

  int CollectAllTargets();

  int CollectAllExternalLibs(int cnt);

  void WriteHeader(cmGeneratedFileStream& str) const;

  void WriteConnections(const std::string& targetName,
                        std::set<std::string>& insertedNodes,
                        std::set<std::string>& insertedConnections,
                        cmGeneratedFileStream& str) const;

  void WriteDependerConnections(const std::string& targetName,
                                std::set<std::string>& insertedNodes,
                                std::set<std::string>& insertedConnections,
                                cmGeneratedFileStream& str) const;

  void WriteNode(const std::string& targetName,
                 const cmGeneratorTarget* target,
                 std::set<std::string>& insertedNodes,
                 cmGeneratedFileStream& str) const;

  void WriteFooter(cmGeneratedFileStream& str) const;

  bool IgnoreThisTarget(const std::string& name);

  bool GenerateForTargetType(cmStateEnums::TargetType targetType) const;

  std::string GraphType;
  std::string GraphName;
  std::string GraphHeader;
  std::string GraphNodePrefix;

  std::vector<cmsys::RegularExpression> TargetsToIgnoreRegex;

  const std::vector<cmLocalGenerator*>& LocalGenerators;

  std::map<std::string, const cmGeneratorTarget*> TargetPtrs;
  // maps from the actual target names to node names in dot:
  std::map<std::string, std::string> TargetNamesNodes;

  bool GenerateForExecutables;
  bool GenerateForStaticLibs;
  bool GenerateForSharedLibs;
  bool GenerateForModuleLibs;
  bool GenerateForInterface;
  bool GenerateForExternals;
  bool GeneratePerTarget;
  bool GenerateDependers;
  bool HaveTargetsAndLibs;
};

#endif
