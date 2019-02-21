# The program units in this file consist of a module/submodule
# tree represented by the following graph:
#
#        parent
#          |
#         / \
#        /   \
#    child   sibling
#      |
#  grandchild
#      |
#  GreatGrandChild
#
# where the parent node is a module and all other nodes are submodules.

add_executable(submod
  main.f90
  parent.f90
  child.f90
  grandchild.f90
  greatgrandchild.f90
  sibling.f90
  )
