include(RunCMake)

run_cmake(defaultmessage)
run_cmake(nomessage)
run_cmake(message-internal-warning)
run_cmake(nomessage-internal-warning)
run_cmake(warnmessage)
# message command sets fatal occurred flag, so check each type of error

# seperately
run_cmake(errormessage_deprecated)
run_cmake(errormessage_dev)
