# wrong argument count
cmake_parse_arguments()
cmake_parse_arguments(prefix OPT)
cmake_parse_arguments(prefix OPT SINGLE)
cmake_parse_arguments(prefix OPT SINGLE MULTI) # not an error

# duplicate keywords
cmake_parse_arguments(prefix "OPT;OPT" "" "")
cmake_parse_arguments(prefix "" "OPT;OPT" "")
cmake_parse_arguments(prefix "" "" "OPT;OPT")

cmake_parse_arguments(prefix "OPT" "OPT" "")
cmake_parse_arguments(prefix "" "OPT" "OPT")
cmake_parse_arguments(prefix "OPT" "" "OPT")
