include(FetchContent)

FetchContent_Declare(
  t1
  DOWNLOAD_COMMAND ${CMAKE_COMMAND} -E echo "Download command executed"
)

FetchContent_Populate(t1)
FetchContent_Populate(t1) # Triggers error
