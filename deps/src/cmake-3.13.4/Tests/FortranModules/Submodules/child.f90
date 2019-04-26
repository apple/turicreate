! Test the notation for a 1st-generation direct
! descendant of a parent module
SUBMODULE ( parent ) child
  implicit none
CONTAINS
  module function child_function() result(child_stuff)
    logical :: child_stuff
    child_stuff=.true.
  end function
END SUBMODULE child
