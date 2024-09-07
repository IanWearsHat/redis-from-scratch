cmake_minimum_required(VERSION 3.13)

project(redis-starter-cpp)

file(GLOB_RECURSE SOURCE_FILES src/*.cpp)

set(CMAKE_CXX_STANDARD 23) # Enable the C++23 standard
set(THREADS_PREFER_PTHREAD_FLAG ON)

find_package(Threads REQUIRED)
set(PATH_ASIO "$ENV{HOME}\\vcpkg\\packages\\asio_x64-linux\\include")

add_executable(server ${SOURCE_FILES})

target_include_directories(server PRIVATE ${PATH_ASIO})

target_link_libraries(server PRIVATE Threads::Threads)