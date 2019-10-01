if(NOT EXISTS "${Data}")
  message(SEND_ERROR "Input file:\n  ${Data}\ndoes not exist!")
endif()
if(NOT EXISTS "${Other}")
  message(SEND_ERROR "Input file:\n  ${Other}\ndoes not exist!")
endif()
# Verify that the 'Data' object was found in the second store location left
# from Data1 target downloads and that the 'Other' object was found in the
# first store location left from Data3 target downloads.  Neither object
# should exist in the opposite store.
foreach(should_exist
    "${Store0}/MD5/aaad162b85f60d1eb57ca71a23e8efd7"
    "${Store1}/MD5/8c018830e3efa5caf3c7415028335a57"
    )
  if(NOT EXISTS ${should_exist})
    message(SEND_ERROR "Store file:\n  ${should_exist}\nshould exist!")
  endif()
endforeach()
foreach(should_not_exist
    "${Store0}/MD5/8c018830e3efa5caf3c7415028335a57"
    "${Store1}/MD5/aaad162b85f60d1eb57ca71a23e8efd7"
    )
  if(EXISTS ${should_not_exist})
    message(SEND_ERROR "Store file:\n  ${should_not_exist}\nshould not exist!")
  endif()
endforeach()
