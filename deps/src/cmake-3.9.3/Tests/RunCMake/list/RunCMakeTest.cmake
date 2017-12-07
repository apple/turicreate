include(RunCMake)

run_cmake(EmptyFilterRegex)
run_cmake(EmptyGet0)
run_cmake(EmptyRemoveAt0)
run_cmake(EmptyInsert-1)

run_cmake(NoArguments)
run_cmake(InvalidSubcommand)
run_cmake(GET-CMP0007-WARN)

run_cmake(FILTER-REGEX-InvalidRegex)
run_cmake(GET-InvalidIndex)
run_cmake(INSERT-InvalidIndex)
run_cmake(REMOVE_AT-InvalidIndex)

run_cmake(FILTER-REGEX-TooManyArguments)
run_cmake(LENGTH-TooManyArguments)
run_cmake(REMOVE_DUPLICATES-TooManyArguments)
run_cmake(REVERSE-TooManyArguments)
run_cmake(SORT-TooManyArguments)

run_cmake(FILTER-NotList)
run_cmake(REMOVE_AT-NotList)
run_cmake(REMOVE_DUPLICATES-NotList)
run_cmake(REMOVE_ITEM-NotList)
run_cmake(REVERSE-NotList)
run_cmake(SORT-NotList)

run_cmake(FILTER-REGEX-InvalidMode)
run_cmake(FILTER-REGEX-InvalidOperator)
run_cmake(FILTER-REGEX-Valid0)
run_cmake(FILTER-REGEX-Valid1)
