# Remove the log file.
file(REMOVE ${Log})

# Run a child that sleeps longer than the timout of this test.
# Log its output so check.cmake can verify it dies.
execute_process(COMMAND ${Sleep} ERROR_FILE ${Log})
