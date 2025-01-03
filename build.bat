@echo off

:: -mwindows : Compile a Windows executable, no cmd window
set PROD_FLAGS=-O2 -mwindows -s
set DEV_FLAGS=-g -ggdb
set CFLAGS=-Wall -Wextra -Wimplicit -Wpedantic -Wno-unused-function -Wno-unused-variable -std=c11

:: Compiler-Options to include dependencies
set LIB_PATHS=-L./bin
set INCLUDES=-I./deps/raylib/src -I./deps/ail
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
	xcopy assets\ bin\assets\ /E /Q
	:: Build raylib
	cd deps/raylib/src
	make PLATFORM=PLATFORM_DESKTOP RAYLIB_LIBTYPE=STATIC RAYLIB_RELEASE_PATH=../../../bin RAYLIB_BUILD_MODE=RELEASE
	cd ../../..
)


:: Build executable
cmd /c if exist bin\VectorFields.exe del /F bin\VectorFields.exe
set DEV=1
if "%~1"=="r" set DEV=0
if "%~1"=="-r" set DEV=0
if defined DEV (
	set CFLAGS=%CFLAGS% %DEV_FLAGS%
) else (
	set CFLAGS=%CFLAGS% %PROD_FLAGS%
)

@echo on
gcc %CFLAGS% -o bin/VectorFields src/main.c src/helpers.c src/ir.c %DEPS%
@echo off
