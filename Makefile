.PHONY: all hello trace simplify metric judge check clean
all:
	cmake -S . -B build
	cmake --build build --parallel
hello trace simplify metric judge check:
	cmake -S . -B build
	cmake --build build --target $@ --parallel
	./build/$@ $(ARGS)
clean:
	cmake --build build --target clean
