cmake_minimum_required(VERSION 3.15)

project(NppChineseConverter LANGUAGES CXX C)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Options
option(USE_SYSTEM_LIBLZMA "Use system-provided liblzma library (e.g., from vcpkg) instead of building from source. Note: This option has no effect if USE_SYSTEM_LIBARCHIVE is enabled." OFF)
option(USE_SYSTEM_LIBARCHIVE "Use system-provided libarchive library (e.g., from vcpkg) instead of building from source" OFF)

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

# Setup libarchive and liblzma
if(USE_SYSTEM_LIBARCHIVE)
    message(STATUS "Using system-provided libarchive library (liblzma included)")
    
    # Prefer static libraries
    set(CMAKE_FIND_LIBRARY_SUFFIXES_BACKUP ${CMAKE_FIND_LIBRARY_SUFFIXES})
    if(WIN32)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
    
    find_package(LibArchive REQUIRED)
    
    # Restore original suffixes
    set(CMAKE_FIND_LIBRARY_SUFFIXES ${CMAKE_FIND_LIBRARY_SUFFIXES_BACKUP})
    
    add_library(archive_imported ALIAS LibArchive::LibArchive)

    # Runtime check: verify libarchive actually supports LZMA/XZ filters.
    # Some builds provide the functions but return ARCHIVE_FATAL at runtime when LZMA backend is missing.
    if(NOT CMAKE_CROSSCOMPILING)
        file(MAKE_DIRECTORY ${CMAKE_BINARY_DIR}/cmake)
        set(_ARCHIVE_LZMA_TEST_SRC "#include <archive.h>\nint main(){\n  struct archive *a = archive_write_new();\n  if(!a) return 2;\n  int r = 0;\n  /* Prefer XZ (LZMA2) used by 7z, fallback to LZMA */\n  r = archive_write_add_filter_xz(a);\n  if(r == ARCHIVE_OK || r == ARCHIVE_WARN){ archive_write_free(a); return 0; }\n  r = archive_write_add_filter_lzma(a);\n  if(r == ARCHIVE_OK || r == ARCHIVE_WARN){ archive_write_free(a); return 0; }\n  archive_write_free(a);\n  return 1;\n}\n")
        set(_ARCHIVE_LZMA_TEST_FILE ${CMAKE_BINARY_DIR}/cmake/CheckArchiveLZMA.c)
        file(WRITE ${_ARCHIVE_LZMA_TEST_FILE} "${_ARCHIVE_LZMA_TEST_SRC}")

        try_run(_ARCHIVE_LZMA_RUN_RESULT _ARCHIVE_LZMA_COMPILE_RESULT
            ${CMAKE_BINARY_DIR}/cmake
            ${_ARCHIVE_LZMA_TEST_FILE}
            LINK_LIBRARIES LibArchive::LibArchive
            RUN_OUTPUT_VARIABLE _ARCHIVE_LZMA_RUN_OUTPUT
        )

        if(_ARCHIVE_LZMA_COMPILE_RESULT AND _ARCHIVE_LZMA_RUN_RESULT EQUAL 0)
            set(LIBARCHIVE_SUPPORTS_LZMA TRUE CACHE BOOL "System libarchive supports LZMA/XZ" FORCE)
            message(STATUS "System libarchive supports LZMA/XZ filters (validated by try_run)")
        else()
            set(LIBARCHIVE_SUPPORTS_LZMA FALSE CACHE BOOL "System libarchive supports LZMA/XZ" FORCE)
            message(FATAL_ERROR "System libarchive appears to lack LZMA/XZ support (functions return ARCHIVE_FATAL).\n" 
                                "Output: ${_ARCHIVE_LZMA_RUN_OUTPUT}\n"
                                "Tip: Use a package with LZMA enabled (e.g., vcpkg triplet x64-windows-static or ensure feature 'lzma' is on), or disable USE_SYSTEM_LIBARCHIVE to build from source.")
        endif()
    else()
        message(FATAL_ERROR "Cross-compiling: cannot validate system libarchive LZMA/XZ support. Disable USE_SYSTEM_LIBARCHIVE or build natively to validate, or vendor libarchive.")
    endif()
else()
    message(STATUS "Building libarchive from source")
    
    # If building libarchive from source, need to configure liblzma first
    if(USE_SYSTEM_LIBLZMA)
        message(STATUS "Using system-provided liblzma library")
        
        # Configure libarchive with minimal options: only LZMA support
        set(_OLD_BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS})
        set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
        
        # Disable all compression formats except LZMA
        set(ENABLE_LZMA ON CACHE BOOL "" FORCE)
        set(ENABLE_ZLIB OFF CACHE BOOL "" FORCE)
        set(ENABLE_BZip2 OFF CACHE BOOL "" FORCE)
        set(ENABLE_LIBB2 OFF CACHE BOOL "" FORCE)
        set(ENABLE_LZ4 OFF CACHE BOOL "" FORCE)
        set(ENABLE_LZO OFF CACHE BOOL "" FORCE)
        set(ENABLE_ZSTD OFF CACHE BOOL "" FORCE)
        
        # Disable crypto libraries (not needed for decompression)
        set(ENABLE_MBEDTLS OFF CACHE BOOL "" FORCE)
        set(ENABLE_NETTLE OFF CACHE BOOL "" FORCE)
        set(ENABLE_OPENSSL OFF CACHE BOOL "" FORCE)
        set(ENABLE_CNG OFF CACHE BOOL "" FORCE)
        
        # Disable XML support
        set(ENABLE_LIBXML2 OFF CACHE BOOL "" FORCE)
        set(ENABLE_EXPAT OFF CACHE BOOL "" FORCE)
        set(ENABLE_WIN32_XMLLITE OFF CACHE BOOL "" FORCE)
        
        # Disable regex support
        set(ENABLE_PCREPOSIX OFF CACHE BOOL "" FORCE)
        set(ENABLE_PCRE2POSIX OFF CACHE BOOL "" FORCE)
        set(POSIX_REGEX_LIB OFF CACHE BOOL "" FORCE)
        
        # Disable extra features
        set(ENABLE_LIBGCC OFF CACHE BOOL "" FORCE)
        set(ENABLE_XATTR OFF CACHE BOOL "" FORCE)
        set(ENABLE_ACL OFF CACHE BOOL "" FORCE)
        set(ENABLE_ICONV OFF CACHE BOOL "" FORCE)
        
        # Disable command-line tools
        set(ENABLE_TAR OFF CACHE BOOL "" FORCE)
        set(ENABLE_CPIO OFF CACHE BOOL "" FORCE)
        set(ENABLE_CAT OFF CACHE BOOL "" FORCE)
        
        # Disable tests
        set(ENABLE_TEST OFF CACHE BOOL "" FORCE)
        
        find_package(LibLZMA REQUIRED)
        add_library(lzma_imported ALIAS LibLZMA::LibLZMA)
        add_subdirectory(deps/libarchive)
        set(BUILD_SHARED_LIBS ${_OLD_BUILD_SHARED_LIBS} CACHE BOOL "" FORCE)
        
        # libarchive creates 'archive_static' target when BUILD_SHARED_LIBS=OFF
        if(TARGET archive_static)
            add_library(archive_imported ALIAS archive_static)
            target_compile_definitions(archive_static INTERFACE LIBARCHIVE_STATIC)
        elseif(TARGET archive)
            add_library(archive_imported ALIAS archive)
            target_compile_definitions(archive INTERFACE LIBARCHIVE_STATIC)
        else()
            message(FATAL_ERROR "libarchive did not create expected target (archive or archive_static)")
        endif()
    else()
        # If liblzma is also need to be built from source
        # Use ExternalProject to build both liblzma and libarchive
        message(STATUS "Building liblzma from source")
        message(STATUS "Dependencies will be installed to: ${STAGING_DIR}")
        
        ExternalProject_Add(liblzma_ep
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/liblzma
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${STAGING_DIR}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DBUILD_SHARED_LIBS=OFF
            TEST_COMMAND ""
        )
        
        add_library(lzma_imported STATIC IMPORTED GLOBAL)
        set_property(TARGET lzma_imported PROPERTY
            IMPORTED_LOCATION ${STAGING_DIR}/lib/lzma.lib
        )
        add_dependencies(lzma_imported liblzma_ep)
        
        ExternalProject_Add(libarchive_ep
            SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/deps/libarchive
            DEPENDS liblzma_ep
            CMAKE_ARGS
                -DCMAKE_INSTALL_PREFIX=${STAGING_DIR}
                -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}
                -DCMAKE_PREFIX_PATH=${STAGING_DIR}
                -DBUILD_SHARED_LIBS=OFF
                # Only enable LZMA support
                -DENABLE_LZMA=ON
                -DENABLE_ZLIB=OFF
                -DENABLE_BZip2=OFF
                -DENABLE_LIBB2=OFF
                -DENABLE_LZ4=OFF
                -DENABLE_LZO=OFF
                -DENABLE_ZSTD=OFF
                # Disable crypto
                -DENABLE_MBEDTLS=OFF
                -DENABLE_NETTLE=OFF
                -DENABLE_OPENSSL=OFF
                -DENABLE_CNG=OFF
                # Disable XML
                -DENABLE_LIBXML2=OFF
                -DENABLE_EXPAT=OFF
                -DENABLE_WIN32_XMLLITE=OFF
                # Disable regex
                -DENABLE_PCREPOSIX=OFF
                -DENABLE_PCRE2POSIX=OFF
                -DPOSIX_REGEX_LIB=OFF
                # Disable extras
                -DENABLE_LIBGCC=OFF
                -DENABLE_XATTR=OFF
                -DENABLE_ACL=OFF
                -DENABLE_ICONV=OFF
                # Disable tools
                -DENABLE_TAR=OFF
                -DENABLE_CPIO=OFF
                -DENABLE_CAT=OFF
                # Disable tests
                -DENABLE_TEST=OFF
            TEST_COMMAND ""
        )
        
        add_library(archive_imported STATIC IMPORTED GLOBAL)
        set_property(TARGET archive_imported PROPERTY
            IMPORTED_LOCATION ${STAGING_DIR}/lib/archive.lib
        )
        set_property(TARGET archive_imported APPEND PROPERTY
            INTERFACE_LINK_LIBRARIES lzma_imported
        )
        set_property(TARGET archive_imported APPEND PROPERTY
            INTERFACE_COMPILE_DEFINITIONS LIBARCHIVE_STATIC
        )
        add_dependencies(archive_imported libarchive_ep)
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
    Opencc_BUILT_AS_STATIC
)

add_dependencies(NppChineseConverter archive_imported GenerateOpenCCData)

target_link_libraries(NppChineseConverter PRIVATE
    libopencc
    archive_imported
    shlwapi     # Nppsys
)

target_include_directories(NppChineseConverter PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
    "${CMAKE_CURRENT_SOURCE_DIR}/nppsys"
    "${CMAKE_BINARY_DIR}/generated"
    "${STAGING_DIR}/include"
)