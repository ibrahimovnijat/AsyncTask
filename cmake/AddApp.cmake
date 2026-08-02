# cppframework_add_app(<name> SOURCES <src...> [LIBS <lib...>])
#
# Declares an application executable with the project's standard warning set
# and links it against the given CppFramework:: libraries, e.g.:
#   add_app(mytool
#       SOURCES main.cpp
#       LIBS <LIB_NAME>
#   )
function(add_app name)
    cmake_parse_arguments(ARG "" "" "SOURCES;LIBS" ${ARGN})

    if(NOT ARG_SOURCES)
        message(FATAL_ERROR "add_app(${name}): SOURCES is required")
    endif()

    add_executable(${name} ${ARG_SOURCES})

    if(ARG_LIBS)
        target_link_libraries(${name} PRIVATE ${ARG_LIBS})
    endif()

    set_warnings(${name})
    
endfunction()
