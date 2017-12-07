file(STRINGS UTF-16BE.txt str ENCODING UTF-16BE LENGTH_MINIMUM 4)
message("${str}")
file(STRINGS UTF-16BE.txt str LENGTH_MINIMUM 4)
message("${str}")
