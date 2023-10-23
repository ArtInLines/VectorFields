PROD_FLAGS="-O2 -s"
DEV_FLAGS="-g -ggdb"
CFLAGS="-Wall -Wextra -Wimplicit -Wpedantic -Wno-unused-function -std=c99"

LIB_PATHS="-L./bin"
INCLUDES="-I./deps/raylib/src -I./deps/ail"
RAYLIB_DEP="-lraylib -lopengl32 -lgdi32 -lwinmm"
DEPS="$INCLUDES $LIB_PATHS $RAYLIB_DEP"

if [[ $1 == "a" ]] || [ ! -d "./bin" ]; then
	# Remove old bin folder
	rm -rf "./bin"
	mkdir "./bin"
	cp -r "./assets" "./bin/assets"
	# Build raylib
	cd deps/raylib/src
	make PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=STATIC RAYLIB_RELEASE_PATH=../../../bin RAYLIB_BUILD_MODE=RELEASE
	cd ../../..
fi

if [ -f "./bin/VectorFields" ]; then
	rm -f "./bin/VectorFields"
fi
if [[ $1 == "d" ]] || [[ $1 == "-d" ]]; then
	CFLAGS="$CFLAGS $DEV_FLAGS"
else
	CFLAGS="$CFLAGS $PROD_FLAGS"
fi
gcc $CFLAGS -o bin/VectorFields src/helpers.c src/ir.c src/main.c $DEPS