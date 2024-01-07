configure_file(${CMAKE_CURRENT_SOURCE_DIR}/version.hpp.in ${CMAKE_CURRENT_SOURCE_DIR}/version.hpp @ONLY)

add_executable(
    ${PROJECT_NAME}
    main.cpp
    version.hpp
    common.hpp
    http_listener.hpp
    http_listener.cpp
    http_session.hpp
    http_session.cpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 17)

# boost windows-specific setting
if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0601)
endif()

target_link_libraries(
    ${PROJECT_NAME}
    Boost::beast
    nlohmann_json::nlohmann_json
)

# install server binary to bin

install(
    TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)

# install the client webapp to bin/client

install(
    DIRECTORY client
    DESTINATION bin
)