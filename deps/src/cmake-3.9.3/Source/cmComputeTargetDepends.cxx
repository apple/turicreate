/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmComputeTargetDepends.h"

#include "cmComputeComponentGraph.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLinkItem.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmPolicies.h"
#include "cmSourceFile.h"
#include "cmState.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmTargetDepend.h"
#include "cmake.h"

#include <assert.h>
#include <sstream>
#include <stdio.h>
#include <utility>

class cmListFileBacktrace;

/*

This class is meant to analyze inter-target dependencies globally
during the generation step.  The goal is to produce a set of direct
dependencies for each target such that no cycles are left and the
build order is safe.

For most target types cyclic dependencies are not allowed.  However
STATIC libraries may depend on each other in a cyclic fashion.  In
general the directed dependency graph forms a directed-acyclic-graph
of strongly connected components.  All strongly connected components
should consist of only STATIC_LIBRARY targets.

In order to safely break dependency cycles we must preserve all other
dependencies passing through the corresponding strongly connected component.
The approach taken by this class is as follows:

  - Collect all targets and form the original dependency graph
  - Run Tarjan's algorithm to extract the strongly connected components
    (error if any member of a non-trivial component is not STATIC)
  - The original dependencies imply a DAG on the components.
    Use the implied DAG to construct a final safe set of dependencies.

The final dependency set is constructed as follows:

  - For each connected component targets are placed in an arbitrary
    order.  Each target depends on the target following it in the order.
    The first target is designated the head and the last target the tail.
    (most components will be just 1 target anyway)

  - Original dependencies between targets in different components are
    converted to connect the depender's component tail to the
    dependee's component head.

In most cases this will reproduce the original dependencies.  However
when there are cycles of static libraries they will be broken in a
safe manner.

For example, consider targets A0, A1, A2, B0, B1, B2, and C with these
dependencies:

  A0 -> A1 -> A2 -> A0  ,  B0 -> B1 -> B2 -> B0 -> A0  ,  C -> B0

Components may be identified as

  Component 0: A0, A1, A2
  Component 1: B0, B1, B2
  Component 2: C

Intra-component dependencies are:

  0: A0 -> A1 -> A2   , head=A0, tail=A2
  1: B0 -> B1 -> B2   , head=B0, tail=B2
  2: head=C, tail=C

The inter-component dependencies are converted as:

  B0 -> A0  is component 1->0 and becomes  B2 -> A0
  C  -> B0  is component 2->1 and becomes  C  -> B0

This leads to the final target dependencies:

  C -> B0 -> B1 -> B2 -> A0 -> A1 -> A2

These produce a safe build order since C depends directly or
transitively on all the static libraries it links.

*/

cmComputeTargetDepends::cmComputeTargetDepends(cmGlobalGenerator* gg)
{
  this->GlobalGenerator = gg;
  cmake* cm = this->GlobalGenerator->GetCMakeInstance();
  this->DebugMode =
    cm->GetState()->GetGlobalPropertyAsBool("GLOBAL_DEPENDS_DEBUG_MODE");
  this->NoCycles =
    cm->GetState()->GetGlobalPropertyAsBool("GLOBAL_DEPENDS_NO_CYCLES");
}

cmComputeTargetDepends::~cmComputeTargetDepends()
{
}

bool cmComputeTargetDepends::Compute()
{
  // Build the original graph.
  this->CollectTargets();
  this->CollectDepends();
  if (this->DebugMode) {
    this->DisplayGraph(this->InitialGraph, "initial");
  }

  // Identify components.
  cmComputeComponentGraph ccg(this->InitialGraph);
  if (this->DebugMode) {
    this->DisplayComponents(ccg);
  }
  if (!this->CheckComponents(ccg)) {
    return false;
  }

  // Compute the final dependency graph.
  if (!this->ComputeFinalDepends(ccg)) {
    return false;
  }
  if (this->DebugMode) {
    this->DisplayGraph(this->FinalGraph, "final");
  }

  return true;
}

void cmComputeTargetDepends::GetTargetDirectDepends(cmGeneratorTarget const* t,
                                                    cmTargetDependSet& deps)
{
  // Lookup the index for this target.  All targets should be known by
  // this point.
  std::map<cmGeneratorTarget const*, int>::const_iterator tii =
    this->TargetIndex.find(t);
  assert(tii != this->TargetIndex.end());
  int i = tii->second;

  // Get its final dependencies.
  EdgeList const& nl = this->FinalGraph[i];
  for (EdgeList::const_iterator ni = nl.begin(); ni != nl.end(); ++ni) {
    cmGeneratorTarget const* dep = this->Targets[*ni];
    cmTargetDependSet::iterator di = deps.insert(dep).first;
    di->SetType(ni->IsStrong());
  }
}

void cmComputeTargetDepends::CollectTargets()
{
  // Collect all targets from all generators.
  std::vector<cmLocalGenerator*> const& lgens =
    this->GlobalGenerator->GetLocalGenerators();
  for (unsigned int i = 0; i < lgens.size(); ++i) {
    const std::vector<cmGeneratorTarget*> targets =
      lgens[i]->GetGeneratorTargets();
    for (std::vector<cmGeneratorTarget*>::const_iterator ti = targets.begin();
         ti != targets.end(); ++ti) {
      cmGeneratorTarget* gt = *ti;
      int index = static_cast<int>(this->Targets.size());
      this->TargetIndex[gt] = index;
      this->Targets.push_back(gt);
    }
  }
}

void cmComputeTargetDepends::CollectDepends()
{
  // Allocate the dependency graph adjacency lists.
  this->InitialGraph.resize(this->Targets.size());

  // Compute each dependency list.
  for (unsigned int i = 0; i < this->Targets.size(); ++i) {
    this->CollectTargetDepends(i);
  }
}

void cmComputeTargetDepends::CollectTargetDepends(int depender_index)
{
  // Get the depender.
  cmGeneratorTarget const* depender = this->Targets[depender_index];
  if (depender->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    return;
  }

  // Loop over all targets linked directly in all configs.
  // We need to make targets depend on the union of all config-specific
  // dependencies in all targets, because the generated build-systems can't
  // deal with config-specific dependencies.
  {
    std::set<std::string> emitted;

    std::vector<std::string> configs;
    depender->Makefile->GetConfigurations(configs);
    if (configs.empty()) {
      configs.push_back("");
    }
    for (std::vector<std::string>::const_iterator it = configs.begin();
         it != configs.end(); ++it) {
      std::vector<cmSourceFile const*> objectFiles;
      depender->GetExternalObjects(objectFiles, *it);
      for (std::vector<cmSourceFile const*>::const_iterator oi =
             objectFiles.begin();
           oi != objectFiles.end(); ++oi) {
        std::string objLib = (*oi)->GetObjectLibrary();
        if (!objLib.empty() && emitted.insert(objLib).second) {
          if (depender->GetType() != cmStateEnums::EXECUTABLE &&
              depender->GetType() != cmStateEnums::STATIC_LIBRARY &&
              depender->GetType() != cmStateEnums::SHARED_LIBRARY &&
              depender->GetType() != cmStateEnums::MODULE_LIBRARY) {
            this->GlobalGenerator->GetCMakeInstance()->IssueMessage(
              cmake::FATAL_ERROR,
              "Only executables and non-OBJECT libraries may "
              "reference target objects.",
              depender->GetBacktrace());
            return;
          }
          const_cast<cmGeneratorTarget*>(depender)->Target->AddUtility(objLib);
        }
      }

      cmLinkImplementation const* impl = depender->GetLinkImplementation(*it);

      // A target should not depend on itself.
      emitted.insert(depender->GetName());
      for (std::vector<cmLinkImplItem>::const_iterator lib =
             impl->Libraries.begin();
           lib != impl->Libraries.end(); ++lib) {
        // Don't emit the same library twice for this target.
        if (emitted.insert(*lib).second) {
          this->AddTargetDepend(depender_index, *lib, true);
          this->AddInterfaceDepends(depender_index, *lib, *it, emitted);
        }
      }
    }
  }

  // Loop over all utility dependencies.
  {
    std::set<cmLinkItem> const& tutils = depender->GetUtilityItems();
    std::set<std::string> emitted;
    // A target should not depend on itself.
    emitted.insert(depender->GetName());
    for (std::set<cmLinkItem>::const_iterator util = tutils.begin();
         util != tutils.end(); ++util) {
      // Don't emit the same utility twice for this target.
      if (emitted.insert(*util).second) {
        this->AddTargetDepend(depender_index, *util, false);
      }
    }
  }
}

void cmComputeTargetDepends::AddInterfaceDepends(
  int depender_index, const cmGeneratorTarget* dependee,
  const std::string& config, std::set<std::string>& emitted)
{
  cmGeneratorTarget const* depender = this->Targets[depender_index];
  if (cmLinkInterface const* iface =
        dependee->GetLinkInterface(config, depender)) {
    for (std::vector<cmLinkItem>::const_iterator lib =
           iface->Libraries.begin();
         lib != iface->Libraries.end(); ++lib) {
      // Don't emit the same library twice for this target.
      if (emitted.insert(*lib).second) {
        this->AddTargetDepend(depender_index, *lib, true);
        this->AddInterfaceDepends(depender_index, *lib, config, emitted);
      }
    }
  }
}

void cmComputeTargetDepends::AddInterfaceDepends(
  int depender_index, cmLinkItem const& dependee_name,
  const std::string& config, std::set<std::string>& emitted)
{
  cmGeneratorTarget const* depender = this->Targets[depender_index];
  cmGeneratorTarget const* dependee = dependee_name.Target;
  // Skip targets that will not really be linked.  This is probably a
  // name conflict between an external library and an executable
  // within the project.
  if (dependee && dependee->GetType() == cmStateEnums::EXECUTABLE &&
      !dependee->IsExecutableWithExports()) {
    dependee = CM_NULLPTR;
  }

  if (dependee) {
    // A target should not depend on itself.
    emitted.insert(depender->GetName());
    this->AddInterfaceDepends(depender_index, dependee, config, emitted);
  }
}

void cmComputeTargetDepends::AddTargetDepend(int depender_index,
                                             cmLinkItem const& dependee_name,
                                             bool linking)
{
  // Get the depender.
  cmGeneratorTarget const* depender = this->Targets[depender_index];

  // Check the target's makefile first.
  cmGeneratorTarget const* dependee = dependee_name.Target;

  if (!dependee && !linking &&
      (depender->GetType() != cmStateEnums::GLOBAL_TARGET)) {
    cmake::MessageType messageType = cmake::AUTHOR_WARNING;
    bool issueMessage = false;
    std::ostringstream e;
    switch (depender->GetPolicyStatusCMP0046()) {
      case cmPolicies::WARN:
        e << cmPolicies::GetPolicyWarning(cmPolicies::CMP0046) << "\n";
        issueMessage = true;
      case cmPolicies::OLD:
        break;
      case cmPolicies::NEW:
      case cmPolicies::REQUIRED_IF_USED:
      case cmPolicies::REQUIRED_ALWAYS:
        issueMessage = true;
        messageType = cmake::FATAL_ERROR;
    }
    if (issueMessage) {
      cmake* cm = this->GlobalGenerator->GetCMakeInstance();

      e << "The dependency target \"" << dependee_name << "\" of target \""
        << depender->GetName() << "\" does not exist.";

      cmListFileBacktrace const* backtrace =
        depender->GetUtilityBacktrace(dependee_name);
      if (backtrace) {
        cm->IssueMessage(messageType, e.str(), *backtrace);
      } else {
        cm->IssueMessage(messageType, e.str());
      }
    }
  }

  // Skip targets that will not really be linked.  This is probably a
  // name conflict between an external library and an executable
  // within the project.
  if (linking && dependee && dependee->GetType() == cmStateEnums::EXECUTABLE &&
      !dependee->IsExecutableWithExports()) {
    dependee = CM_NULLPTR;
  }

  if (dependee) {
    this->AddTargetDepend(depender_index, dependee, linking);
  }
}

void cmComputeTargetDepends::AddTargetDepend(int depender_index,
                                             const cmGeneratorTarget* dependee,
                                             bool linking)
{
  if (dependee->IsImported() ||
      dependee->GetType() == cmStateEnums::INTERFACE_LIBRARY) {
    // Skip IMPORTED and INTERFACE targets but follow their utility
    // dependencies.
    std::set<cmLinkItem> const& utils = dependee->GetUtilityItems();
    for (std::set<cmLinkItem>::const_iterator i = utils.begin();
         i != utils.end(); ++i) {
      if (cmGeneratorTarget const* transitive_dependee = i->Target) {
        this->AddTargetDepend(depender_index, transitive_dependee, false);
      }
    }
  } else {
    // Lookup the index for this target.  All targets should be known by
    // this point.
    std::map<cmGeneratorTarget const*, int>::const_iterator tii =
      this->TargetIndex.find(dependee);
    assert(tii != this->TargetIndex.end());
    int dependee_index = tii->second;

    // Add this entry to the dependency graph.
    this->InitialGraph[depender_index].push_back(
      cmGraphEdge(dependee_index, !linking));
  }
}

void cmComputeTargetDepends::DisplayGraph(Graph const& graph,
                                          const std::string& name)
{
  fprintf(stderr, "The %s target dependency graph is:\n", name.c_str());
  int n = static_cast<int>(graph.size());
  for (int depender_index = 0; depender_index < n; ++depender_index) {
    EdgeList const& nl = graph[depender_index];
    cmGeneratorTarget const* depender = this->Targets[depender_index];
    fprintf(stderr, "target %d is [%s]\n", depender_index,
            depender->GetName().c_str());
    for (EdgeList::const_iterator ni = nl.begin(); ni != nl.end(); ++ni) {
      int dependee_index = *ni;
      cmGeneratorTarget const* dependee = this->Targets[dependee_index];
      fprintf(stderr, "  depends on target %d [%s] (%s)\n", dependee_index,
              dependee->GetName().c_str(), ni->IsStrong() ? "strong" : "weak");
    }
  }
  fprintf(stderr, "\n");
}

void cmComputeTargetDepends::DisplayComponents(
  cmComputeComponentGraph const& ccg)
{
  fprintf(stderr, "The strongly connected components are:\n");
  std::vector<NodeList> const& components = ccg.GetComponents();
  int n = static_cast<int>(components.size());
  for (int c = 0; c < n; ++c) {
    NodeList const& nl = components[c];
    fprintf(stderr, "Component (%d):\n", c);
    for (NodeList::const_iterator ni = nl.begin(); ni != nl.end(); ++ni) {
      int i = *ni;
      fprintf(stderr, "  contains target %d [%s]\n", i,
              this->Targets[i]->GetName().c_str());
    }
  }
  fprintf(stderr, "\n");
}

bool cmComputeTargetDepends::CheckComponents(
  cmComputeComponentGraph const& ccg)
{
  // All non-trivial components should consist only of static
  // libraries.
  std::vector<NodeList> const& components = ccg.GetComponents();
  int nc = static_cast<int>(components.size());
  for (int c = 0; c < nc; ++c) {
    // Get the current component.
    NodeList const& nl = components[c];

    // Skip trivial components.
    if (nl.size() < 2) {
      continue;
    }

    // Immediately complain if no cycles are allowed at all.
    if (this->NoCycles) {
      this->ComplainAboutBadComponent(ccg, c);
      return false;
    }

    // Make sure the component is all STATIC_LIBRARY targets.
    for (NodeList::const_iterator ni = nl.begin(); ni != nl.end(); ++ni) {
      if (this->Targets[*ni]->GetType() != cmStateEnums::STATIC_LIBRARY) {
        this->ComplainAboutBadComponent(ccg, c);
        return false;
      }
    }
  }
  return true;
}

void cmComputeTargetDepends::ComplainAboutBadComponent(
  cmComputeComponentGraph const& ccg, int c, bool strong)
{
  // Construct the error message.
  std::ostringstream e;
  e << "The inter-target dependency graph contains the following "
    << "strongly connected component (cycle):\n";
  std::vector<NodeList> const& components = ccg.GetComponents();
  std::vector<int> const& cmap = ccg.GetComponentMap();
  NodeList const& cl = components[c];
  for (NodeList::const_iterator ci = cl.begin(); ci != cl.end(); ++ci) {
    // Get the depender.
    int i = *ci;
    cmGeneratorTarget const* depender = this->Targets[i];

    // Describe the depender.
    e << "  \"" << depender->GetName() << "\" of type "
      << cmState::GetTargetTypeName(depender->GetType()) << "\n";

    // List its dependencies that are inside the component.
    EdgeList const& nl = this->InitialGraph[i];
    for (EdgeList::const_iterator ni = nl.begin(); ni != nl.end(); ++ni) {
      int j = *ni;
      if (cmap[j] == c) {
        cmGeneratorTarget const* dependee = this->Targets[j];
        e << "    depends on \"" << dependee->GetName() << "\""
          << " (" << (ni->IsStrong() ? "strong" : "weak") << ")\n";
      }
    }
  }
  if (strong) {
    // Custom command executable dependencies cannot occur within a
    // component of static libraries.  The cycle must appear in calls
    // to add_dependencies.
    e << "The component contains at least one cycle consisting of strong "
      << "dependencies (created by add_dependencies) that cannot be broken.";
  } else if (this->NoCycles) {
    e << "The GLOBAL_DEPENDS_NO_CYCLES global property is enabled, so "
      << "cyclic dependencies are not allowed even among static libraries.";
  } else {
    e << "At least one of these targets is not a STATIC_LIBRARY.  "
      << "Cyclic dependencies are allowed only among static libraries.";
  }
  cmSystemTools::Error(e.str().c_str());
}

bool cmComputeTargetDepends::IntraComponent(std::vector<int> const& cmap,
                                            int c, int i, int* head,
                                            std::set<int>& emitted,
                                            std::set<int>& visited)
{
  if (!visited.insert(i).second) {
    // Cycle in utility depends!
    return false;
  }
  if (emitted.insert(i).second) {
    // Honor strong intra-component edges in the final order.
    EdgeList const& el = this->InitialGraph[i];
    for (EdgeList::const_iterator ei = el.begin(); ei != el.end(); ++ei) {
      int j = *ei;
      if (cmap[j] == c && ei->IsStrong()) {
        this->FinalGraph[i].push_back(cmGraphEdge(j, true));
        if (!this->IntraComponent(cmap, c, j, head, emitted, visited)) {
          return false;
        }
      }
    }

    // Prepend to a linear linked-list of intra-component edges.
    if (*head >= 0) {
      this->FinalGraph[i].push_back(cmGraphEdge(*head, false));
    } else {
      this->ComponentTail[c] = i;
    }
    *head = i;
  }
  return true;
}

bool cmComputeTargetDepends::ComputeFinalDepends(
  cmComputeComponentGraph const& ccg)
{
  // Get the component graph information.
  std::vector<NodeList> const& components = ccg.GetComponents();
  Graph const& cgraph = ccg.GetComponentGraph();

  // Allocate the final graph.
  this->FinalGraph.resize(0);
  this->FinalGraph.resize(this->InitialGraph.size());

  // Choose intra-component edges to linearize dependencies.
  std::vector<int> const& cmap = ccg.GetComponentMap();
  this->ComponentHead.resize(components.size());
  this->ComponentTail.resize(components.size());
  int nc = static_cast<int>(components.size());
  for (int c = 0; c < nc; ++c) {
    int head = -1;
    std::set<int> emitted;
    NodeList const& nl = components[c];
    for (NodeList::const_reverse_iterator ni = nl.rbegin(); ni != nl.rend();
         ++ni) {
      std::set<int> visited;
      if (!this->IntraComponent(cmap, c, *ni, &head, emitted, visited)) {
        // Cycle in add_dependencies within component!
        this->ComplainAboutBadComponent(ccg, c, true);
        return false;
      }
    }
    this->ComponentHead[c] = head;
  }

  // Convert inter-component edges to connect component tails to heads.
  int n = static_cast<int>(cgraph.size());
  for (int depender_component = 0; depender_component < n;
       ++depender_component) {
    int depender_component_tail = this->ComponentTail[depender_component];
    EdgeList const& nl = cgraph[depender_component];
    for (EdgeList::const_iterator ni = nl.begin(); ni != nl.end(); ++ni) {
      int dependee_component = *ni;
      int dependee_component_head = this->ComponentHead[dependee_component];
      this->FinalGraph[depender_component_tail].push_back(
        cmGraphEdge(dependee_component_head, ni->IsStrong()));
    }
  }
  return true;
}
