PROD_FLAGS="-O2 -s"
DEV_FLAGS="-g -ggdb"
CFLAGS="-Wall -Wextra -Wimplicit -Wpedantic -Wno-unused-function -Wno-unused-variable -std=c11"

LIB_PATHS="-L./bin"
INCLUDES="-I./deps/raylib/src -I./deps/ail"
RAYLIB_DEP="-lraylib -lm"
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
if [[ $1 == "r" ]] || [[ $1 == "-r" ]]; then
	CFLAGS="$CFLAGS $PROD_FLAGS"
else
	CFLAGS="$CFLAGS $DEV_FLAGS"
fi

set -xe
gcc $CFLAGS -o bin/VectorFields src/helpers.c src/ir.c src/main.c $DEPS
