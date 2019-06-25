# SFrame Query Operator Design Doc

The new SFrame Query Engine design doc is to allow for greater performance
optimizations by increasing abstraction of data representation from computation,
and to set the path for going distributed.

The basic design follows most query execution design principles, we have a
logical query graph, a query optimizer that translates the logical query graph
to a physical plan, and a physical plan executor that runs the plan.

## Operator
All operators are implemented in operator/
New operators have to be added to the enumeration operator_types in
operator_properties.hpp and operator_properties.cpp.

Every operator has 2 components, a logical component which aids in planning,
and a physical component which just performs execution. An operator may or may
not take other operators as inputs.

### Physical Operator
The physical component of the operator is implemented as a class which inherits
from the query_operator base class.  (For a simple example, see
operators/transform.hpp)

To create a new operator, add to the enum in operator_properties and define
your new new operator as a class operator_impl<enum> (the operator itself
is just a specialization of the operator_impl class around the enum)
The functions which must be implemented are
 - attributes(): describes the attributes of the operator.
 - name(): a printable name of the operator
 - execute(): which reads elements from its inputs, and generates output
Optionally:
 - print(): Makes a human readable description of the operator.
            Defaults to just name()

### Logical Operator
The logical graph is a graph of shared_ptr<planner_node> objects, and hence
the logical component for each operator is simply described by different
values in the planner_node object.

The planner node is a simple datastructure comprising of just a few
elements:
 - *operator_type* One of the operator enumerations in operator_properties.hpp
 - *operator_parameters* (map of string->flexible_type>: The parameters for
 the operator. This is operator
 dependent and is defined by the operator itself. Generally, users of
 the planner node should not need this, and should just call

 operator_impl<enum>::make_planner_node() to create an operator node
 - *any_operator_parameters* (map of string->any): Non-portable parameters.
 Operators which use this will generally not work for going distributed.
 - *inputs* (vector of shared_ptr<planner_node>) Inputs to the operators
 are defined here.

To define a logical operator you must add to the operator_type enumeration
and define a specialization of the planner_node_traits class
(once again, see operators/transform.hpp for a simple example) which describes

A constructor for the the logical node:
    operator_impl<enum>::make_planner_node(...)

A conversion from the logical node to the physical operator:
    operator_impl<enum>::plan_to_operator(...)

A type inference routine:
    operator_impl<enum>::infer_type(...)

A length inference routine:
    operator_impl<enum>::infer_length(...)

### Logical Operator Invariances

The logical operator graph is simple and is just a shared_ptr of planner_node
objects, each of which just contains a bunch of runtime attributes. This allows
the planner graph to be easily modified. However, it is important that all
modifications obey certain invariances for sanity.

Assuming I have a function
   mutate(std::shared_ptr<planner_node> X)

The invariant any planner graph modification functions have to maintain is:
 - If $ X $ is a shared_ptr to a planner_node which exists in the input graph
 - Then if $ X $ appears in the graph after mutation, it *must* maintain
 equivalent semantics as $ X $ in the input graph. In other words: the result
 of materialization of $ X $ before calling mutate must be exactly equivalent
 to the result of materialization of $ X $ after calling mutate.

Within these constraints allow for certain capabilities to be well defined.
For instance, given a graph of SFrame.

   S1-->S2-->S3-->S4

If S3 is ever materialized, it could be rewritten inplace become an SFrame
source node thus spliting the graph:

   S1-->S2  S3-->S4

Also, caching of certain attributes of the data (like length, materialized
information, etc) becomes well defined.

### Other notes
Note that not every logical operator needs to have a physical equivalent (the
query optimizer or executor can eliminate the operators or make new ones),
but every physical operator must have a logical equivalent.

For convenience, operators may be typedefed to simpler names. For instance,

    typedef operator_impl<TRANSFORM_NODE> op_transform;

## Other Utilities

Performs type inference on a logical node:

    infer_planner_node_type(std::shared_ptr<planner_node>)

Performs length inference on a logical node:

    infer_planner_node_length(std::shared_ptr<planner_node>)

Get a printable node name from the enum, or vice versa.

    planner_node_name_to_type and planner_node_type_to_name


## SArray / Sframe execution process
unity_sframe / unity_sarray are the outward facing wrappers of the SFrame and
SArray. These classes will each maintain:
 - A schema (types and column names)
 - Length of the SFrame / Sarray if known
 - A shared_ptr<planner_node> for lazy evaluation.
    - even if the SFrame/SArray is already materialized, this should
      contain an sframe_source or an sarray_source node.

To materialize an SFrame, the following is necessary:
  sf = query_planner().materialize(std::shared_ptr<planner_node>)

## Query Planner
The query planner class performs 2 stages:

### Query Optimization

First stage is a query optimization pass, it rewrites the planner_node graph
to optimize for generating the desired final output. It can rewrite whatever
component of the graph as it needs as long as it ultimately generates the
desired output. See Logical Operator Invariances for details on the allowed
transformations.

### Query Execution

The output graph is then partitioned into sections to be executed by the
subplan_executor.

The subplan executor takes a vector<shared_ptr<planner_node> > and returns an
sframe which is the result of the concatenation of executing each of the plans.

## Global Lock
Planner nodes are ... not very parallel.
Many different objects (like unity_sframe / unity_sarray) contain planner_nodes,
but the planner_nodes reference each other to get things done.
So adding locks to the higher level objects (unity_sframe / unity_sarray) do
not keep things safe.

A better solution could be introduced here, but for now a global lock will
resolve all issues.
