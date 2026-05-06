cmake_minimum_required(VERSION 3.20)

project(CC_OpenCl
    VERSION 1.0.1
    DESCRIPTION "Standalone CC_OpenCl OpenCL driver DLL"
    LANGUAGES C CXX
)

# CC_OpenCL.c intentionally contains C++ raw-string literals for embedded
# OpenCL kernels: R"CLC(... )CLC". Therefore MSVC must compile it as C++.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenCL REQUIRED)

add_library(CC_OpenCl SHARED
    src/CC_OpenCL.c
    src/CipherCore_NoiseCtrl.c
)

set_source_files_properties(
    src/CC_OpenCL.c
    src/CipherCore_NoiseCtrl.c
    PROPERTIES LANGUAGE CXX
)

target_include_directories(CC_OpenCl
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include
)

target_link_libraries(CC_OpenCl
    PRIVATE
        OpenCL::OpenCL
)

target_compile_definitions(CC_OpenCl
    PRIVATE
        _CRT_SECURE_NO_WARNINGS
        CL_TARGET_OPENCL_VERSION=300
        CL_USE_DEPRECATED_OPENCL_1_2_APIS
)

if(MSVC)
    target_compile_options(CC_OpenCl PRIVATE /TP /W3 /wd4996 /wd4244 /wd4267 /wd5105)
else()
    target_compile_options(CC_OpenCl PRIVATE -Wall -Wextra -Wno-unused-parameter -Wno-unused-function)
    if(UNIX AND NOT APPLE)
        target_link_libraries(CC_OpenCl PRIVATE m)
    endif()
endif()

set_target_properties(CC_OpenCl PROPERTIES
    OUTPUT_NAME "CC_OpenCl"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

install(TARGETS CC_OpenCl
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)
