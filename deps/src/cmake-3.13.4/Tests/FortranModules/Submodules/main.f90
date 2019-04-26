program main
  use parent, only : child_function,grandchild_subroutine
  use parent, only : sibling_function,GreatGrandChild_subroutine
  implicit none
  if (child_function()) call grandchild_subroutine
  if (sibling_function()) call GreatGrandChild_subroutine
end program
