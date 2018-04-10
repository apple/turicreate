# Working with Graphs

Graphs allow us to understand complex networks by focusing on
relationships between pairs of items. Each item is represented by a
*vertex* in the graph, and relationships between items are represented
by *edges*.

To facilitate graph-oriented data analysis, Turi Create offers a
[SGraph](https://apple.github.io/turicreate/docs/api/generated/turicreate.SGraph.html)
object, a scalable graph data structure backed by SFrames.  In this
chapter, we show that SGraphs allow arbitrary dictionary attributes on
vertices and edges, flexible vertex and edge query functions, and
seamless transformation to and from SFrames.

#### Creating an SGraph

There are several ways to create an SGraph. The simplest is to start
with an empty graph, then add vertices and edges in the form of lists of
turicreate.Vertex and turicreate.Edge objects. SGraphs are structurally
immutable; in the following snippet, `add_vertices` and `add_edges` both
return a new graph.

```python
from turicreate import SGraph, Vertex, Edge
g = SGraph()
verts = [Vertex(0, attr={'breed': 'labrador'}),
         Vertex(1, attr={'breed': 'labrador'}),
         Vertex(2, attr={'breed': 'vizsla'})]
g = g.add_vertices(verts)
g = g.add_edges(Edge(1, 2))
print g
```
```no-highlight
SGraph({'num_edges': 1, 'num_vertices': 3})
Vertex Fields:['__id', 'breed']
Edge Fields:['__src_id', '__dst_id']
```
We can chain these steps together to make a new graph in a single line.

```python
g = SGraph().add_vertices([Vertex(i) for i in range(10)]).add_edges(
    [Edge(i, i+1) for i in range(9)])
```

SGraphs can also be created from an edge list stored in an SFrame. Vertices are
added to the graph automatically based on the edge list, and columns of the
SFrame not used as source or destination vertex IDs are assumed to be edge
attributes. Suppose we import a dataset of James Bond characters to
an SFrame, then build the graph.

```python
from turicreate import SFrame
edge_data = SFrame.read_csv('bond_edges.csv')

g = SGraph()
g = g.add_edges(edge_data, src_field='src', dst_field='dst')
print(g)
```
```no-highlight
SGraph({'num_edges': 20, 'num_vertices': 10})
```

The SGraph constructor also accepts vertex and edge SFrames directly. We can
construct the same James Bond graph with the following two lines:

```python
vertex_data = SFrame.read_csv('bond_vertices.csv')

g = SGraph(vertices=vertex_data, edges=edge_data, vid_field='name',
           src_field='src', dst_field='dst')
```

Finally, an SGraph can be created directly from a file, either local or
remote, using the
[turicreate.load_sgraph()](https://apple.github.io/turicreate/docs/api/generated/turicreate.load_sgraph.html)
method. Loading a graph with this method works with both the native
binary save format and a variety of text formats. In the following
example we save the SGraph in binary format to a new folder called
"james_bond", then re-load it under a different name.

```python
g.save('james_bond.sgraph')
new_graph = turicreate.load_sgraph('james_bond.sgraph')
```

#### Inspecting SGraphs

Small graphs can be explored very efficiently with the `SGraph.show` method,
which displays a plot of the graph. The vertex labels can be IDs or any vertex
attribute.

![Bond graph](images/bond_basic.png)

For large graphs visual displays are difficult, but graph exploration
can still be done with the `SGraph.summary`---which prints the number of
vertices and edges---or by retrieving and plotting subsets of edges and
vertices.

```python
print(g.summary())
```
```no-highlight
{'num_edges': 20, 'num_vertices': 10}
```

To retrieve the contents of an SGraph, the `get_vertices` and `get_edges`
methods return SFrames. These functions can filter edges and vertices based on
vertex IDs or attributes. Omitting IDs and attributes returns all vertices or
edges.

```python
sub_verts = g.get_vertices(ids=['James Bond'])
print(sub_verts)
```
```no-highlight
+------------+--------+-----------------+---------+
|    __id    | gender | license_to_kill | villain |
+------------+--------+-----------------+---------+
| James Bond |   M    |        1        |    0    |
+------------+--------+-----------------+---------+
[1 rows x 4 columns]
```

```python
sub_edges = g.get_edges(fields={'relation': 'worksfor'})
print(sub_edges)
```
```no-highlight
+---------------+-------------+----------+
|    __src_id   |   __dst_id  | relation |
+---------------+-------------+----------+
|       M       |  Moneypenny | worksfor |
|       M       |  James Bond | worksfor |
|       M       |      Q      | worksfor |
| Elliot Carver | Henry Gupta | worksfor |
| Elliot Carver |  Gotz Otto  | worksfor |
+---------------+-------------+----------+
[5 rows x 3 columns]
```

The
[get_neighborhood](https://apple.github.io/turicreate/docs/api/generated/turicreate.SGraph.get_neighborhood.html#turicreate.SGraph.get_neighborhood)
method provides a convenient way to retrieve the subset of a graph near
a set of target vertices, also known as the *egocentric neighborhood* of
the target vertices. The `radius` of the neighborhood is the maximum
length of a path between any of the targets and a neighborhood vertex.
If `full_subgraph` is true, then edges between neighborhood vertices are
included even if the edges are not on direct paths between a target and
a neighbor.

```python
targets = ['James Bond', 'Moneypenny']
subgraph = g.get_neighborhood(ids=targets, radius=1, full_subgraph=True)
```
![Bond neighborhood](images/bond_neighborhood.png)



#### Modifying SGraphs

SGraphs are *structurally immutable*, but the data stored on vertices and edges
can be mutated using two special SGraph properties. `SGraph.vertices` and
`SGraph.edges` are SFrames containing the vertex and edge data, respectively.
The following examples show the difference between the special graph-related
SFrames and normal SFrames. First, note that the following lines both produce
the same effect.

```python
g.edges.print_rows(5)
g.get_edges().print_rows(5)
```
```no-highlight
+----------------+----------------+------------+
|    __src_id    |    __dst_id    |  relation  |
+----------------+----------------+------------+
|   Moneypenny   |       M        | managed_by |
| Inga Bergstorm |   James Bond   |   friend   |
|   Moneypenny   |       Q        | colleague  |
|  Henry Gupta   | Elliot Carver  | killed_by  |
|   James Bond   | Inga Bergstorm |   friend   |
+----------------+----------------+------------+
[5 rows x 3 columns]
```

The difference is that the return value of `g.get_edges()` is a normal SFrame
independent from `g`, whereas `g.edges` is bound to `g`. We can modify the edge
data using this special edge SFrame. The next snippet mutates the relation
attribute on the edges of `g`. In particular, it extracts the first letter and
converts it to upper case.

```python
g.edges['relation'] = g.edges['relation'].apply(lambda x: x[0].upper())
g.get_edges().print_rows(5)
```
```no-highlight
+----------------+----------------+----------+
|    __src_id    |    __dst_id    | relation |
+----------------+----------------+----------+
|   Moneypenny   |       M        |    M     |
| Inga Bergstorm |   James Bond   |    F     |
|   Moneypenny   |       Q        |    C     |
|  Henry Gupta   | Elliot Carver  |    K     |
|   James Bond   | Inga Bergstorm |    F     |
|      ...       |      ...       |   ...    |
+----------------+----------------+----------+
[20 rows x 3 columns]
```

On the other hand, the following code does not mutate the relation attribute on
the edges of `g`. If it had a permanent effect, the relation field would be
converted a lower case letter, but in the result it clearly remains upper case.

```python
e = g.get_edges()  # e is a normal SFrame independent of g.
e['relation'] = e['relation'].apply(lambda x: x[0].lower())
g.get_edges().print_rows(5)
```
```no-highlight
+----------------+----------------+----------+
|    __src_id    |    __dst_id    | relation |
+----------------+----------------+----------+
|   Moneypenny   |       M        |    M     |
| Inga Bergstorm |   James Bond   |    F     |
|   Moneypenny   |       Q        |    C     |
|  Henry Gupta   | Elliot Carver  |    K     |
|   James Bond   | Inga Bergstorm |    F     |
|      ...       |      ...       |   ...    |
+----------------+----------------+----------+
[20 rows x 3 columns]
```

Calling a method like `head()`, `tail()`, or `append()` on a special
graph-related SFrame also results in a new instance of a regular SFrame. For
example, the following code does not mutate `g`.

```python
e = g.edges.head(5)
e['is_friend'] = e['relation'].apply(lambda x: x[0] == 'F')
```

Another important difference of these two special SFrames is that the `__id`,
`__src_id`, and `__dst_id` fields are not mutable because changing them would
change the structure of the graph and SGraph is *structurally immutable*.

Otherwise, `g.vertices` and `g.edges` act like normal SFrames, which makes
modifying graph data very easy. For example, adding (removing) an edge
field is the same as adding (removing) a column to (from) an SFrame:

```python
g.edges['weight'] = 1.0
del g.edges['weight']
```

The
[triple_apply](https://apple.github.io/turicreate/docs/api/generated/turicreate.SGraph.triple_apply.html#turicreate.SGraph.triple_apply)
method provides a particularly powerful way to modify SGraph
vertex and edge attributes. `triple_apply` applies a user-defined function to
all edges asynchronously, allowing you to do a computation that modifies edge
data based on vertex data, or vice versa. A wide range of
methods---[single-source shortest
path](http://en.wikipedia.org/wiki/Shortest_path_problem) and [weighted
PageRank](http://en.wikipedia.org/wiki/PageRank), for example---can be expressed
very simply with this primitive.

The first step is to define a function that takes as input an edge in the graph,
together with the incident source and destination vertices. This *triple apply
function* modifies vertex and edge fields in some way, then returns the modified
(source vertex, edge, destination vertex) triple. In this example, we compute
the *degree* of each vertex in the James Bond graph, which is the number of
edges that touch each vertex.

```python
def increment_degree(src, edge, dst):
    src['degree'] += 1
    dst['degree'] += 1
    return (src, edge, dst)
```

The next step is to create a new field in our SGraph's vertex data to hold the
answer.

```python
g.vertices['degree'] = 0
```

Finally, we use the `triple_apply` method to apply the function to all of the
edges (together with their incident source and destination vertices). This
method requires specification of which fields are allowed to be changed by the
our function.

```python
g = g.triple_apply(increment_degree, mutated_fields=['degree'])
print g.vertices.sort('degree', ascending=False)
```
```no-highlight
+----------------+--------+--------+-----------------+---------+
|      __id      | degree | gender | license_to_kill | villain |
+----------------+--------+--------+-----------------+---------+
|   James Bond   |   8    |   M    |        1        |    0    |
| Elliot Carver  |   7    |   M    |        0        |    1    |
|       M        |   6    |   M    |        1        |    0    |
|   Moneypenny   |   4    |   F    |        1        |    0    |
|       Q        |   4    |   M    |        1        |    0    |
|  Paris Carver  |   3    |   F    |        0        |    1    |
| Inga Bergstorm |   2    |   F    |        0        |    0    |
|  Henry Gupta   |   2    |   M    |        0        |    1    |
|    Wai Lin     |   2    |   F    |        1        |    0    |
|   Gotz Otto    |   2    |   M    |        0        |    1    |
+----------------+--------+--------+-----------------+---------+
[10 rows x 5 columns]
```

James Bond is quite the popular guy!

To learn more, check out the [graph analytics toolkits](../graph_analytics/README.md), the
[API Reference](https://apple.github.io/turicreate/docs/api/generated/turicreate.SGraph.html)
for SGraphs.
