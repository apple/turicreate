project(uuid)

make_library(uuid OBJECT
  SOURCES
  clear.c compare.c copy.c gen_uuid.c isnull.c pack.c parse.c unpack.c unparse.c uuid_time.c randutils.c
)

target_compile_definitions(uuid PRIVATE -DHAVE_CONFIG_H)
