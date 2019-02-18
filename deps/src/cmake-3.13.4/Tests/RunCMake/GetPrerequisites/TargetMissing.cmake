include(GetPrerequisites)
set(result_var value before call)
get_prerequisites(does_not_exist result_var 0 0 "" "")
message("result_var='${result_var}'")
