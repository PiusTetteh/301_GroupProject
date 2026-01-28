cmake_minimum_required(VERSION 3.10)
project(Multikernel_OS)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler flags for optimization and warnings
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# Enable threading
set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

# Source files
set(SOURCES
    main.cpp
    core_kernel.cpp
    multikernel_system.cpp
)

# Header files
set(HEADERS
    multikernel.h
)

# Create executable
add_executable(multikernel_os ${SOURCES} ${HEADERS})

# Link threading library
target_link_libraries(multikernel_os PRIVATE Threads::Threads)

# Print build information
message(STATUS "Building Multikernel OS")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
