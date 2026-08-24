#!/usr/bin/env bash

# THIS IS AN AI GENERATED SCRIPT
# I HATE BASH

set -euo pipefail

project_dir="$(cd "$(dirname "$0")" && pwd)"
build_dir="${SHOWCASE_BUILD_DIR:-$project_dir/build}"
api_origin="${SHOWCASE_API_ORIGIN:-https://showcase-api.flafy.dev}"
# api_origin="${SHOWCASE_API_ORIGIN:-http://127.0.0.1:18080}"
host_cc="$(command -v cc)"
host_cxx="$(command -v c++)"
ninja="$(command -v ninja)"
toolchain_file="$TOOLCHAIN_REPO/clang-cl-msvc.cmake"
configure=true

if (($# > 1)) || { (($# == 1)) && [[ "$1" != --no-cache ]]; }; then
    echo "usage: $0 [--no-cache]" >&2
    exit 2
fi

config_stamp="$build_dir/.showcase-config"
config_fingerprint="$(printf '%s\n' \
    "api_origin=$api_origin" \
    "cmake=$(command -v cmake)" \
    "host_cc=$host_cc" \
    "host_cxx=$host_cxx" \
    "ninja=$ninja" \
    "toolchain_file=$toolchain_file" \
    "geode=$(command -v geode)")"

if [[ "${SHOWCASE_RECONFIGURE:-0}" != 1 && "${1:-}" != --no-cache \
    && -f "$build_dir/build.ninja" && -f "$config_stamp" \
    && "$(<"$config_stamp")" == "$config_fingerprint" ]]; then
    configure=false
fi

if [[ "$configure" == true ]]; then
    cmake -S "$project_dir" -B "$build_dir" -G Ninja \
        -DCMAKE_BUILD_TYPE=RelWithDebInfo \
        -DSKIP_BUILDING_CODEGEN=OFF \
        "-DCROSS_TOOLCHAIN_FLAGS_NATIVE=-DHEADLESSGD_NATIVE_TOOLCHAIN=ON;-DCMAKE_C_COMPILER=$host_cc;-DCMAKE_CXX_COMPILER=$host_cxx" \
        -DCMAKE_TRY_COMPILE_CONFIGURATION=Release \
        -DGEODE_DONT_INSTALL_MODS=ON \
        "-DSHOWCASE_API_ORIGIN=$api_origin" \
        -DCMAKE_MAKE_PROGRAM="$ninja" \
        -DCMAKE_TOOLCHAIN_FILE="$toolchain_file"
    printf '%s\n' "$config_fingerprint" > "$config_stamp"
fi
cmake --build "$build_dir"

install_dir="${SHOWCASE_INSTALL_DIR:-}"
if [[ -z "$install_dir" && -d "$project_dir/../../local/game/geode/mods" ]]; then
    install_dir="$project_dir/../../local/game/geode/mods"
fi
if [[ -n "$install_dir" ]]; then
    install -m 0644 "$build_dir/flafy.showcase.geode" "$install_dir/flafy.showcase.geode"
fi
