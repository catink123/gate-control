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
    websocket_session.hpp
    websocket_session.cpp
    json_message.hpp
    json_message.cpp
    arduino_messenger.hpp
    arduino_messenger.cpp
    auth_table.hpp
    auth.hpp
    auth.cpp
    common_state.hpp
    common_state.cpp
    config.hpp
)

add_executable(
    configurator 
    auth_table.hpp
    configurator.cpp
)

add_library(console_prettifier INTERFACE console_prettifier.hpp)

set_target_properties(${PROJECT_NAME} PROPERTIES CXX_STANDARD 20)
set_target_properties(configurator PROPERTIES CXX_STANDARD 20)
set_target_properties(console_prettifier PROPERTIES CXX_STANDARD 20 LINKER_LANGUAGE CXX)

# boost windows-specific setting
if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0601)
endif()

target_link_libraries(
    ${PROJECT_NAME}
    Boost::beast
    nlohmann_json::nlohmann_json
    cryptopp::cryptopp
)

target_link_libraries(
    configurator
    console_prettifier
)

# install the server binary and the configurator to bin

install(
    TARGETS ${PROJECT_NAME} configurator
    RUNTIME DESTINATION bin
)

# install the client webapp to bin/client

install(
    DIRECTORY client
    DESTINATION bin
)
