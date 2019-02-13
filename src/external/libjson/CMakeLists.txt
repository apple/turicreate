project(TuriExternalDependencies)

add_library(libjson OBJECT
  Source/internalJSONNode.cpp
  Source/JSONNode_Mutex.cpp
  Source/JSONStream.cpp
  Source/JSONNode.cpp
  Source/JSONWriter.cpp
  Source/JSONChildren.cpp
  Source/JSONDebug.cpp
  Source/JSONMemory.cpp
  Source/JSONValidator.cpp
  Source/libjson.cpp
  Source/JSONPreparse.cpp
  Source/JSONWorker.cpp
  Source/JSONAllocator.cpp
  Source/JSONIterators.cpp
)

target_compile_options(libjson PRIVATE -w)