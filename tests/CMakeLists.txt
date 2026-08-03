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

add_executable(unit_tests
    asynctask_cli/commandParserTest.cpp
    tasklib/taskManagerTest.cpp
    # The CLI parser is part of the executable, not a library, so compile the
    # unit under test straight into the test binary.
    ${PROJECT_SOURCE_DIR}/asynctask_cli/cli/command.cpp
)

target_link_libraries(unit_tests
    PRIVATE
        tasklib
        GTest::gtest_main
)

set_warnings(unit_tests)
gtest_discover_tests(unit_tests)
