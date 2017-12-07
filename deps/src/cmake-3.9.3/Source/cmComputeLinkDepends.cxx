/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmComputeLinkDepends.h"

#include "cmAlgorithms.h"
#include "cmComputeComponentGraph.h"
#include "cmGeneratorTarget.h"
#include "cmGlobalGenerator.h"
#include "cmLocalGenerator.h"
#include "cmMakefile.h"
#include "cmStateTypes.h"
#include "cmSystemTools.h"
#include "cmTarget.h"
#include "cmake.h"

#include <algorithm>
#include <assert.h>
#include <iterator>
#include <sstream>
#include <stdio.h>
#include <string.h>
#include <utility>

/*

This file computes an ordered list of link items to use when linking a
single target in one configuration.  Each link item is identified by
the string naming it.  A graph of dependencies is created in which
each node corresponds to one item and directed edges lead from nodes to
those which must *follow* them on the link line.  For example, the
graph

  A -> B -> C

will lead to the link line order

  A B C

The set of items placed in the graph is formed with a breadth-first
search of the link dependencies starting from the main target.

There are two types of items: those with known direct dependencies and
those without known dependencies.  We will call the two types "known
items" and "unknown items", respectively.  Known items are those whose
names correspond to targets (built or imported) and those for which an
old-style <item>_LIB_DEPENDS variable is defined.  All other items are
unknown and we must infer dependencies for them.  For items that look
like flags (beginning with '-') we trivially infer no dependencies,
and do not include them in the dependencies of other items.

Known items have dependency lists ordered based on how the user
specified them.  We can use this order to infer potential dependencies
of unknown items.  For example, if link items A and B are unknown and
items X and Y are known, then we might have the following dependency
lists:

  X: Y A B
  Y: A B

The explicitly known dependencies form graph edges

  X -> Y  ,  X -> A  ,  X -> B  ,  Y -> A  ,  Y -> B

We can also infer the edge

  A -> B

because *every* time A appears B is seen on its right.  We do not know
whether A really needs symbols from B to link, but it *might* so we
must preserve their order.  This is the case also for the following
explicit lists:

  X: A B Y
  Y: A B

Here, A is followed by the set {B,Y} in one list, and {B} in the other
list.  The intersection of these sets is {B}, so we can infer that A
depends on at most B.  Meanwhile B is followed by the set {Y} in one
list and {} in the other.  The intersection is {} so we can infer that
B has no dependencies.

Let's make a more complex example by adding unknown item C and
considering these dependency lists:

  X: A B Y C
  Y: A C B

The explicit edges are

  X -> Y  ,  X -> A  ,  X -> B  ,  X -> C  ,  Y -> A  ,  Y -> B  ,  Y -> C

For the unknown items, we infer dependencies by looking at the
"follow" sets:

  A: intersect( {B,Y,C} , {C,B} ) = {B,C} ; infer edges  A -> B  ,  A -> C
  B: intersect( {Y,C}   , {}    ) = {}    ; infer no edges
  C: intersect( {}      , {B}   ) = {}    ; infer no edges

Note that targets are never inferred as dependees because outside
libraries should not depend on them.

------------------------------------------------------------------------------

The initial exploration of dependencies using a BFS associates an
integer index with each link item.  When the graph is built outgoing
edges are sorted by this index.

After the initial exploration of the link interface tree, any
transitive (dependent) shared libraries that were encountered and not
included in the interface are processed in their own BFS.  This BFS
follows only the dependent library lists and not the link interfaces.
They are added to the link items with a mark indicating that the are
transitive dependencies.  Then cmComputeLinkInformation deals with
them on a per-platform basis.

The complete graph formed from all known and inferred dependencies may
not be acyclic, so an acyclic version must be created.
The original graph is converted to a directed acyclic graph in which
each node corresponds to a strongly connected component of the
original graph.  For example, the dependency graph

  X -> A -> B -> C -> A -> Y

contains strongly connected components {X}, {A,B,C}, and {Y}.  The
implied directed acyclic graph (DAG) is

  {X} -> {A,B,C} -> {Y}

We then compute a topological order for the DAG nodes to serve as a
reference for satisfying dependencies efficiently.  We perform the DFS
in reverse order and assign topological order indices counting down so
that the result is as close to the original BFS order as possible
without violating dependencies.

------------------------------------------------------------------------------

The final link entry order is constructed as follows.  We first walk
through and emit the *original* link line as specified by the user.
As each item is emitted, a set of pending nodes in the component DAG
is maintained.  When a pending component has been completely seen, it
is removed from the pending set and its dependencies (following edges
of the DAG) are added.  A trivial component (those with one item) is
complete as soon as its item is seen.  A non-trivial component (one
with more than one item; assumed to be static libraries) is complete
when *all* its entries have been seen *twice* (all entries seen once,
then all entries seen again, not just each entry twice).  A pending
component tracks which items have been seen and a count of how many
times the component needs to be seen (once for trivial components,
twice for non-trivial).  If at any time another component finishes and
re-adds an already pending component, the pending component is reset
so that it needs to be seen in its entirety again.  This ensures that
all dependencies of a component are satisfied no matter where it
appears.

After the original link line has been completed, we append to it the
remaining pending components and their dependencies.  This is done by
repeatedly emitting the first item from the first pending component
and following the same update rules as when traversing the original
link line.  Since the pending components are kept in topological order
they are emitted with minimal repeats (we do not want to emit a
component just to have it added again when another component is
completed later).  This process continues until no pending components
remain.  We know it will terminate because the component graph is
guaranteed to be acyclic.

The final list of items produced by this procedure consists of the
original user link line followed by minimal additional items needed to
satisfy dependencies.  The final list is then filtered to de-duplicate
items that we know the linker will re-use automatically (shared libs).

*/

cmComputeLinkDepends::cmComputeLinkDepends(const cmGeneratorTarget* target,
                                           const std::string& config)
{
  // Store context information.
  this->Target = target;
  this->Makefile = this->Target->Target->GetMakefile();
  this->GlobalGenerator =
    this->Target->GetLocalGenerator()->GetGlobalGenerator();
  this->CMakeInstance = this->GlobalGenerator->GetCMakeInstance();

  // The configuration being linked.
  this->HasConfig = !config.empty();
  this->Config = (this->HasConfig) ? config : std::string();
  std::vector<std::string> debugConfigs =
    this->Makefile->GetCMakeInstance()->GetDebugConfigs();
  this->LinkType = CMP0003_ComputeLinkType(this->Config, debugConfigs);

  // Enable debug mode if requested.
  this->DebugMode = this->Makefile->IsOn("CMAKE_LINK_DEPENDS_DEBUG_MODE");

  // Assume no compatibility until set.
  this->OldLinkDirMode = false;

  // No computation has been done.
  this->CCG = CM_NULLPTR;
}

cmComputeLinkDepends::~cmComputeLinkDepends()
{
  cmDeleteAll(this->InferredDependSets);
  delete this->CCG;
}

void cmComputeLinkDepends::SetOldLinkDirMode(bool b)
{
  this->OldLinkDirMode = b;
}

std::vector<cmComputeLinkDepends::LinkEntry> const&
cmComputeLinkDepends::Compute()
{
  // Follow the link dependencies of the target to be linked.
  this->AddDirectLinkEntries();

  // Complete the breadth-first search of dependencies.
  while (!this->BFSQueue.empty()) {
    // Get the next entry.
    BFSEntry qe = this->BFSQueue.front();
    this->BFSQueue.pop();

    // Follow the entry's dependencies.
    this->FollowLinkEntry(qe);
  }

  // Complete the search of shared library dependencies.
  while (!this->SharedDepQueue.empty()) {
    // Handle the next entry.
    this->HandleSharedDependency(this->SharedDepQueue.front());
    this->SharedDepQueue.pop();
  }

  // Infer dependencies of targets for which they were not known.
  this->InferDependencies();

  // Cleanup the constraint graph.
  this->CleanConstraintGraph();

  // Display the constraint graph.
  if (this->DebugMode) {
    fprintf(stderr, "---------------------------------------"
                    "---------------------------------------\n");
    fprintf(stderr, "Link dependency analysis for target %s, config %s\n",
            this->Target->GetName().c_str(),
            this->HasConfig ? this->Config.c_str() : "noconfig");
    this->DisplayConstraintGraph();
  }

  // Compute the final ordering.
  this->OrderLinkEntires();

  // Compute the final set of link entries.
  // Iterate in reverse order so we can keep only the last occurrence
  // of a shared library.
  std::set<int> emmitted;
  for (std::vector<int>::const_reverse_iterator
         li = this->FinalLinkOrder.rbegin(),
         le = this->FinalLinkOrder.rend();
       li != le; ++li) {
    int i = *li;
    LinkEntry const& e = this->EntryList[i];
    cmGeneratorTarget const* t = e.Target;
    // Entries that we know the linker will re-use do not need to be repeated.
    bool uniquify = t && t->GetType() == cmStateEnums::SHARED_LIBRARY;
    if (!uniquify || emmitted.insert(i).second) {
      this->FinalLinkEntries.push_back(e);
    }
  }
  // Reverse the resulting order since we iterated in reverse.
  std::reverse(this->FinalLinkEntries.begin(), this->FinalLinkEntries.end());

  // Display the final set.
  if (this->DebugMode) {
    this->DisplayFinalEntries();
  }

  return this->FinalLinkEntries;
}

std::map<std::string, int>::iterator cmComputeLinkDepends::AllocateLinkEntry(
  std::string const& item)
{
  std::map<std::string, int>::value_type index_entry(
    item, static_cast<int>(this->EntryList.size()));
  std::map<std::string, int>::iterator lei =
    this->LinkEntryIndex.insert(index_entry).first;
  this->EntryList.push_back(LinkEntry());
  this->InferredDependSets.push_back(CM_NULLPTR);
  this->EntryConstraintGraph.push_back(EdgeList());
  return lei;
}

int cmComputeLinkDepends::AddLinkEntry(cmLinkItem const& item)
{
  // Check if the item entry has already been added.
  std::map<std::string, int>::iterator lei = this->LinkEntryIndex.find(item);
  if (lei != this->LinkEntryIndex.end()) {
    // Yes.  We do not need to follow the item's dependencies again.
    return lei->second;
  }

  // Allocate a spot for the item entry.
  lei = this->AllocateLinkEntry(item);

  // Initialize the item entry.
  int index = lei->second;
  LinkEntry& entry = this->EntryList[index];
  entry.Item = item;
  entry.Target = item.Target;
  entry.IsFlag = (!entry.Target && item[0] == '-' && item[1] != 'l' &&
                  item.substr(0, 10) != "-framework");

  // If the item has dependencies queue it to follow them.
  if (entry.Target) {
    // Target dependencies are always known.  Follow them.
    BFSEntry qe = { index, CM_NULLPTR };
    this->BFSQueue.push(qe);
  } else {
    // Look for an old-style <item>_LIB_DEPENDS variable.
    std::string var = entry.Item;
    var += "_LIB_DEPENDS";
    if (const char* val = this->Makefile->GetDefinition(var)) {
      // The item dependencies are known.  Follow them.
      BFSEntry qe = { index, val };
      this->BFSQueue.push(qe);
    } else if (!entry.IsFlag) {
      // The item dependencies are not known.  We need to infer them.
      this->InferredDependSets[index] = new DependSetList;
    }
  }

  return index;
}

void cmComputeLinkDepends::FollowLinkEntry(BFSEntry qe)
{
  // Get this entry representation.
  int depender_index = qe.Index;
  LinkEntry const& entry = this->EntryList[depender_index];

  // Follow the item's dependencies.
  if (entry.Target) {
    // Follow the target dependencies.
    if (cmLinkInterface const* iface =
          entry.Target->GetLinkInterface(this->Config, this->Target)) {
      const bool isIface =
        entry.Target->GetType() == cmStateEnums::INTERFACE_LIBRARY;
      // This target provides its own link interface information.
      this->AddLinkEntries(depender_index, iface->Libraries);

      if (isIface) {
        return;
      }

      // Handle dependent shared libraries.
      this->FollowSharedDeps(depender_index, iface);

      // Support for CMP0003.
      for (std::vector<cmLinkItem>::const_iterator oi =
             iface->WrongConfigLibraries.begin();
           oi != iface->WrongConfigLibraries.end(); ++oi) {
        this->CheckWrongConfigItem(*oi);
      }
    }
  } else {
    // Follow the old-style dependency list.
    this->AddVarLinkEntries(depender_index, qe.LibDepends);
  }
}

void cmComputeLinkDepends::FollowSharedDeps(int depender_index,
                                            cmLinkInterface const* iface,
                                            bool follow_interface)
{
  // Follow dependencies if we have not followed them already.
  if (this->SharedDepFollowed.insert(depender_index).second) {
    if (follow_interface) {
      this->QueueSharedDependencies(depender_index, iface->Libraries);
    }
    this->QueueSharedDependencies(depender_index, iface->SharedDeps);
  }
}

void cmComputeLinkDepends::QueueSharedDependencies(
  int depender_index, std::vector<cmLinkItem> const& deps)
{
  for (std::vector<cmLinkItem>::const_iterator li = deps.begin();
       li != deps.end(); ++li) {
    SharedDepEntry qe;
    qe.Item = *li;
    qe.DependerIndex = depender_index;
    this->SharedDepQueue.push(qe);
  }
}

void cmComputeLinkDepends::HandleSharedDependency(SharedDepEntry const& dep)
{
  // Check if the target already has an entry.
  std::map<std::string, int>::iterator lei =
    this->LinkEntryIndex.find(dep.Item);
  if (lei == this->LinkEntryIndex.end()) {
    // Allocate a spot for the item entry.
    lei = this->AllocateLinkEntry(dep.Item);

    // Initialize the item entry.
    LinkEntry& entry = this->EntryList[lei->second];
    entry.Item = dep.Item;
    entry.Target = dep.Item.Target;

    // This item was added specifically because it is a dependent
    // shared library.  It may get special treatment
    // in cmComputeLinkInformation.
    entry.IsSharedDep = true;
  }

  // Get the link entry for this target.
  int index = lei->second;
  LinkEntry& entry = this->EntryList[index];

  // This shared library dependency must follow the item that listed
  // it.
  this->EntryConstraintGraph[dep.DependerIndex].push_back(index);

  // Target items may have their own dependencies.
  if (entry.Target) {
    if (cmLinkInterface const* iface =
          entry.Target->GetLinkInterface(this->Config, this->Target)) {
      // Follow public and private dependencies transitively.
      this->FollowSharedDeps(index, iface, true);
    }
  }
}

void cmComputeLinkDepends::AddVarLinkEntries(int depender_index,
                                             const char* value)
{
  // This is called to add the dependencies named by
  // <item>_LIB_DEPENDS.  The variable contains a semicolon-separated
  // list.  The list contains link-type;item pairs and just items.
  std::vector<std::string> deplist;
  cmSystemTools::ExpandListArgument(value, deplist);

  // Look for entries meant for this configuration.
  std::vector<cmLinkItem> actual_libs;
  cmTargetLinkLibraryType llt = GENERAL_LibraryType;
  bool haveLLT = false;
  for (std::vector<std::string>::const_iterator di = deplist.begin();
       di != deplist.end(); ++di) {
    if (*di == "debug") {
      llt = DEBUG_LibraryType;
      haveLLT = true;
    } else if (*di == "optimized") {
      llt = OPTIMIZED_LibraryType;
      haveLLT = true;
    } else if (*di == "general") {
      llt = GENERAL_LibraryType;
      haveLLT = true;
    } else if (!di->empty()) {
      // If no explicit link type was given prior to this entry then
      // check if the entry has its own link type variable.  This is
      // needed for compatibility with dependency files generated by
      // the export_library_dependencies command from CMake 2.4 and
      // lower.
      if (!haveLLT) {
        std::string var = *di;
        var += "_LINK_TYPE";
        if (const char* val = this->Makefile->GetDefinition(var)) {
          if (strcmp(val, "debug") == 0) {
            llt = DEBUG_LibraryType;
          } else if (strcmp(val, "optimized") == 0) {
            llt = OPTIMIZED_LibraryType;
          }
        }
      }

      // If the library is meant for this link type then use it.
      if (llt == GENERAL_LibraryType || llt == this->LinkType) {
        cmLinkItem item(*di, this->FindTargetToLink(depender_index, *di));
        actual_libs.push_back(item);
      } else if (this->OldLinkDirMode) {
        cmLinkItem item(*di, this->FindTargetToLink(depender_index, *di));
        this->CheckWrongConfigItem(item);
      }

      // Reset the link type until another explicit type is given.
      llt = GENERAL_LibraryType;
      haveLLT = false;
    }
  }

  // Add the entries from this list.
  this->AddLinkEntries(depender_index, actual_libs);
}

void cmComputeLinkDepends::AddDirectLinkEntries()
{
  // Add direct link dependencies in this configuration.
  cmLinkImplementation const* impl =
    this->Target->GetLinkImplementation(this->Config);
  this->AddLinkEntries(-1, impl->Libraries);
  for (std::vector<cmLinkItem>::const_iterator wi =
         impl->WrongConfigLibraries.begin();
       wi != impl->WrongConfigLibraries.end(); ++wi) {
    this->CheckWrongConfigItem(*wi);
  }
}

template <typename T>
void cmComputeLinkDepends::AddLinkEntries(int depender_index,
                                          std::vector<T> const& libs)
{
  // Track inferred dependency sets implied by this list.
  std::map<int, DependSet> dependSets;

  // Loop over the libraries linked directly by the depender.
  for (typename std::vector<T>::const_iterator li = libs.begin();
       li != libs.end(); ++li) {
    // Skip entries that will resolve to the target getting linked or
    // are empty.
    cmLinkItem const& item = *li;
    if (item == this->Target->GetName() || item.empty()) {
      continue;
    }

    // Add a link entry for this item.
    int dependee_index = this->AddLinkEntry(*li);

    // The dependee must come after the depender.
    if (depender_index >= 0) {
      this->EntryConstraintGraph[depender_index].push_back(dependee_index);
    } else {
      // This is a direct dependency of the target being linked.
      this->OriginalEntries.push_back(dependee_index);
    }

    // Update the inferred dependencies for earlier items.
    for (std::map<int, DependSet>::iterator dsi = dependSets.begin();
         dsi != dependSets.end(); ++dsi) {
      // Add this item to the inferred dependencies of other items.
      // Target items are never inferred dependees because unknown
      // items are outside libraries that should not be depending on
      // targets.
      if (!this->EntryList[dependee_index].Target &&
          !this->EntryList[dependee_index].IsFlag &&
          dependee_index != dsi->first) {
        dsi->second.insert(dependee_index);
      }
    }

    // If this item needs to have dependencies inferred, do so.
    if (this->InferredDependSets[dependee_index]) {
      // Make sure an entry exists to hold the set for the item.
      dependSets[dependee_index];
    }
  }

  // Store the inferred dependency sets discovered for this list.
  for (std::map<int, DependSet>::iterator dsi = dependSets.begin();
       dsi != dependSets.end(); ++dsi) {
    this->InferredDependSets[dsi->first]->push_back(dsi->second);
  }
}

cmGeneratorTarget const* cmComputeLinkDepends::FindTargetToLink(
  int depender_index, const std::string& name)
{
  // Look for a target in the scope of the depender.
  cmGeneratorTarget const* from = this->Target;
  if (depender_index >= 0) {
    if (cmGeneratorTarget const* depender =
          this->EntryList[depender_index].Target) {
      from = depender;
    }
  }
  return from->FindTargetToLink(name);
}

void cmComputeLinkDepends::InferDependencies()
{
  // The inferred dependency sets for each item list the possible
  // dependencies.  The intersection of the sets for one item form its
  // inferred dependencies.
  for (unsigned int depender_index = 0;
       depender_index < this->InferredDependSets.size(); ++depender_index) {
    // Skip items for which dependencies do not need to be inferred or
    // for which the inferred dependency sets are empty.
    DependSetList* sets = this->InferredDependSets[depender_index];
    if (!sets || sets->empty()) {
      continue;
    }

    // Intersect the sets for this item.
    DependSetList::const_iterator i = sets->begin();
    DependSet common = *i;
    for (++i; i != sets->end(); ++i) {
      DependSet intersection;
      std::set_intersection(common.begin(), common.end(), i->begin(), i->end(),
                            std::inserter(intersection, intersection.begin()));
      common = intersection;
    }

    // Add the inferred dependencies to the graph.
    cmGraphEdgeList& edges = this->EntryConstraintGraph[depender_index];
    edges.insert(edges.end(), common.begin(), common.end());
  }
}

void cmComputeLinkDepends::CleanConstraintGraph()
{
  for (Graph::iterator i = this->EntryConstraintGraph.begin();
       i != this->EntryConstraintGraph.end(); ++i) {
    // Sort the outgoing edges for each graph node so that the
    // original order will be preserved as much as possible.
    std::sort(i->begin(), i->end());

    // Make the edge list unique.
    i->erase(std::unique(i->begin(), i->end()), i->end());
  }
}

void cmComputeLinkDepends::DisplayConstraintGraph()
{
  // Display the graph nodes and their edges.
  std::ostringstream e;
  for (unsigned int i = 0; i < this->EntryConstraintGraph.size(); ++i) {
    EdgeList const& nl = this->EntryConstraintGraph[i];
    e << "item " << i << " is [" << this->EntryList[i].Item << "]\n";
    e << cmWrap("  item ", nl, " must follow it", "\n") << "\n";
  }
  fprintf(stderr, "%s\n", e.str().c_str());
}

void cmComputeLinkDepends::OrderLinkEntires()
{
  // Compute the DAG of strongly connected components.  The algorithm
  // used by cmComputeComponentGraph should identify the components in
  // the same order in which the items were originally discovered in
  // the BFS.  This should preserve the original order when no
  // constraints disallow it.
  this->CCG = new cmComputeComponentGraph(this->EntryConstraintGraph);

  // The component graph is guaranteed to be acyclic.  Start a DFS
  // from every entry to compute a topological order for the
  // components.
  Graph const& cgraph = this->CCG->GetComponentGraph();
  int n = static_cast<int>(cgraph.size());
  this->ComponentVisited.resize(cgraph.size(), 0);
  this->ComponentOrder.resize(cgraph.size(), n);
  this->ComponentOrderId = n;
  // Run in reverse order so the topological order will preserve the
  // original order where there are no constraints.
  for (int c = n - 1; c >= 0; --c) {
    this->VisitComponent(c);
  }

  // Display the component graph.
  if (this->DebugMode) {
    this->DisplayComponents();
  }

  // Start with the original link line.
  for (std::vector<int>::const_iterator i = this->OriginalEntries.begin();
       i != this->OriginalEntries.end(); ++i) {
    this->VisitEntry(*i);
  }

  // Now explore anything left pending.  Since the component graph is
  // guaranteed to be acyclic we know this will terminate.
  while (!this->PendingComponents.empty()) {
    // Visit one entry from the first pending component.  The visit
    // logic will update the pending components accordingly.  Since
    // the pending components are kept in topological order this will
    // not repeat one.
    int e = *this->PendingComponents.begin()->second.Entries.begin();
    this->VisitEntry(e);
  }
}

void cmComputeLinkDepends::DisplayComponents()
{
  fprintf(stderr, "The strongly connected components are:\n");
  std::vector<NodeList> const& components = this->CCG->GetComponents();
  for (unsigned int c = 0; c < components.size(); ++c) {
    fprintf(stderr, "Component (%u):\n", c);
    NodeList const& nl = components[c];
    for (NodeList::const_iterator ni = nl.begin(); ni != nl.end(); ++ni) {
      int i = *ni;
      fprintf(stderr, "  item %d [%s]\n", i, this->EntryList[i].Item.c_str());
    }
    EdgeList const& ol = this->CCG->GetComponentGraphEdges(c);
    for (EdgeList::const_iterator oi = ol.begin(); oi != ol.end(); ++oi) {
      int i = *oi;
      fprintf(stderr, "  followed by Component (%d)\n", i);
    }
    fprintf(stderr, "  topo order index %d\n", this->ComponentOrder[c]);
  }
  fprintf(stderr, "\n");
}

void cmComputeLinkDepends::VisitComponent(unsigned int c)
{
  // Check if the node has already been visited.
  if (this->ComponentVisited[c]) {
    return;
  }

  // We are now visiting this component so mark it.
  this->ComponentVisited[c] = 1;

  // Visit the neighbors of the component first.
  // Run in reverse order so the topological order will preserve the
  // original order where there are no constraints.
  EdgeList const& nl = this->CCG->GetComponentGraphEdges(c);
  for (EdgeList::const_reverse_iterator ni = nl.rbegin(); ni != nl.rend();
       ++ni) {
    this->VisitComponent(*ni);
  }

  // Assign an ordering id to this component.
  this->ComponentOrder[c] = --this->ComponentOrderId;
}

void cmComputeLinkDepends::VisitEntry(int index)
{
  // Include this entry on the link line.
  this->FinalLinkOrder.push_back(index);

  // This entry has now been seen.  Update its component.
  bool completed = false;
  int component = this->CCG->GetComponentMap()[index];
  std::map<int, PendingComponent>::iterator mi =
    this->PendingComponents.find(this->ComponentOrder[component]);
  if (mi != this->PendingComponents.end()) {
    // The entry is in an already pending component.
    PendingComponent& pc = mi->second;

    // Remove the entry from those pending in its component.
    pc.Entries.erase(index);
    if (pc.Entries.empty()) {
      // The complete component has been seen since it was last needed.
      --pc.Count;

      if (pc.Count == 0) {
        // The component has been completed.
        this->PendingComponents.erase(mi);
        completed = true;
      } else {
        // The whole component needs to be seen again.
        NodeList const& nl = this->CCG->GetComponent(component);
        assert(nl.size() > 1);
        pc.Entries.insert(nl.begin(), nl.end());
      }
    }
  } else {
    // The entry is not in an already pending component.
    NodeList const& nl = this->CCG->GetComponent(component);
    if (nl.size() > 1) {
      // This is a non-trivial component.  It is now pending.
      PendingComponent& pc = this->MakePendingComponent(component);

      // The starting entry has already been seen.
      pc.Entries.erase(index);
    } else {
      // This is a trivial component, so it is already complete.
      completed = true;
    }
  }

  // If the entry completed a component, the component's dependencies
  // are now pending.
  if (completed) {
    EdgeList const& ol = this->CCG->GetComponentGraphEdges(component);
    for (EdgeList::const_iterator oi = ol.begin(); oi != ol.end(); ++oi) {
      // This entire component is now pending no matter whether it has
      // been partially seen already.
      this->MakePendingComponent(*oi);
    }
  }
}

cmComputeLinkDepends::PendingComponent&
cmComputeLinkDepends::MakePendingComponent(unsigned int component)
{
  // Create an entry (in topological order) for the component.
  PendingComponent& pc =
    this->PendingComponents[this->ComponentOrder[component]];
  pc.Id = component;
  NodeList const& nl = this->CCG->GetComponent(component);

  if (nl.size() == 1) {
    // Trivial components need be seen only once.
    pc.Count = 1;
  } else {
    // This is a non-trivial strongly connected component of the
    // original graph.  It consists of two or more libraries
    // (archives) that mutually require objects from one another.  In
    // the worst case we may have to repeat the list of libraries as
    // many times as there are object files in the biggest archive.
    // For now we just list them twice.
    //
    // The list of items in the component has been sorted by the order
    // of discovery in the original BFS of dependencies.  This has the
    // advantage that the item directly linked by a target requiring
    // this component will come first which minimizes the number of
    // repeats needed.
    pc.Count = this->ComputeComponentCount(nl);
  }

  // Store the entries to be seen.
  pc.Entries.insert(nl.begin(), nl.end());

  return pc;
}

int cmComputeLinkDepends::ComputeComponentCount(NodeList const& nl)
{
  unsigned int count = 2;
  for (NodeList::const_iterator ni = nl.begin(); ni != nl.end(); ++ni) {
    if (cmGeneratorTarget const* target = this->EntryList[*ni].Target) {
      if (cmLinkInterface const* iface =
            target->GetLinkInterface(this->Config, this->Target)) {
        if (iface->Multiplicity > count) {
          count = iface->Multiplicity;
        }
      }
    }
  }
  return count;
}

void cmComputeLinkDepends::DisplayFinalEntries()
{
  fprintf(stderr, "target [%s] links to:\n", this->Target->GetName().c_str());
  for (std::vector<LinkEntry>::const_iterator lei =
         this->FinalLinkEntries.begin();
       lei != this->FinalLinkEntries.end(); ++lei) {
    if (lei->Target) {
      fprintf(stderr, "  target [%s]\n", lei->Target->GetName().c_str());
    } else {
      fprintf(stderr, "  item [%s]\n", lei->Item.c_str());
    }
  }
  fprintf(stderr, "\n");
}

void cmComputeLinkDepends::CheckWrongConfigItem(cmLinkItem const& item)
{
  if (!this->OldLinkDirMode) {
    return;
  }

  // For CMake 2.4 bug-compatibility we need to consider the output
  // directories of targets linked in another configuration as link
  // directories.
  if (item.Target && !item.Target->IsImported()) {
    this->OldWrongConfigItems.insert(item.Target);
  }
}
