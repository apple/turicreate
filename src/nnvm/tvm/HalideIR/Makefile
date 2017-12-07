LDFLAGS = -pthread -lm
CFLAGS =  -std=c++11 -Wall -O2\
	 -Iinclude -Idmlc-core/include -Isrc -fPIC

ifdef no_rtti
	CFLAGS += -fno-rtti
endif

# specify tensor path
.PHONY: clean all test doc

CCSUFFIX=cpp

all: lib/libHalideIR.a lib/libHalideIR.so
SRC = $(wildcard src/*.$(CCSUFFIX) src/*/*.$(CCSUFFIX))
ALL_OBJ = $(patsubst src/%.$(CCSUFFIX), build/%.o, $(SRC))
ALL_DEP = $(ALL_OBJ)


build/%.o: src/%.$(CCSUFFIX)
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) -MM -MT build/$*.o $< >build/$*.d
	$(CXX) -c $(CFLAGS) -c $< -o $@


lib/libHalideIR.a: $(ALL_DEP)
	@mkdir -p $(@D)
	ar crv $@ $(filter %.o, $?)

lib/libHalideIR.so: $(ALL_DEP)
	@mkdir -p $(@D)
	$(CXX) $(CFLAGS) -shared -o $@ $(filter %.o %.a, $^) $(LDFLAGS)


lint:
	python2 dmlc-core/scripts/lint.py tvm cpp include src/tvm

doc:
	doxygen docs/Doxyfile

clean:
	$(RM) -rf build lib bin *~ */*~ */*/*~ */*/*/*~ */*.o */*/*.o */*/*/*.o */*.d */*/*.d */*/*/*.d

-include build/*.d
-include build/*/*.d
