cmake_minimum_required(VERSION 3.16)
project(CaffisChat VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find required packages
find_package(Boost REQUIRED COMPONENTS system thread)
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBPQXX REQUIRED libpqxx)
find_package(OpenSSL REQUIRED)

# Include directories
include_directories(${CMAKE_SOURCE_DIR}/include)
include_directories(${Boost_INCLUDE_DIRS})
include_directories(${LIBPQXX_INCLUDE_DIRS})

# Source files - Fix: Only include files that exist and have real implementations
set(SOURCES
    src/main.cpp
    src/websocket_server.cpp
)

# Create executable
add_executable(caffis_chat ${SOURCES})

# Link libraries
target_link_libraries(caffis_chat 
    ${Boost_LIBRARIES}
    ${LIBPQXX_LIBRARIES}
    OpenSSL::SSL
    OpenSSL::Crypto
    pthread
)

# Compiler flags
target_compile_options(caffis_chat PRIVATE -Wall -Wextra -O2)

# Make sure the executable is copied to the right location
install(TARGETS caffis_chat DESTINATION bin)