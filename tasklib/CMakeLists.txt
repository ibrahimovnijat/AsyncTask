set(TASKLIB_HEADERS
    include/tasklib/common.hpp)

set(TASK_LIB_SOURCES
    src/common.cpp)

add_library(tasklib STATIC 
        ${TASKLIB_HEADERS}
        ${TASK_LIB_SOURCES})
# add_library(PIX4D_task::tasklib ALIAS tasklib) // Redundant?

# Consumers include headers as: #include <tasklib/common.hpp>
target_include_directories(tasklib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_warnings(tasklib)

