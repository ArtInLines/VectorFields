@echo off

:: -mwindows : Compile a Windows executable, no cmd window
set PROD_FLAGS=-O2 -mwindows -s
set DEV_FLAGS=-g -ggdb
set CFLAGS=-Wall -Wextra -Wimplicit -Wpedantic -Wno-unused-function -std=c99

:: Compiler-Options to include dependencies
set LIB_PATHS=-L./bin
set INCLUDES=-I./deps/raylib/src
set RAYLIB_DEP=-lraylib -lopengl32 -lgdi32 -lwinmm -lpthread
set DEPS=%INCLUDES% %LIB_PATHS% %RAYLIB_DEP%

:: Build all dependencies
set BUILD_ALL=
if "%~1"=="a" set BUILD_ALL=1
if not exist bin set BUILD_ALL=1
if defined BUILD_ALL (
	:: Remove old bin folder
	if exist bin rmdir bin /S /Q
	mkdir bin
	:: Build raylib
	cd deps/raylib/src
	make PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=SHARED RAYLIB_RELEASE_PATH=../../../bin RAYLIB_BUILD_MODE=DEBUG
	cd ../../..
)


:: Build executable
cmd /c if exist bin\main.exe del /F bin\main.exe
gcc %CFLAGS% %PROD_FLAGS% -o bin/main src/main.c %DEPS%