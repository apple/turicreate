program main
  use parent, only : child_function,grandchild_subroutine
  implicit none
  if (child_function()) call grandchild_subroutine
end program
