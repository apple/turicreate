# In some projects, people don't use TARGET_LINK_LIBRARIES, but just
# use an all-encompassing LINK_LIBRARIES. And sometimes they don't
# specify them in the correct order.

link_libraries( Two )
link_libraries( Five )

add_library( SixA SixASrc.c )

add_library( SixB SixBSrc.c )
target_link_libraries( SixB Four )

