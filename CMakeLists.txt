cmake_minimum_required(VERSION 3.15)

project(NppChineseConverter LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(ExternalProject)

# Initialize Submodules
find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/deps/OpenCC/CMakeLists.txt")
        message(STATUS "Git submodules are not initialized. Initializing them now.")
        execute_process(
            COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
            RESULT_VARIABLE GIT_SUBMODULE_RESULT
        )
        if(NOT GIT_SUBMODULE_RESULT EQUAL "0")
            message(FATAL_ERROR "Failed to initialize Git submodules.")
        endif()
    endif()
endif()

# Define a common installation directory for external project dependencies
set(STAGING_DIR ${CMAKE_BINARY_DIR}/deps_install)
message(STATUS "Dependencies will be installed to: ${STAGING_DIR}")

# Define liblzma and libarchive as an external project
# Because libarchive need liblzma library in configure step.
ExternalProject_Add(liblzma_ep
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/liblzma
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${STAGING_DIR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DBUILD_SHARED_LIBS=OFF
    TEST_COMMAND ""
)

ExternalProject_Add(libarchive_ep
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/libarchive
    DEPENDS liblzma_ep
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${STAGING_DIR}
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
        -DCMAKE_PREFIX_PATH=${STAGING_DIR}
        -DBUILD_SHARED_LIBS=OFF
        -DENABLE_TEST=OFF
        -DENABLE_CAT=OFF
        -DENABLE_TAR=OFF
    TEST_COMMAND ""
)

add_library(lzma_imported STATIC IMPORTED GLOBAL)
set_property(TARGET lzma_imported PROPERTY
    IMPORTED_LOCATION ${STAGING_DIR}/lib/lzma.lib
)
add_dependencies(lzma_imported liblzma_ep)

add_library(archive_imported STATIC IMPORTED GLOBAL)
set_property(TARGET archive_imported PROPERTY
    IMPORTED_LOCATION ${STAGING_DIR}/lib/archive.lib
)
set_property(TARGET archive_imported APPEND PROPERTY
    INTERFACE_LINK_LIBRARIES lzma_imported
)
add_dependencies(archive_imported libarchive_ep)

# Initialize OpenCC
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(USE_SYSTEM_MARISA OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/OpenCC)

# Generate OpenCC dictionaries
find_package(Python REQUIRED COMPONENTS Interpreter)
find_program(SEVENZIP_EXECUTABLE 
    NAMES 7z 7za 
    HINTS "C:/Program Files/7-Zip" "C:/Program Files (x86)/7-Zip"
    REQUIRED
)
message(STATUS "Found 7-Zip: ${SEVENZIP_EXECUTABLE}")

set(SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
set(TEMP_DICTS_DIR ${CMAKE_BINARY_DIR}/opencc_dicts_temp)
set(COMPILED_DICTS_DIR ${CMAKE_BINARY_DIR}/deps/OpenCC/data)
set(CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/OpenCC/data/config)
set(SEVENZIP_FILE ${CMAKE_BINARY_DIR}/opencc.7z)
set(HEADER_FILE ${CMAKE_BINARY_DIR}/generated/opencc_archive.h)

add_custom_target(
    GenerateOpenCCData
    DEPENDS libopencc Dictionaries
    COMMAND ${Python_EXECUTABLE} ${SCRIPTS_DIR}/copy_dicts.py
            --compiled-dir ${COMPILED_DICTS_DIR}
            --config-dir ${CONFIG_DIR}
            --dest-dir ${TEMP_DICTS_DIR}
    COMMAND ${SEVENZIP_EXECUTABLE} a -t7z -m0=lzma2 -y ${SEVENZIP_FILE} ${TEMP_DICTS_DIR}/*.json ${TEMP_DICTS_DIR}/*.ocd2
    COMMAND ${Python_EXECUTABLE} ${SCRIPTS_DIR}/file_to_c_array.py
            --input ${SEVENZIP_FILE}
            --output ${HEADER_FILE}
            --array-name opencc_data
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating OpenCC data header from dictionaries..."
    BYPRODUCTS ${HEADER_FILE} ${SEVENZIP_FILE}
)


# Project Configuration

file(GLOB_RECURSE PLUGIN_SOURCES
    "src/*.cpp"
    "src/*.h"
    "nppsys/*.c"
    "nppsys/*.h"
)

add_library(NppChineseConverter SHARED ${PLUGIN_SOURCES} ${HEADER_FILE})

target_compile_definitions(NppChineseConverter PRIVATE 
    UNICODE
    _UNICODE
    LIBARCHIVE_STATIC
    Opencc_BUILT_AS_STATIC
)

add_dependencies(NppChineseConverter archive_imported GenerateOpenCCData)

target_link_libraries(NppChineseConverter PRIVATE
    libopencc
    archive_imported
    shlwapi     # Nppsys
    bcrypt      # libarchive
)

target_include_directories(NppChineseConverter PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/nppsys"
    "${CMAKE_BINARY_DIR}/generated"
    "${STAGING_DIR}/include"
)