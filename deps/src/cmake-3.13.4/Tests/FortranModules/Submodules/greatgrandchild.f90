! Test the notation for an Nth-generation descendant
! for N>1, which necessitates the colon.
submodule ( parent : grandchild ) GreatGrandChild
contains
  module subroutine GreatGrandChild_subroutine()
    print *,"Test passed."
  end subroutine
end submodule GreatGrandChild
