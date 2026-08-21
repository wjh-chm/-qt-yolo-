cmake_minimum_required(VERSION 3.16)

project(smart_monitor_server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(smart_monitor_server
    main.cpp
    util/config.cpp
    db/db_connection.cpp
    net/tcp_server.cpp
    protocol/protocol.cpp
    service/auth_service.cpp
    service/video_service.cpp
    service/image_service.cpp
    service/log_service.cpp
)
target_include_directories(smart_monitor_server PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(smart_monitor_server
    mysqlclient
    ssl
    crypto
)