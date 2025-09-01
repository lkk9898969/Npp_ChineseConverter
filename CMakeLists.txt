# CMake 最低版本要求，建議 3.15 或更高
cmake_minimum_required(VERSION 3.15)

# 專案名稱與基本設定
project(NppChineseConverter LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- 步驟 1: 初始化 Git 子模組 ---
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

# --- 步驟 2: 編譯依賴函式庫 ---
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries" FORCE)
add_subdirectory(deps/liblzma)
list(APPEND CMAKE_PREFIX_PATH "${CMAKE_BINARY_DIR}/deps/liblzma")
set(ENABLE_TESTS OFF CACHE BOOL "" FORCE)
set(ENABLE_CAT OFF CACHE BOOL "" FORCE)
set(ENABLE_TAR OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/libarchive)
set(ENABLE_TESTING OFF CACHE BOOL "" FORCE)
set(ENABLE_DOCUMENTATION OFF CACHE BOOL "" FORCE)
set(USE_SYSTEM_MARISA OFF CACHE BOOL "" FORCE)
add_subdirectory(deps/OpenCC)

# --- 步驟 3: 自動化字典處理與 .h 檔案生成 ---
find_package(Python REQUIRED COMPONENTS Interpreter)
find_program(SEVENZIP_EXECUTABLE 
    NAMES 7z 7za 
    HINTS "C:/Program Files/7-Zip" "C:/Program Files (x86)/7-Zip"
    REQUIRED
)
message(STATUS "Found 7-Zip: ${SEVENZIP_EXECUTABLE}")

set(SCRIPTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/scripts)
set(OPENCC_DICTS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/opencc_dicts)
set(COMPILED_DICTS_DIR ${CMAKE_BINARY_DIR}/deps/OpenCC/data)
set(CONFIG_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/OpenCC/data/config)
set(SEVENZIP_FILE ${OPENCC_DICTS_DIR}/opencc.7z)
set(HEADER_FILE ${OPENCC_DICTS_DIR}/opencc_archive.h)

add_custom_target(
    GenerateOpenCCData
    COMMAND ${Python_EXECUTABLE} ${SCRIPTS_DIR}/copy_dicts.py
            --compiled-dir ${COMPILED_DICTS_DIR}
            --config-dir ${CONFIG_DIR}
            --dest-dir ${OPENCC_DICTS_DIR}
    COMMAND ${SEVENZIP_EXECUTABLE} a -t7z -m0=lzma2 -y ${SEVENZIP_FILE} ${OPENCC_DICTS_DIR}/*.json ${OPENCC_DICTS_DIR}/*.ocd2
    COMMAND ${Python_EXECUTABLE} ${SCRIPTS_DIR}/file_to_c_array.py
            --input ${SEVENZIP_FILE}
            --output ${HEADER_FILE}
            --array-name opencc_data
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    COMMENT "Generating OpenCC data header from dictionaries..."
    DEPENDS libopencc
    BYPRODUCTS ${HEADER_FILE} ${SEVENZIP_FILE}
)


# --- 步驟 4: 設定主專案 (NppChineseConverter) ---

include_directories(${OPENCC_DICTS_DIR})

file(GLOB_RECURSE PLUGIN_SOURCES
    "src/*.cpp"
    "src/*.h"
    "nppsys/*.c"
    "nppsys/*.h"
)

add_library(NppChineseConverter SHARED ${PLUGIN_SOURCES} ${HEADER_FILE})

# *** MODIFIED LINES START ***
# 為您的插件目標定義所有必要的巨集
target_compile_definitions(NppChineseConverter PRIVATE 
    UNICODE 
    _UNICODE
    LIBARCHIVE_STATIC      # 告訴 libarchive 不要使用 dllimport
    Opencc_BUILT_AS_STATIC # 告訴 opencc 不要使用 dllimport
)
# *** MODIFIED LINES END ***

add_dependencies(NppChineseConverter GenerateOpenCCData)

# *** MODIFIED LINES START ***
# 連結所有依賴的函式庫，並加入必要的 Windows 系統函式庫
target_link_libraries(NppChineseConverter PRIVATE
    libopencc
    archive_static
    liblzma
    shlwapi     # 提供 PathAppendW, PathFileExistsW 等函式
    bcrypt      # libarchive 需要的加密函式庫
)
# *** MODIFIED LINES END ***

target_include_directories(NppChineseConverter PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/nppsys"
    "${CMAKE_CURRENT_SOURCE_DIR}/opencc_dicts"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/OpenCC/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/libarchive/libarchive"
    "${CMAKE_CURRENT_SOURCE_DIR}/deps/liblzma/src/liblzma/api"
)