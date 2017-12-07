
LIB_FLAGS = -larmadillo
#LIB_FLAGS = -lblas -llapack
#LIB_FLAGS = -lopenblas -llapack

CXX_FLAGS = -std=c++11 -Wshadow -Wall -pedantic -O0
#CXX_FLAGS = -std=c++11 -Wshadow -Wall -pedantic -O0 -fopenmp
#CXX_FLAGS = -std=c++11 -Wshadow -Wall -pedantic -O0 -DARMA_DONT_USE_WRAPPER
#CXX_FLAGS = -std=c++11 -Wshadow -Wall -pedantic -O2

OBJECTS = $(patsubst %.cpp,%.o,$(wildcard *.cpp))

%.o: %.cpp $(DEPS)
	$(CXX) $(CXX_FLAGS) -o $@ -c $<

main: $(OBJECTS)
	$(CXX) $(CXX_FLAGS) -o $@ $(OBJECTS) $(LIB_FLAGS)


all: main

.PHONY: clean

clean:
	rm -f main *.o
