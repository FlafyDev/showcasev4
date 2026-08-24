cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
if ("${CMAKE_SYSTEM_NAME}" STREQUAL "iOS" OR IOS)
    set(CMAKE_OSX_ARCHITECTURES "arm64")
else()
    set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64")
endif()
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

set(CMAKE_INTERPROCEDURAL_OPTIMIZATION ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SHOWCASE_API_ORIGIN "https://showcase-api.flafy.dev" CACHE STRING "Showcase API origin")

project(showcase-v4 VERSION 4.0.0)

# Add all source files inside src (recursively)
file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)

# Set up the mod binary
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_compile_definitions(${PROJECT_NAME} PRIVATE SHOWCASE_API_ORIGIN="${SHOWCASE_API_ORIGIN}")

if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Unable to find Geode SDK! Please define GEODE_SDK environment variable to point to Geode")
else()
    message(STATUS "Found Geode: $ENV{GEODE_SDK}")
endif()

add_subdirectory($ENV{GEODE_SDK} ${CMAKE_CURRENT_BINARY_DIR}/geode)

target_include_directories(${PROJECT_NAME} PUBLIC src)

CPMAddPackage("gh:GlobedGD/argon@1.5.1")
CPMAddPackage(
    NAME GDReplayFormat
    GITHUB_REPOSITORY maxnut/GDReplayFormat
    GIT_TAG 2a0ca3cff9b1d16863037a464da5fe27c28a5823
    DOWNLOAD_ONLY YES
)
target_sources(${PROJECT_NAME} PRIVATE $ENV{GEODE_SDK}/loader/hash/hash.cpp)
target_include_directories(${PROJECT_NAME} PRIVATE $ENV{GEODE_SDK}/loader/hash)
target_include_directories(${PROJECT_NAME} PRIVATE ${GDReplayFormat_SOURCE_DIR}/include)
target_link_libraries(${PROJECT_NAME} geode-sdk argon)

# Set up dependencies, resources, and link Geode.
setup_geode_mod(${PROJECT_NAME})
