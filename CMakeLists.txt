cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES "src/Server.cpp")
file(GLOB_RECURSE CLIENT "src/Client.cpp")

# list(REMOVE_ITEM SOURCE_FILES "src/Client.cpp")


set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
set(PATH_ASIO "$ENV{HOME}\\vcpkg\\packages\\asio_x64-linux\\include")

add_executable(server ${SOURCE_FILES})
add_executable(client ${CLIENT})

target_include_directories(client PRIVATE ${PATH_ASIO})
target_link_libraries(client PRIVATE Threads::Threads)

target_include_directories(server PRIVATE ${PATH_ASIO})
target_link_libraries(server PRIVATE Threads::Threads)

