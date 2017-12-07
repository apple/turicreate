MODULE Available
! no conent
END MODULE

PROGRAM PPTEST
! value of InPPFalseBranch ; values of SkipToEnd
! 0 <empty>
#ifndef FOO
  ! 1 ; <0>
  USE NotAvailable
# ifndef FOO
    ! 2 ; <0,0>
    USE NotAvailable
# else
    ! 2 ; <0,0>
    USE NotAvailable
# endif
  ! 1 ; <0>
# ifdef FOO
    ! 2 ; <0,1>
    USE NotAvailable
# else
    ! 2 ; <0,1>
    USE NotAvailable
# endif
  ! 1 ; <0>
#else
  ! 0 ; <0>
  USE Available
# ifndef FOO
    ! 1 ; <0,0>
    USE NotAvailable
# else
    ! 0 ; <0,0>
    USE Available
# endif
  ! 0 ; <0>
# ifdef FOO
    ! 0 ; <0,1>
    USE Available
# else
    ! 1 ; <0,1>
    USE NotAvailable
# endif
  ! 0 ; <0>
#endif
! 0 ; <empty>

USE PPAvailable

#include "test_preprocess.h"

END PROGRAM
