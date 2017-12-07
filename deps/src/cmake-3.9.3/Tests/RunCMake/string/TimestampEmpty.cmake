set(ENV{SOURCE_DATE_EPOCH} "")
string(TIMESTAMP RESULT "%Y-%m-%d %H:%M:%S" UTC)
message("RESULT=${RESULT}")
