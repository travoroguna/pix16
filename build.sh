#!/bin/bash

set -e # exit on error

# Config
project_root="$(cd "$(dirname "$0")" && pwd -P)"
exe_name="pix16_debug"

pushd $project_root
    #build_hash=$(more "$project_root/.git/refs/heads/master")
    build_hash=""

    mkdir -p build

    pushd build
        flags="-std=c++11 -Wno-deprecated-declarations -Wno-int-to-void-pointer-cast -Wno-writable-strings -Wno-dangling-else -Wno-switch -Wno-undefined-internal -Wno-logical-op-parentheses"
        libs="-framework Cocoa -framework OpenGL"
        time g++ -DDEBUG=1 -DBUILD_HASH=$build_hash -I ../src $flags $libs ../src/pix16_win32.cpp -o $exe_name

        ./$exe_name
    popd
popd