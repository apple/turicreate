set(ENV{SOURCE_DATE_EPOCH} "invalid-integer")
string(TIMESTAMP RESULT "%Y-%m-%d %H:%M:%S" UTC)
message("RESULT=${RESULT}")
