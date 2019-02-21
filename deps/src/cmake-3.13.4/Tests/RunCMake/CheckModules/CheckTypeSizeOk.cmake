enable_language(C)
enable_language(CXX)
include(CheckTypeSize)
check_type_size(int SIZEOF_INT)
check_type_size(int SIZEOF_INT BUILTIN_TYPES_ONLY)
check_type_size(int SIZEOF_INT LANGUAGE C)
check_type_size(int SIZEOF_INT LANGUAGE CXX)
check_type_size(int SIZEOF_INT BUILTIN_TYPES_ONLY LANGUAGE C)

# Weird but ok... only last value is considered
check_type_size(int SIZEOF_INT BUILTIN_TYPES_ONLY BUILTIN_TYPES_ONLY)
check_type_size(int SIZEOF_INT LANGUAGE C LANGUAGE CXX)
