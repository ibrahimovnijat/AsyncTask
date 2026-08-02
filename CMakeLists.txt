cmake_minimum_required(VERSION 4.2)
project(pix4d_asyncTask
    VERSION 0.1.0
    LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


# Static libs stay linkable into future shared libs/plugins
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Include reusable cmake functions
list(APPEND_CMAKE_MODULE_PATH, ${PROJECT_SOURCE_DIR}/cmake)
include(AddApp)
include(CompilerWarning)

# Add configuration/build options
option(BUILD_TESTS "Build the gtests" OFF)
option(WARNING_AS_ERRORS "Treat compiler warnings as errors" ON)
option(BUILD_ASYNCTASK_CLI "Build the asynctask_cli app" ON)
# option(INSTALL "Generate install/export rules for libs" OFF) # redundant?

# Speed up rebuilds when ccache is available; harmless otherwise.
find_program(CCACHE_PROGRAM ccache)
if(CCACHE_PROGRAM)
    set(CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_PROGRAM})
endif()

# # Optional sanitizer instrumentation for the whole tree, e.g.:
# #   cmake -S . -B build-tsan -DTASKLIB_SANITIZER=thread
# set(TASKLIB_SANITIZER "" CACHE STRING "Optional sanitizer: thread | address | undefined")
# if(TASKLIB_SANITIZER)
#     add_compile_options(-fsanitize=${TASKLIB_SANITIZER} -g -fno-omit-frame-pointer)
#     add_link_options(-fsanitize=${TASKLIB_SANITIZER})
# endif()


if(BUILD_ASYNCTASK_CLI)
    add_subdirectory(asynctask_cli)
endif()

if(BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
