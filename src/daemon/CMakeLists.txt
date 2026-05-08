# src/daemon/CMakeLists.txt

# 1. Define the executable with ALL its sources at once
add_executable(nanshe-d 
    main.cpp
    ../platform/openbsd/OpenBSD_Daemon.cpp
    ../engine/llama/Llama_Engine.cpp
)

# 2. Setup the library paths (inherited from Root)
set(LLAMA_LIB_DIR  "${LLAMA_BUILD_DIR}/src")
set(GGML_LIB_DIR   "${LLAMA_BUILD_DIR}/ggml/src")
set(COMMON_LIB_DIR "${LLAMA_BUILD_DIR}/common")

# 3. Link the logic
target_link_libraries(nanshe-d PRIVATE
    # High-level API
    "${LLAMA_LIB_DIR}/libllama.a"

    # Common helpers
    "${COMMON_LIB_DIR}/libllama-common.a"
    "${COMMON_LIB_DIR}/libllama-common-base.a"

    # The GGML stack
    "${GGML_LIB_DIR}/libggml.a"
    "${GGML_LIB_DIR}/libggml-cpu.a"
    "${GGML_LIB_DIR}/libggml-base.a"

    # System requirements
    pthread
    m
)

# 4. Platform specific extras
if(${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
    target_link_libraries(nanshe-d PRIVATE util)
endif()
