cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

project(
    GateControl
    VERSION 1.0.0
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
)

add_subdirectory(${PROJECT_SOURCE_DIR}/src)