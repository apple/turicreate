build_command(MAKECOMMAND_DEFAULT_VALUE)
message(AUTHOR_WARNING "build_command() returned:\n ${MAKECOMMAND_DEFAULT_VALUE}")
project(${RunCMake_TEST} NONE)
