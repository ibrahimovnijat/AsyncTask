find_package(GTest CONFIG QUIET)

if(NOT GTest_FOUND)
    message(STATUS "GTest not found via find_package; fetching with FetchContent")
    include(FetchContent)
    FetchContent_Declare(googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.17.0.tar.gz
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
        SYSTEM
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
    set(BUILD_GMOCK OFF CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(googletest)
endif()

include(GoogleTest)

add_executable(tasklib_tests
    tasklib/CommonTest.cpp
)

target_link_libraries(tasklib_tests
    PRIVATE
        tasklib
        GTest::gtest_main
)

set_warnings(tasklib_tests)
gtest_discover_tests(tasklib_tests)
