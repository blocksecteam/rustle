.PHONY: pass clean

pass: $(patsubst %.cpp, %.so, $(filter-out near_core.cpp, $(wildcard *.cpp)))

%.so: %.cpp near_core.cpp near_core.h
	$(LLVM_CLANG) ${CXXFLAGS} $< near_core.cpp -o $@ ${LDFLAGS}

clean:
	rm -rf *.so
