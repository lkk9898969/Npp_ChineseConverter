cmake_minimum_required(VERSION 3.15)

project(NppChineseConverter VERSION 1.0.1 LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_definitions(_CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_WARNINGS)

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


# LZMA SDK Object Library
file(GLOB LZMA_SDK_SOURCES
    "deps/lzma_sdk/src/*.c"
)
add_library(LzmaSdkObjs OBJECT ${LZMA_SDK_SOURCES})
target_include_directories(LzmaSdkObjs PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps/lzma_sdk/inc")
set_target_properties(LzmaSdkObjs PROPERTIES FOLDER "deps")


# Project Configuration
file(GLOB_RECURSE PLUGIN_SOURCES
    "src/*.cpp"
    "src/*.rc"
    "nppsys/*.c"
)

add_library(NppChineseConverter SHARED ${PLUGIN_SOURCES} ${HEADER_FILE} $<TARGET_OBJECTS:LzmaSdkObjs>)

if(NOT PROJECT_VERSION_PATCH)
    set(PROJECT_VERSION_PATCH 0)
endif()
if(NOT PROJECT_VERSION_TWEAK)
    set(PROJECT_VERSION_TWEAK 0)
endif()

target_compile_definitions(NppChineseConverter PRIVATE 
    UNICODE
    _UNICODE
    Opencc_BUILT_AS_STATIC
    NPP_PLUGIN_VERSION_MAJOR=${PROJECT_VERSION_MAJOR}
    NPP_PLUGIN_VERSION_MINOR=${PROJECT_VERSION_MINOR}
    NPP_PLUGIN_VERSION_PATCH=${PROJECT_VERSION_PATCH}
    NPP_PLUGIN_VERSION_TWEAK=${PROJECT_VERSION_TWEAK}
    NPP_PLUGIN_VERSION_STRING="${PROJECT_VERSION}"
)

# add_dependencies(NppChineseConverter archive_imported GenerateOpenCCData)

target_link_libraries(NppChineseConverter PRIVATE
    libopencc
    # archive_imported
    shlwapi     # Nppsys
)

target_include_directories(NppChineseConverter PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/nppsys"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/lzma_sdk/inc"
    "${CMAKE_BINARY_DIR}/generated"
    "${STAGING_DIR}/include"
)