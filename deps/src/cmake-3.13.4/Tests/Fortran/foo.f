      module test_mod
      interface dummy
         module procedure sub
      end interface
      contains
        subroutine sub
        end subroutine

      end module test_mod
