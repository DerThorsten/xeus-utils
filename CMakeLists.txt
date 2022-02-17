#############################################################################
#Copyright (c) 2022,                                          
#                                                                         
#Distributed under the terms of the BSD 3-Clause License.                 
#                                                                         
#The full license is in the file LICENSE, distributed with this software. 
#############################################################################

cmake_minimum_required(VERSION 3.4.3)
project(xeus-utils)
set(CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake;${CMAKE_MODULE_PATH}")

set(XEUS_UTILS_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Versionning
# ===========

file(STRINGS "${XEUS_UTILS_INCLUDE_DIR}/xeus-utils/xeus_utils_config.hpp" version_defines
     REGEX "#define XEUS_UTILS_VERSION_(MAJOR|MINOR|PATCH)")
foreach (ver ${version_defines})
    if (ver MATCHES "#define XEUS_UTILS_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(XEUS_UTILS_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif ()
endforeach ()
set(${PROJECT_NAME}_VERSION
    ${XEUS_UTILS_VERSION_MAJOR}.${XEUS_UTILS_VERSION_MINOR}.${XEUS_UTILS_VERSION_PATCH})
message(STATUS "Building xeus-utils v${${PROJECT_NAME}_VERSION}")

# Configuration
# =============

include(GNUInstallDirs)

if (NOT DEFINED XEUS_UTILS_KERNELSPEC_PATH)
    set(XEUS_UTILS_KERNELSPEC_PATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/")
endif ()



configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xutils/kernel.json.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels/xutils/kernel.json"
)

# Build options
# =============

OPTION(XEUS_UTILS_DISABLE_ARCH_NATIVE "disable -march=native flag" OFF)
OPTION(XEUS_UTILS_DISABLE_TUNE_GENERIC "disable -mtune=generic flag" OFF)

option(XEUS_UTILS_BUILD_STATIC "Build xeus-utils static library" ON)
OPTION(XEUS_UTILS_BUILD_SHARED "Split xutils build into executable and library" ON)
OPTION(XEUS_UTILS_BUILD_EXECUTABLE "Build the xutils executable" ON)

OPTION(XEUS_UTILS_USE_SHARED_XEUS "Link xutils  with the xeus shared library (instead of the static library)" ON)
OPTION(XEUS_UTILS_USE_SHARED_XEUS_UTILS "Link xutils  with the xeus shared library (instead of the static library)" ON)

OPTION(XEUS_UTILS_EMSCRIPTEN_WASM_BUILD "Build for wasm with emscripten" OFF)


if(XEUS_UTILS_EMSCRIPTEN_WASM_BUILD)
    add_compile_definitions(XEUS_UTILS_EMSCRIPTEN_WASM_BUILD)
    message("Build with emscripten")
    SET(XEUS_UTILS_DISABLE_ARCH_NATIVE ON)
    SET(XEUS_UTILS_DISABLE_TUNE_GENERIC ON)
    SET(XEUS_UTILS_BUILD_STATIC ON)
    SET(XEUS_UTILS_BUILD_SHARED OFF)
    SET(XEUS_UTILS_BUILD_EXECUTABLE OFF)
    SET(XEUS_UTILS_USE_SHARED_XEUS OFF)
    SET(XEUS_UTILS_USE_SHARED_XEUS_UTILS OFF)
    SET(XEUS_UTILS_BUILD_TESTS OFF)
endif()

# Dependencies
# ============
set(xtl_REQUIRED_VERSION 0.6.23)
set(xeus_REQUIRED_VERSION 2.0.0)

# for the emscripten build boost
# is included via some magic linker flag
if(NOT XEUS_UTILS_EMSCRIPTEN_WASM_BUILD)
    find_package(Boost REQUIRED)
else()
    set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} -s USE_BOOST_HEADERS=1")
endif()

find_package(CLI11 REQUIRED)

if (NOT TARGET xtl)
    find_package(xtl ${xtl_REQUIRED_VERSION} REQUIRED)
endif ()
if (NOT TARGET xeus AND NOT TARGET xeus-static)
    find_package(xeus ${xeus_REQUIRED_VERSION} REQUIRED)
endif ()



# Flags
# =====
include(CheckCXXCompilerFlag)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4251 /wd4141")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4018 /wd4267 /wd4715 /wd4146 /wd4129")
endif ()

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")

    if(NOT XEUS_UTILS_EMSCRIPTEN_WASM_BUILD)    
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-parameter -Wextra -Wreorder")
    endif()
    

    if (XEUS_UTILS_DISABLE_ARCH_NATIVE AND NOT XEUS_UTILS_DISABLE_TUNE_GENERIC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -mtune=generic")
    elseif (XEUS_UTILS_DISABLE_TUNE_GENERIC)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
    else ()
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
    endif ()


    CHECK_CXX_COMPILER_FLAG("-std=c++17" HAS_CPP_17_FLAG)
    if (HAS_CPP_17_FLAG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")
    else ()
        message(FATAL_ERROR "Unsupported compiler -- xeus requires C++17 support!")
    endif ()

endif ()

if(XEUS_UTILS_EMSCRIPTEN_WASM_BUILD)

    include(XeusEmscriptenWasmFlags)


    # fetch
    set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} -s  FETCH=1  -s NO_EXIT_RUNTIME=1")
    set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} -s USE_ZLIB=1")

    # async
    set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} --bind")
    set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} -s ASYNCIFY=1")
    #set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} -s 'ASYNCIFY_IMPORTS=[\"async_get_input_function\"]'")
    set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} -s 'ASYNCIFY_STACK_SIZE=20480'")

    # ENV (https://github.com/emscripten-core/emscripten/commit/6d9681ad04f60b41ef6345ab06c29bbc9eeb84e0)
    set(EMSCRIPTEN_FEATURES "${EMSCRIPTEN_FEATURES} -s \"EXTRA_EXPORTED_RUNTIME_METHODS=[ENV']\"")

    # filesystem
    #file(COPY asset_dir DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
    #set(XEUS_EMSCRIPTEN_WASM_FLAGS "${XEUS_EMSCRIPTEN_WASM_FLAGS} --embed-file asset_dir")

    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${XEUS_EMSCRIPTEN_WASM_FLAGS}")
    set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS}  ${XEUS_EMSCRIPTEN_WASM_FLAGS} ---proxy-to-worker")
endif()



# Source files
# ============

set(XEUS_UTILS_HEADERS
    include/xeus-utils/xeus_utils_config.hpp
    include/xeus-utils/xinterpreter.hpp
)

set(XEUS_UTILS_SRC
    src/xinterpreter.cpp 
)

set(XEUS_UTILS_MAIN_SRC
    src/main.cpp
)


# Targets and link - Macros
# =========================

include(CheckCXXCompilerFlag)

string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib; ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")

macro(xeus_utils_set_common_options target_name)
    if (MSVC)
        target_compile_options(${target_name} PUBLIC /wd4251 /wd4141)
        target_compile_options(${target_name} PUBLIC /wd4018 /wd4267 /wd4715 /wd4146 /wd4129)
    endif ()

    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR
        CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR
        CMAKE_CXX_COMPILER_ID MATCHES "Intel")

        target_compile_options(${target_name} PUBLIC -Wunused-parameter -Wextra -Wreorder)

        # Mtune generic/native
        if (XEUS_UTILS_DISABLE_ARCH_NATIVE AND NOT XEUS_UTILS_DISABLE_TUNE_GENERIC)
            target_compile_options(${target_name} PUBLIC -mtune=generic)
        elseif (XEUS_UTILS_DISABLE_TUNE_GENERIC)
        else ()
            target_compile_options(${target_name} PUBLIC -march=native)
        endif ()

        # C++17 flag
        CHECK_CXX_COMPILER_FLAG("-std=c++17" HAS_CPP_17_FLAG)
        if (HAS_CPP_17_FLAG)
            target_compile_features(${target_name} PRIVATE cxx_std_17)
        else ()
            message(FATAL_ERROR "Unsupported compiler -- xeus-utils requires C++17 support!")
        endif ()
    endif ()

    if (APPLE)
        set_target_properties(${target_name} PROPERTIES
            MACOSX_RPATH ON
        )
    else ()
        set_target_properties(${target_name} PROPERTIES
            BUILD_WITH_INSTALL_RPATH 1
            SKIP_BUILD_RPATH FALSE
        )
    endif ()

    set_target_properties(${target_name} PROPERTIES
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
endmacro()

# Common macro kernels (xutils )
macro(xeus_utils_set_kernel_options target_name)
    if (XEUS_UTILS_USE_SHARED_XEUS_UTILS)
        target_link_libraries(${target_name} PRIVATE xeus-utils)
        if(CMAKE_DL_LIBS)
            target_link_libraries(${target_name} PRIVATE ${CMAKE_DL_LIBS} util)
        endif()
    else ()
        target_link_libraries(${target_name} PRIVATE xeus-utils-static)
    endif()

    if(NOT XEUS_UTILS_EMSCRIPTEN_WASM_BUILD)
        find_package(Threads) 
        target_link_libraries(${target_name} PRIVATE ${CMAKE_THREAD_LIBS_INIT})
    endif()
    

endmacro()

# Common macro for shared and static library
macro(xeus_utils_create_target target_name linkage output_name)
    string(TOUPPER "${linkage}" linkage_upper)

    if (NOT ${linkage_upper} MATCHES "^(SHARED|STATIC)$")
        message(FATAL_ERROR "Invalid library linkage: ${linkage}")
    endif ()

    add_library(${target_name} ${linkage_upper} ${XEUS_UTILS_SRC} ${XEUS_UTILS_HEADERS})
    xeus_utils_set_common_options(${target_name})

    set_target_properties(${target_name} PROPERTIES
                          PUBLIC_HEADER "${XEUS_UTILS_HEADERS}"
                          PREFIX ""
                          VERSION ${${PROJECT_NAME}_VERSION}
                          SOVERSION ${XEUS_UTILS_VERSION_MAJOR}
                          OUTPUT_NAME "lib${output_name}")

    target_compile_definitions(${target_name} PUBLIC "XEUS_UTILS_EXPORTS")

    target_compile_features(${target_name} PRIVATE cxx_std_17)

    target_include_directories(${target_name}
                               PUBLIC
                               $<BUILD_INTERFACE:${XEUS_UTILS_INCLUDE_DIR}>
                               $<INSTALL_INTERFACE:include>)

    if (XEUS_UTILS_USE_SHARED_XEUS)
        set(XEUS_UTILS_XEUS_TARGET xeus)
    else ()
        set(XEUS_UTILS_XEUS_TARGET xeus-static)
    endif ()

    if(XEUS_UTILS_EMSCRIPTEN_WASM_BUILD)
        target_link_libraries(${target_name} PUBLIC ${XEUS_UTILS_XEUS_TARGET} xtl CLI11::CLI11)
    else()
        target_link_libraries(${target_name} PUBLIC ${XEUS_UTILS_XEUS_TARGET} xtl CLI11::CLI11 Boost::boost)
    endif()

    if (WIN32 OR CYGWIN)
        # 
    elseif (APPLE)
        target_link_libraries(${target_name} PRIVATE "-undefined dynamic_lookup")
    endif ()

    if(NOT XEUS_UTILS_EMSCRIPTEN_WASM_BUILD)
        find_package(Threads) # TODO: add Threads as a dependence of xeus-static?
        target_link_libraries(${target_name} PRIVATE ${CMAKE_THREAD_LIBS_INIT})
    endif()
    

endmacro()

# xeus-utils
# ===========

set(XEUS_UTILS_TARGETS "")

if (XEUS_UTILS_BUILD_SHARED)
    # Build libraries
    xeus_utils_create_target(xeus-utils SHARED xeus-utils)
    list(APPEND XEUS_UTILS_TARGETS xeus-utils)
endif ()

if (XEUS_UTILS_BUILD_STATIC)
    # On Windows, a static library should use a different output name
    # to avoid the conflict with the import library of a shared one.
    if (CMAKE_HOST_WIN32)
        xeus_utils_create_target(xeus-utils-static STATIC xeus-utils-static)
    else ()
        xeus_utils_create_target(xeus-utils-static STATIC xeus-utils)
    endif ()
    list(APPEND XEUS_UTILS_TARGETS xeus-utils-static)
endif ()

# xeus-utils
# =======
if (XEUS_UTILS_BUILD_EXECUTABLE)
    add_executable(xutils ${XEUS_UTILS_MAIN_SRC})
    target_compile_features(xutils PRIVATE cxx_std_17)
    xeus_utils_set_common_options(xutils)
    xeus_utils_set_kernel_options(xutils)
endif()

if(XEUS_UTILS_EMSCRIPTEN_WASM_BUILD)
    set(XEUS_WASM_INSTALL_DIR "/home/" CACHE STRING "install path for wasm files")
    add_executable(xeus_kernel src/main_emscripten_kernel.cpp )
    target_link_libraries(xeus_kernel  xeus-utils-static)
    target_compile_features(xeus_kernel PRIVATE cxx_std_17)
    # install(FILES
    #     "$<TARGET_FILE_DIR:xeus_kernel>/xeus_kernel.js"
    #     "$<TARGET_FILE_DIR:xeus_kernel>/xeus_kernel.wasm"
    #     DESTINATION ${XEUS_WASM_INSTALL_DIR})


endif()


# Installation
# ============
include(CMakePackageConfigHelpers)

set(XEUS_UTILS_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE STRING "install path for xeus-utilsConfig.cmake")



# Install xeus-utils and xeus-utils-static
if (XEUS_UTILS_BUILD_SHARED)
    install(TARGETS ${XEUS_UTILS_TARGETS}
            EXPORT ${PROJECT_NAME}-targets
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/xeus-utils)

    # Makes the project importable from the build directory
    export(EXPORT ${PROJECT_NAME}-targets
           FILE "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Targets.cmake")
endif ()

# Install xutils
if (XEUS_UTILS_BUILD_EXECUTABLE)
    install(TARGETS xutils
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

    # Configuration and data directories for jupyter and xeus-utils
    set(XJUPYTER_DATA_DIR "share/jupyter"    CACHE STRING "Jupyter data directory")

    # Install xutils Jupyter kernelspec
    set(KERNELSPEC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/share/jupyter/kernels)
    install(DIRECTORY ${KERNELSPEC_DIR}
            DESTINATION ${XJUPYTER_DATA_DIR}
            PATTERN "*.in" EXCLUDE)


    # Extra path for installing Jupyter kernelspec
    if (XEXTRA_JUPYTER_DATA_DIR)
        install(DIRECTORY ${KERNELSPEC_DIR}
                DESTINATION ${XEXTRA_JUPYTER_DATA_DIR}
                PATTERN "*.in" EXCLUDE)
    endif ()
endif ()


# install xeus wasm
if(XEUS_UTILS_EMSCRIPTEN_WASM_BUILD)


    install(TARGETS xeus_kernel
        ARCHIVE DESTINATION ${CMAKE_INSTALL_BINDIR})


endif()



# Configure 'xeus-utilsConfig.cmake' for a build tree
set(XEUS_UTILS_CONFIG_CODE "####### Expanded from \@XEUS_UTILS_CONFIG_CODE\@ #######\n")
set(XEUS_UTILS_CONFIG_CODE "${XEUS_UTILS_CONFIG_CODE}set(CMAKE_MODULE_PATH \"${CMAKE_CURRENT_SOURCE_DIR}/cmake;\${CMAKE_MODULE_PATH}\")\n")
set(XEUS_UTILS_CONFIG_CODE "${XEUS_UTILS_CONFIG_CODE}##################################################")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${PROJECT_BINARY_DIR})

# Configure 'xeus-utilsConfig.cmake.in for an install tree
set(XEUS_UTILS_CONFIG_CODE "")
configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${XEUS_UTILS_CMAKECONFIG_INSTALL_DIR})

write_basic_package_version_file(${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
                                 VERSION ${${PROJECT_NAME}_VERSION}
                                 COMPATIBILITY AnyNewerVersion)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/CMakeFiles/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              DESTINATION ${XEUS_UTILS_CMAKECONFIG_INSTALL_DIR})

if (XEUS_UTILS_BUILD_SHARED)
    install(EXPORT ${PROJECT_NAME}-targets
            FILE ${PROJECT_NAME}Targets.cmake
            DESTINATION ${XEUS_UTILS_CMAKECONFIG_INSTALL_DIR})
endif ()
