clean:
	rm -rf build CMakeUserPresets.json

install-deps:
	conan install conanfile.txt --build=missing

cmake:
	cmake --preset conan-release

build: cmake
	cmake --build --preset conan-release

run: build
	./build/Release/src/server/structuredb-server
