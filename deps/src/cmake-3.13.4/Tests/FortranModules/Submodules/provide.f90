! The program units in this file consist of a
! module/submodule tree represented by the following
! graph:
!
!        parent
!          |
!         / \
!        /   \
!    child   sibling
!      |
!  grandchild
!
! where the parent node is a module and all other
! nodes are submodules.

module parent
  implicit none

  interface

    ! Test Fortran 2008 "module function" syntax
    module function child_function() result(child_stuff)
      logical :: child_stuff
    end function

    ! Test Fortran 2008 "module subroutine" syntax
    module subroutine grandchild_subroutine()
    end subroutine

  end interface

end module parent

! Test the notation for a 1st-generation direct
! descendant of a parent module
submodule ( parent ) child
  implicit none
contains
  module function child_function() result(child_stuff)
    logical :: child_stuff
    child_stuff=.true.
  end function
end submodule child

! Empty submodule for checking disambiguation of
! nodes at the same vertical level in the tree
submodule ( parent ) sibling
end submodule sibling

! Test the notation for an Nth-generation descendant
! for N>1, which necessitates the colon.
submodule ( parent : child ) grandchild
contains
  module subroutine grandchild_subroutine()
    print *,"Test passed."
  end subroutine
end submodule grandchild
