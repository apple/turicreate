module test_interface

    interface dummy
        module procedure module_function
    end interface

contains

    subroutine module_function
    end subroutine

end module test_interface