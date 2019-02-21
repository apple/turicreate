set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)


set(PBASE ${CMAKE_CURRENT_LIST_DIR})
set(PARALLEL_SRC
  ${PBASE}/aaa/bbb/item.cpp
  ${PBASE}/aaa/bbb/data.qrc
  ${PBASE}/aaa/item.cpp
  ${PBASE}/aaa/data.qrc

  ${PBASE}/bbb/aaa/item.cpp
  ${PBASE}/bbb/aaa/data.qrc
  ${PBASE}/bbb/item.cpp
  ${PBASE}/bbb/data.qrc

  ${PBASE}/ccc/item.cpp
  ${PBASE}/ccc/data.qrc

  ${PBASE}/item.cpp
  ${PBASE}/data.qrc
  ${PBASE}/main.cpp
)
