! Test the notation for an Nth-generation descendant
! for N>1, which necessitates the colon.
submodule ( parent : child ) grandchild
contains
  module subroutine grandchild_subroutine()
    print *,"Test passed."
  end subroutine
end submodule grandchild
