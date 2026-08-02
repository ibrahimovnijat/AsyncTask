set(TASKLIB_HEADERS
    include/tasklib/types.hpp
    include/tasklib/taskControl.hpp
    include/tasklib/taskManager.hpp)

set(TASK_LIB_SOURCES
    src/taskControl.cpp
    src/taskManager.cpp)

add_library(tasklib STATIC 
        ${TASKLIB_HEADERS}
        ${TASK_LIB_SOURCES})

# Consumers include headers as: #include <tasklib/common.hpp>
target_include_directories(tasklib
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

set_warnings(tasklib)

