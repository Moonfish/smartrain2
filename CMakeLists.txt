cmake_minimum_required(VERSION 3.0)

project (smartrain)

# Have CMake find our pthreads library within our toolchain (required for this library)
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

include_directories("/home/russell/code/cpp/boost")

# add all the *.c files as sources
FILE(GLOB SRC_FILES src/*.cpp)
add_executable(smartrain ${SRC_FILES})

target_link_libraries(smartrain LINK_PUBLIC ${CMAKE_THREAD_LIBS_INIT})

