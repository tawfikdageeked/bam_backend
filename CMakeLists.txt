cmake_minimum_required(VERSION 3.16)
project(bam_api CXX)

# Enforce C++20 and Maximum Optimization (-O3)
set(CMAKE_CXX_STANDARD 20)
add_compile_options(-O3)

# Find Drogon locally (Drogon provides a proper CMake config)
find_package(Drogon REQUIRED)

# Tell CMake to download jwt-cpp directly from GitHub
include(FetchContent)
FetchContent_Declare(
    jwt-cpp
    GIT_REPOSITORY https://github.com/Thalhammer/jwt-cpp.git
    GIT_TAG v0.7.0
)
FetchContent_MakeAvailable(jwt-cpp)

# Add Source Files
add_executable(task_api 
    src/main.cpp 
    src/authorization.cpp 
    src/crypto.cpp
)

# Tell the compiler where your header files are
target_include_directories(task_api PRIVATE include)

# Link everything together
# "sodium" works automatically here because it is in the standard system path
target_link_libraries(task_api 
    PRIVATE 
    Drogon::Drogon 
    sodium
    jwt-cpp::jwt-cpp
)

# Custom 'make delete' command
add_custom_target(delete
    COMMAND rm -f ${CMAKE_BINARY_DIR}/task_api
    COMMENT "Deleted task_api executable"
)
