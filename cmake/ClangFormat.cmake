
set(OPTION_CLANG_FORMAT_ENABLED Off)

# Function to register a target for formatting (if enabled)
function(add_clang_format_target target)
    if(NOT OPTION_CLANG_FORMAT_ENABLED)
        return()
    endif()

    if(NOT TARGET format-all)
        add_custom_target(format-all)

        set_target_properties(format-all
            PROPERTIES
            FOLDER "Maintenance"
            EXCLUDE_FROM_DEFAULT_BUILD 1
        )
    endif()

    add_custom_target(
        format-${target}
        COMMAND
            ${clang_format_EXECUTABLE}
                -i
                -style=file
                ${ARGN}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    set_target_properties(format-${target}
        PROPERTIES
        FOLDER "Maintenance"
        EXCLUDE_FROM_DEFAULT_BUILD 1
    )

    add_dependencies(format-all format-${target})
endfunction()

# Enable or disable clang-format
function(enable_clang_format status)
    if(NOT ${status})
        set(OPTION_CLANG_FORMAT_ENABLED ${status} PARENT_SCOPE)
        message(STATUS "Clang-format skipped: Manually disabled")

        return()
    endif()

    find_package(clang_format)

    if(NOT clang_format_FOUND)
        set(OPTION_CLANG_TIDY_ENABLED Off PARENT_SCOPE)
        message(STATUS "Clang-format skipped: Not found")

        return()
    endif()

    set(OPTION_CLANG_FORMAT_ENABLED ${status} PARENT_SCOPE)
    message(STATUS "Clang-format enabled")
endfunction()
