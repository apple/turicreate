include(CheckStructHasMember)
check_struct_has_member("struct timeval" tv_sec sys/select.h HAVE_TIMEVAL_TV_SEC_K LANGUAGE FORTRAN)
