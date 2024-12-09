clean:
	rm -rf build CMakeUserPresets.json

install-deps:
	conan install . --build=missing --settings=build_type=Debug

cmake:
	cmake --preset conan-debug

build: cmake
	cmake --build --preset conan-debug

run: build
	./build/Debug/src/server/structuredb-server

run-cli:
	./build/Debug/src/cli/structuredb-cli

tests: build
	cd ./build/Debug && ctest
