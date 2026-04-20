# Shared host build compile options.

include_guard(GLOBAL)

function(blusys_host_apply_compile_options target)
    target_compile_options(${target} PRIVATE
        $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-rtti>
        $<$<COMPILE_LANGUAGE:CXX>:-fno-threadsafe-statics>
        -Wall
        -Wextra
    )
endfunction()

function(blusys_host_apply_warning_options target)
    target_compile_options(${target} PRIVATE
        -Wall
        -Wextra
    )
endfunction()
