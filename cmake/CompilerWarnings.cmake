
# set_warnings(<target>)

# Applies a standard warning set to <target>. GCC and Clang only;
# other compilers are left untouched with a warning. Honors the
# WARNING_AS_ERRORS option set at the top level.


function(set_warnings target)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "^(GNU|Clang|AppleClang)$")
        message(WARNING
            "set_warnings: unsupported compiler '${CMAKE_CXX_COMPILER_ID}'; "
            "no warning flags applied to target '${target}'")
        return()
    endif()

    target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)

    if(WARNING_AS_ERRORS)
        target_compile_options(${target} PRIVATE -Werror)
    endif()

endfunction()
