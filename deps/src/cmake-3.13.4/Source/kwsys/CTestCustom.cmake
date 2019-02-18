# kwsys.testProcess-10 involves sending SIGINT to a child process, which then
# exits abnormally via a call to _exit(). (On Windows, a call to ExitProcess).
# Naturally, this results in plenty of memory being "leaked" by this child
# process - the memory check results are not meaningful in this case.
#
# kwsys.testProcess-9 also tests sending SIGINT to a child process.  However,
# normal operation of that test involves the child process timing out, and the
# host process kills (SIGKILL) it as a result.  Since it was SIGKILL'ed, the
# resulting memory leaks are not logged by valgrind anyway.  Therefore, we
# don't have to exclude it.

list(APPEND CTEST_CUSTOM_MEMCHECK_IGNORE
  kwsys.testProcess-10
  )
