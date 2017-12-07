/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#include "cmComputeComponentGraph.h"

#include <algorithm>

#include <assert.h>

cmComputeComponentGraph::cmComputeComponentGraph(Graph const& input)
  : InputGraph(input)
{
  // Identify components.
  this->Tarjan();

  // Compute the component graph.
  this->ComponentGraph.resize(0);
  this->ComponentGraph.resize(this->Components.size());
  this->TransferEdges();
}

cmComputeComponentGraph::~cmComputeComponentGraph()
{
}

void cmComputeComponentGraph::Tarjan()
{
  int n = static_cast<int>(this->InputGraph.size());
  TarjanEntry entry = { 0, 0 };
  this->TarjanEntries.resize(0);
  this->TarjanEntries.resize(n, entry);
  this->TarjanComponents.resize(0);
  this->TarjanComponents.resize(n, -1);
  this->TarjanWalkId = 0;
  this->TarjanVisited.resize(0);
  this->TarjanVisited.resize(n, 0);
  for (int i = 0; i < n; ++i) {
    // Start a new DFS from this node if it has never been visited.
    if (!this->TarjanVisited[i]) {
      assert(this->TarjanStack.empty());
      ++this->TarjanWalkId;
      this->TarjanIndex = 0;
      this->TarjanVisit(i);
    }
  }
}

void cmComputeComponentGraph::TarjanVisit(int i)
{
  // We are now visiting this node.
  this->TarjanVisited[i] = this->TarjanWalkId;

  // Initialize the entry.
  this->TarjanEntries[i].Root = i;
  this->TarjanComponents[i] = -1;
  this->TarjanEntries[i].VisitIndex = ++this->TarjanIndex;
  this->TarjanStack.push(i);

  // Follow outgoing edges.
  EdgeList const& nl = this->InputGraph[i];
  for (EdgeList::const_iterator ni = nl.begin(); ni != nl.end(); ++ni) {
    int j = *ni;

    // Ignore edges to nodes that have been reached by a previous DFS
    // walk.  Since we did not reach the current node from that walk
    // it must not belong to the same component and it has already
    // been assigned to a component.
    if (this->TarjanVisited[j] > 0 &&
        this->TarjanVisited[j] < this->TarjanWalkId) {
      continue;
    }

    // Visit the destination if it has not yet been visited.
    if (!this->TarjanVisited[j]) {
      this->TarjanVisit(j);
    }

    // If the destination has not yet been assigned to a component,
    // check if it has a better root for the current object.
    if (this->TarjanComponents[j] < 0) {
      if (this->TarjanEntries[this->TarjanEntries[j].Root].VisitIndex <
          this->TarjanEntries[this->TarjanEntries[i].Root].VisitIndex) {
        this->TarjanEntries[i].Root = this->TarjanEntries[j].Root;
      }
    }
  }

  // Check if we have found a component.
  if (this->TarjanEntries[i].Root == i) {
    // Yes.  Create it.
    int c = static_cast<int>(this->Components.size());
    this->Components.push_back(NodeList());
    NodeList& component = this->Components[c];

    // Populate the component list.
    int j;
    do {
      // Get the next member of the component.
      j = this->TarjanStack.top();
      this->TarjanStack.pop();

      // Assign the member to the component.
      this->TarjanComponents[j] = c;
      this->TarjanEntries[j].Root = i;

      // Store the node in its component.
      component.push_back(j);
    } while (j != i);

    // Sort the component members for clarity.
    std::sort(component.begin(), component.end());
  }
}

void cmComputeComponentGraph::TransferEdges()
{
  // Map inter-component edges in the original graph to edges in the
  // component graph.
  int n = static_cast<int>(this->InputGraph.size());
  for (int i = 0; i < n; ++i) {
    int i_component = this->TarjanComponents[i];
    EdgeList const& nl = this->InputGraph[i];
    for (EdgeList::const_iterator ni = nl.begin(); ni != nl.end(); ++ni) {
      int j = *ni;
      int j_component = this->TarjanComponents[j];
      if (i_component != j_component) {
        // We do not attempt to combine duplicate edges, but instead
        // store the inter-component edges with suitable multiplicity.
        this->ComponentGraph[i_component].push_back(
          cmGraphEdge(j_component, ni->IsStrong()));
      }
    }
  }
}
