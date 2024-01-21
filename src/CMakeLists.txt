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
    auth.hpp
    auth.cpp
    common_state.hpp
    common_state.cpp
)

set_property(TARGET ${PROJECT_NAME} PROPERTY CXX_STANDARD 20)

# boost windows-specific setting
if (WIN32)
    target_compile_definitions(${PROJECT_NAME} PRIVATE _WIN32_WINNT=0x0601)
endif()

target_link_libraries(
    ${PROJECT_NAME}
    Boost::beast
    nlohmann_json::nlohmann_json
    bcrypt
)

# password-hash generator
add_executable(password_hasher hash_generator.cpp)
set_property(TARGET password_hasher PROPERTY CXX_STANDARD 20)
target_link_libraries(password_hasher bcrypt)

# install the server binary and the password hasher to bin

install(
    TARGETS ${PROJECT_NAME} password_hasher
    RUNTIME DESTINATION bin
)

# install the client webapp to bin/client

install(
    DIRECTORY client
    DESTINATION bin
)