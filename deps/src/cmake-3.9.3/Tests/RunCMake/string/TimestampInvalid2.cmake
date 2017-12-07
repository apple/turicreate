set(ENV{SOURCE_DATE_EPOCH} "123trailing-garbage")
string(TIMESTAMP RESULT "%Y-%m-%d %H:%M:%S" UTC)
message("RESULT=${RESULT}")
