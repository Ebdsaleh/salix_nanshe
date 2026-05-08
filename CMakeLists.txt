# salix_nanshe/CMakeLists.txt <project_root>
cmake_minimum_required(VERSION 3.15)
project(salix_nanshe LANGUAGES C CXX)

# Strict Mode: Treat warnings as errors
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


if(MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Wpedantic -Werror)
endif()



# Global Includes -
# We tell CMake that 'src' is a root for headers
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/src")

# Vendor paths
set(LLAMA_DIR "${CMAKE_CURRENT_SOURCE_DIR}/vendor/llama-cpp")
set(LLAMA_BUILD_DIR "${LLAMA_DIR}/build")
include_directories("${LLAMA_DIR}/include")
include_directories("${LLAMA_DIR}/ggml/include")  # Add this line
include_directories("${LLAMA_DIR}/common")

# Descend into the source tree
add_subdirectory(src/daemon)
add_subdirectory(src/client)
# add_subdirectory(src/engine) # Uncomment when ready to build the engine bridge

