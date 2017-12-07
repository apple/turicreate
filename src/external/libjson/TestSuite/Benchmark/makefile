OS=$(shell uname)
ifeq ($(OS), Darwin)
	fastflag = -fast
else
	fastflag = -O3
endif

single:
	g++ main.cpp \
	../../Source/internalJSONNode.cpp \
	../../Source/JSONChildren.cpp ../../Source/JSONDebug.cpp \
	../../Source/JSONIterators.cpp ../../Source/JSONMemory.cpp \
	../../Source/JSONNode_Mutex.cpp ../../Source/JSONNode.cpp \
	../../Source/JSONWorker.cpp ../../Source/JSONWriter.cpp \
	../../Source/libjson.cpp ../../Source/JSONValidator.cpp \
	../../Source/JSONStream.cpp ../../Source/JSONAllocator.cpp \
    ../../Source/JSONPreparse.cpp \
     -Wfatal-errors -DNDEBUG $(fastflag) -ffast-math -fexpensive-optimizations -o testapp

debug:
	g++ main.cpp \
	../../Source/internalJSONNode.cpp \
	../../Source/JSONChildren.cpp ../../Source/JSONDebug.cpp \
	../../Source/JSONIterators.cpp ../../Source/JSONMemory.cpp \
	../../Source/JSONNode_Mutex.cpp ../../Source/JSONNode.cpp \
	../../Source/JSONWorker.cpp ../../Source/JSONWriter.cpp \
	../../Source/libjson.cpp ../../Source/JSONValidator.cpp \
	../../Source/JSONStream.cpp ../../Source/JSONAllocator.cpp \
    ../../Source/JSONPreparse.cpp \
     -Wfatal-errors -DJSON_DEBUG -o testapp

small:
	g++ main.cpp \
	../../Source/internalJSONNode.cpp \
	../../Source/JSONChildren.cpp ../../Source/JSONDebug.cpp \
	../../Source/JSONIterators.cpp ../../Source/JSONMemory.cpp \
	../../Source/JSONNode_Mutex.cpp ../../Source/JSONNode.cpp \
	../../Source/JSONWorker.cpp ../../Source/JSONWriter.cpp \
	../../Source/libjson.cpp ../../Source/JSONValidator.cpp \
	../../Source/JSONStream.cpp ../../Source/JSONAllocator.cpp \
    ../../Source/JSONPreparse.cpp \
     -Wfatal-errors -DNDEBUG -Os -ffast-math -DJSON_LESS_MEMORY -o testapp
