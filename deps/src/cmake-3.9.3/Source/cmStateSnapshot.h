/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */

#ifndef cmStateSnapshot_h
#define cmStateSnapshot_h

#include "cmConfigure.h" // IWYU pragma: keep

#include <string>
#include <vector>

#include "cmLinkedTree.h"
#include "cmPolicies.h"
#include "cmStateTypes.h"

class cmState;
class cmStateDirectory;

class cmStateSnapshot
{
public:
  cmStateSnapshot(cmState* state = CM_NULLPTR);
  cmStateSnapshot(cmState* state, cmStateDetail::PositionType position);

  const char* GetDefinition(std::string const& name) const;
  bool IsInitialized(std::string const& name) const;
  void SetDefinition(std::string const& name, std::string const& value);
  void RemoveDefinition(std::string const& name);
  std::vector<std::string> UnusedKeys() const;
  std::vector<std::string> ClosureKeys() const;
  bool RaiseScope(std::string const& var, const char* varDef);

  void SetListFile(std::string const& listfile);

  std::string GetExecutionListFile() const;

  std::vector<cmStateSnapshot> GetChildren();

  bool IsValid() const;
  cmStateSnapshot GetBuildsystemDirectoryParent() const;
  cmStateSnapshot GetCallStackParent() const;
  cmStateSnapshot GetCallStackBottom() const;
  cmStateEnums::SnapshotType GetType() const;

  void SetPolicy(cmPolicies::PolicyID id, cmPolicies::PolicyStatus status);
  cmPolicies::PolicyStatus GetPolicy(cmPolicies::PolicyID id) const;
  bool HasDefinedPolicyCMP0011();
  void PushPolicy(cmPolicies::PolicyMap const& entry, bool weak);
  bool PopPolicy();
  bool CanPopPolicyScope();

  cmState* GetState() const;

  cmStateDirectory GetDirectory() const;

  void SetProjectName(std::string const& name);
  std::string GetProjectName() const;

  void InitializeFromParent_ForSubdirsCommand();

  struct StrictWeakOrder
  {
    bool operator()(const cmStateSnapshot& lhs,
                    const cmStateSnapshot& rhs) const;
  };

  void SetDirectoryDefinitions();
  void SetDefaultDefinitions();

private:
  friend bool operator==(const cmStateSnapshot& lhs,
                         const cmStateSnapshot& rhs);
  friend bool operator!=(const cmStateSnapshot& lhs,
                         const cmStateSnapshot& rhs);
  friend class cmState;
  friend class cmStateDirectory;
  friend struct StrictWeakOrder;

  void InitializeFromParent();

  cmState* State;
  cmStateDetail::PositionType Position;
};

bool operator==(const cmStateSnapshot& lhs, const cmStateSnapshot& rhs);
bool operator!=(const cmStateSnapshot& lhs, const cmStateSnapshot& rhs);

#endif
