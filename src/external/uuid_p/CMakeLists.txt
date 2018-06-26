project(uuid)

make_library(uuid
  SOURCES
  clear.c compare.c copy.c gen_uuid.c isnull.c pack.c parse.c unpack.c unparse.c uuid_time.c randutils.c
  COMPILE_FLAGS_EXTRA_GCC
    -Wno-implicit-function-declaration
  COMPILE_FLAGS_EXTRA_CLANG
    -Wno-format
)

target_compile_definitions(uuid PRIVATE -DHAVE_CONFIG_H)
