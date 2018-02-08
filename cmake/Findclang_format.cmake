
# Findclang_format results:
# clang_format_FOUND
# clang_format_EXECUTABLE

include(FindPackageHandleStandardArgs)

find_program(clang_format_EXECUTABLE
    NAMES
        clang-format
    PATHS
        "${CLANG_FORMAT_DIR}"
)

find_package_handle_standard_args(clang_format
    FOUND_VAR
        clang_format_FOUND
    REQUIRED_VARS
        clang_format_EXECUTABLE
)

mark_as_advanced(clang_format_EXECUTABLE)
