set(ASYNC_CLI_SOURCES
    cli/command.cpp
    cli/sampleTasks.cpp
    main.cpp)

add_app(asynctask_cli
    SOURCES ${ASYNC_CLI_SOURCES}
    LIBS tasklib
)
