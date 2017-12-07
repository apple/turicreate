/* Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
   file Copyright.txt or https://cmake.org/licensing for details.  */
#ifndef cmComputeComponentGraph_h
#define cmComputeComponentGraph_h

#include "cmConfigure.h" // IWYU pragma: keep

#include "cmGraphAdjacencyList.h"

#include <stack>
#include <vector>

/** \class cmComputeComponentGraph
 * \brief Analyze a graph to determine strongly connected components.
 *
 * Convert a directed graph into a directed acyclic graph whose nodes
 * correspond to strongly connected components of the original graph.
 *
 * We use Tarjan's algorithm to enumerate the components efficiently.
 * An advantage of this approach is that the components are identified
 * in a topologically sorted order.
 */
class cmComputeComponentGraph
{
public:
  // Represent the graph with an adjacency list.
  typedef cmGraphNodeList NodeList;
  typedef cmGraphEdgeList EdgeList;
  typedef cmGraphAdjacencyList Graph;

  cmComputeComponentGraph(Graph const& input);
  ~cmComputeComponentGraph();

  /** Get the adjacency list of the component graph.  */
  Graph const& GetComponentGraph() const { return this->ComponentGraph; }
  EdgeList const& GetComponentGraphEdges(int c) const
  {
    return this->ComponentGraph[c];
  }

  /** Get map from component index to original node indices.  */
  std::vector<NodeList> const& GetComponents() const
  {
    return this->Components;
  }
  NodeList const& GetComponent(int c) const { return this->Components[c]; }

  /** Get map from original node index to component index.  */
  std::vector<int> const& GetComponentMap() const
  {
    return this->TarjanComponents;
  }

private:
  void TransferEdges();

  Graph const& InputGraph;
  Graph ComponentGraph;

  // Tarjan's algorithm.
  struct TarjanEntry
  {
    int Root;
    int VisitIndex;
  };
  std::vector<int> TarjanVisited;
  std::vector<int> TarjanComponents;
  std::vector<TarjanEntry> TarjanEntries;
  std::vector<NodeList> Components;
  std::stack<int> TarjanStack;
  int TarjanWalkId;
  int TarjanIndex;
  void Tarjan();
  void TarjanVisit(int i);

  // Connected components.
};

#endif
