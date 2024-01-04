cmake_minimum_required(VERSION 3.14 FATAL_ERROR)

set(GATE_CONTROL_PROJECT_NAME GateControl)
set(GATE_CONTROL_SOURCE_DIR src)

set(GATE_CONTROL_SOURCE
    ${GATE_CONTROL_SOURCE_DIR}/main.cpp
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

project(${GATE_CONTROL_PROJECT_NAME})

add_executable(${GATE_CONTROL_PROJECT_NAME} ${GATE_CONTROL_SOURCE})

set_property(TARGET ${GATE_CONTROL_PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# link libraries

target_include_directories(${GATE_CONTROL_PROJECT_NAME} PRIVATE ${BOOST_LIBRARIES})

target_link_libraries(
    ${GATE_CONTROL_PROJECT_NAME} PRIVATE
    Boost::beast nlohmann_json::nlohmann_json
)