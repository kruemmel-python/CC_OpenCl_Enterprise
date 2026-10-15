cmake_minimum_required(VERSION 3.20)

project(CC_OpenCl
    VERSION 1.0.1
    DESCRIPTION "Standalone CC_OpenCl OpenCL driver DLL"
    LANGUAGES C CXX
)

# CC_OpenCL.c contains C++ raw-string literals for embedded OpenCL kernels: R"CLC(... )CLC"
# Therefore both C and C++ sources must be compiled as C++17.
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# ---------------------------------------------------------------------------
# 1. Windows x64 & LLVM/MinGW Toolchain Configuration (KR Compiler Android IDE)
# ---------------------------------------------------------------------------
# When targeting Windows with Clang/LLD (e.g. KR Compiler Android IDE cross-compiler),
# configure the runtime libraries explicitly so the MinGW driver does not request
# GCC runtime libraries (-lgcc, -lgcc_s) which are not present in the LLVM sysroot.
if(WIN32)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(_KR_LLVM_LINKER_FLAGS "-fuse-ld=lld -stdlib=libc++ -rtlib=compiler-rt -unwindlib=libunwind")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${_KR_LLVM_LINKER_FLAGS}")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${_KR_LLVM_LINKER_FLAGS}")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${_KR_LLVM_LINKER_FLAGS}")
    endif()
endif()

# ---------------------------------------------------------------------------
# 2. OpenCL Dependency Resolution
# ---------------------------------------------------------------------------
# Try standard system / vcpkg find_package first.
if(NOT TARGET OpenCL::OpenCL)
    find_package(OpenCL QUIET)
endif()

# If OpenCL was not found in sysroot / system, use the vendored Khronos OpenCL
# headers and import library bundled directly with this repository.
if(NOT TARGET OpenCL::OpenCL)
    message(STATUS "OpenCL was not located in sysroot; activating vendored Khronos OpenCL headers and import libraries.")
    add_library(OpenCL::OpenCL INTERFACE IMPORTED)
    target_include_directories(OpenCL::OpenCL INTERFACE
        ${CMAKE_CURRENT_SOURCE_DIR}/CL
    )
    if(MSVC)
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/OpenCL.lib")
            target_link_libraries(OpenCL::OpenCL INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/build/OpenCL.lib")
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CL/OpenCL.lib")
            target_link_libraries(OpenCL::OpenCL INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/CL/OpenCL.lib")
        endif()
    else()
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/CL/libOpenCL.dll.a")
            target_link_libraries(OpenCL::OpenCL INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/CL/libOpenCL.dll.a")
        elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/build/OpenCL.lib")
            target_link_libraries(OpenCL::OpenCL INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/build/OpenCL.lib")
        endif()
    endif()
endif()

# ---------------------------------------------------------------------------
# 3. Main Target: CC_OpenCl SHARED Library
# ---------------------------------------------------------------------------
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
    BEFORE PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/CL
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
    if(WIN32)
        target_link_libraries(CC_OpenCl PRIVATE -lkernel32 -luser32)
        if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
            target_link_options(CC_OpenCl PRIVATE -fuse-ld=lld -stdlib=libc++ -rtlib=compiler-rt -unwindlib=libunwind)
        endif()
    elseif(UNIX AND NOT APPLE)
        target_link_libraries(CC_OpenCl PRIVATE m dl pthread)
    endif()
endif()

# Set properties: Ensure exact output name CC_OpenCl.dll without 'lib' prefix on Windows
set_target_properties(CC_OpenCl PROPERTIES
    OUTPUT_NAME "CC_OpenCl"
    PREFIX ""
    IMPORT_PREFIX ""
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
if(NOT DEFINED KR_EXECUTABLE_OUTPUT)
    set_target_properties(CC_OpenCl PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    )
endif()

# ---------------------------------------------------------------------------
# 4. Windows Test & Validation Executable (Satisfies KR Compiler & Test D)
# ---------------------------------------------------------------------------
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/test_cc_opencl.cpp")
    add_executable(test_cc_opencl tests/test_cc_opencl.cpp)
    target_include_directories(test_cc_opencl PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/CL
    )
    target_link_libraries(test_cc_opencl PRIVATE CC_OpenCl)
    if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_options(test_cc_opencl PRIVATE -fuse-ld=lld -stdlib=libc++ -rtlib=compiler-rt -unwindlib=libunwind)
    endif()
    if(NOT DEFINED KR_EXECUTABLE_OUTPUT)
        set_target_properties(test_cc_opencl PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
        )
    endif()
endif()

# ---------------------------------------------------------------------------
# 5. Artifact Placement for KR Compiler IDE and Packaging
# ---------------------------------------------------------------------------
add_custom_command(TARGET CC_OpenCl POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/../KR_BUILD"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:CC_OpenCl>
        "${CMAKE_SOURCE_DIR}/../KR_BUILD/CC_OpenCl.dll"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_LINKER_FILE:CC_OpenCl>
        "${CMAKE_SOURCE_DIR}/../KR_BUILD/libCC_OpenCl.dll.a"
    COMMENT "Copying CC_OpenCl.dll and import library to KR_BUILD"
)

if(TARGET test_cc_opencl)
    add_custom_command(TARGET test_cc_opencl POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:test_cc_opencl>
            "${CMAKE_SOURCE_DIR}/../KR_BUILD/test_cc_opencl.exe"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:CC_OpenCl>
            "$<TARGET_FILE_DIR:test_cc_opencl>/CC_OpenCl.dll"
        COMMENT "Copying test_cc_opencl.exe to KR_BUILD and CC_OpenCl.dll alongside test executable"
    )
endif()

install(TARGETS CC_OpenCl
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION bin
    ARCHIVE DESTINATION lib
)

if(TARGET test_cc_opencl)
    install(TARGETS test_cc_opencl RUNTIME DESTINATION bin)
endif()
