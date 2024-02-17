cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(
    GateControl
    VERSION 1.1.0
    LANGUAGES CXX
)

include("cmake/modules/CPM.cmake")

# show download progress
set(FETCHCONTENT_QUIET FALSE)

CPMAddPackage("gh:nlohmann/json@3.11.3")

CPMAddPackage(
    NAME Boost
    VERSION 1.84.0
    GITHUB_REPOSITORY "boostorg/boost"
    GIT_TAG "boost-1.84.0"
    OPTIONS
        "BOOST_INCLUDE_LIBRARIES beast"
        "BOOST_ENABLE_CMAKE ON"
        "BUILD_TESTS OFF"
)

CPMAddPackage(
    NAME cryptopp
    VERSION 8.9.0
    GITHUB_REPOSITORY "abdes/cryptopp-cmake"
    GIT_TAG "CRYPTOPP_8_9_0"
    OPTIONS
        "BUILD_TESTS OFF"
        "CRYPTOPP_BUILD_TESTING OFF"
)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)