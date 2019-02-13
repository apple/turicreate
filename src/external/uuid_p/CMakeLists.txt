project(uuid)

add_library(uuid OBJECT
  clear.c compare.c copy.c gen_uuid.c isnull.c pack.c parse.c unpack.c unparse.c uuid_time.c randutils.c
)

target_compile_options(uuid PRIVATE "-w")

target_compile_definitions(uuid PRIVATE -DHAVE_CONFIG_H)
