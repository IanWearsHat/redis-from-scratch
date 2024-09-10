cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

set(SERVER_SOURCE_FILES
    src/Server.cpp
    src/connections.cpp
)
set(CLIENT_SOURCE_FILES
    src/Client.cpp
    src/connections.cpp
)


find_package(Threads REQUIRED)
set(PATH_ASIO "$ENV{HOME}\\vcpkg\\packages\\asio_x64-linux\\include")

add_executable(server ${SERVER_SOURCE_FILES})
add_executable(client ${CLIENT_SOURCE_FILES})

target_include_directories(client PRIVATE ${INCLUDE_DIR} ${PATH_ASIO})
target_include_directories(server PRIVATE ${INCLUDE_DIR} ${PATH_ASIO})

target_link_libraries(client PRIVATE Threads::Threads)
target_link_libraries(server PRIVATE Threads::Threads)
