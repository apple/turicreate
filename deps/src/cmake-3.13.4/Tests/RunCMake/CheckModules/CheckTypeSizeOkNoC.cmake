enable_language(CXX)
include(CheckTypeSize)
check_type_size(int SIZEOF_INT LANGUAGE CXX)
check_type_size(int SIZEOF_INT BUILTIN_TYPES_ONLY LANGUAGE CXX)
