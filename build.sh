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
all_platforms=false
no_cache=false

for arg in "$@"; do
    case "$arg" in
        --all) all_platforms=true ;;
        --no-cache) no_cache=true ;;
        *)
            echo "usage: $0 [--all] [--no-cache]" >&2
            exit 2
            ;;
    esac
done

if [[ "$all_platforms" == true ]]; then
    build_type=Release
else
    build_type=RelWithDebInfo
fi

config_stamp="$build_dir/.showcase-config"
config_fingerprint="$(printf '%s\n' \
    "api_origin=$api_origin" \
    "build_type=$build_type" \
    "cmake=$(command -v cmake)" \
    "host_cc=$host_cc" \
    "host_cxx=$host_cxx" \
    "ninja=$ninja" \
    "toolchain_file=$toolchain_file" \
    "geode=$(command -v geode)")"

if [[ "${SHOWCASE_RECONFIGURE:-0}" != 1 && "$no_cache" != true \
    && -f "$build_dir/build.ninja" && -f "$config_stamp" \
    && "$(<"$config_stamp")" == "$config_fingerprint" ]]; then
    configure=false
fi

if [[ "$configure" == true ]]; then
    cmake -S "$project_dir" -B "$build_dir" -G Ninja \
        -DCMAKE_BUILD_TYPE="$build_type" \
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

if [[ "$all_platforms" == true ]]; then
    ndk_dir="${ANDROID_NDK_ROOT:-${ANDROID_NDK_HOME:-}}"
    if [[ -z "$ndk_dir" && -n "${ANDROID_HOME:-}" ]]; then
        ndk_dir="$ANDROID_HOME/ndk-bundle"
    fi
    if [[ ! -f "$ndk_dir/build/cmake/android.toolchain.cmake" ]]; then
        echo "--all requires the Android NDK" >&2
        exit 2
    fi

    for platform in android32 android64; do
        if [[ "$platform" == android32 ]]; then
            abi=armeabi-v7a
        else
            abi=arm64-v8a
        fi

        android_build_dir="$build_dir/$platform"
        cmake -S "$project_dir" -B "$android_build_dir" -G Ninja \
            -DCMAKE_BUILD_TYPE=Release \
            -DANDROID_ABI="$abi" \
            -DANDROID_PLATFORM=android-23 \
            -DGEODE_DONT_INSTALL_MODS=ON \
            "-DSHOWCASE_API_ORIGIN=$api_origin" \
            -DCMAKE_MAKE_PROGRAM="$ninja" \
            -DCMAKE_TOOLCHAIN_FILE="$ndk_dir/build/cmake/android.toolchain.cmake"
        cmake --build "$android_build_dir"
    done

    geode package merge \
        "$build_dir/flafy.showcase.geode" \
        "$build_dir/android32/flafy.showcase.geode" \
        "$build_dir/android64/flafy.showcase.geode"
fi

install_dir="${SHOWCASE_INSTALL_DIR:-}"
if [[ -z "$install_dir" && -d "$project_dir/../../local/game/geode/mods" ]]; then
    install_dir="$project_dir/../../local/game/geode/mods"
fi
if [[ -n "$install_dir" ]]; then
    install -m 0644 "$build_dir/flafy.showcase.geode" "$install_dir/flafy.showcase.geode"
fi
