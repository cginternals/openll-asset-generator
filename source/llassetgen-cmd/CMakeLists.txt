#
# External dependencies
#


#
# Executable name and options
#

# Target name
set(target llassetgen-cmd)

# Exit here if required dependencies are not met
message(STATUS "Application ${target}")


#
# Sources
#

set(include_path "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(source_path  "${CMAKE_CURRENT_SOURCE_DIR}/source")

set(headers
    ${include_path}/CLI11.h
    ${include_path}/algorithms.h
    ${include_path}/helpstrings.h
    ${include_path}/presets.h)

set(sources
    ${source_path}/main.cpp)

#
# Create executable
#

# Build executable
add_executable(${target}
    MACOSX_BUNDLE
    ${headers}
    ${sources}
)

# Create namespaced alias
add_executable(${META_PROJECT_NAME}::${target} ALIAS ${target})


#
# Project options
#

set_target_properties(${target}
    PROPERTIES
    ${DEFAULT_PROJECT_OPTIONS}
    FOLDER "${IDE_FOLDER}"
)


#
# Include directories
#

target_include_directories(${target}
    PRIVATE
    ${DEFAULT_INCLUDE_DIRECTORIES}
    ${PROJECT_BINARY_DIR}/source/include
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)


#
# Libraries
#

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LIBRARIES}
    ${META_PROJECT_NAME}::llassetgen
)


#
# Compile definitions
#

target_compile_definitions(${target}
    PRIVATE
    ${DEFAULT_COMPILE_DEFINITIONS}
)


#
# Compile options
#

target_compile_options(${target}
    PRIVATE
    ${DEFAULT_COMPILE_OPTIONS}
)


#
# Linker options
#

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LINKER_OPTIONS}
)


#
# Target Health
#

perform_health_checks(
    ${target}
    ${sources}
    ${headers}
)


#
# Source Code Formatting
#

add_clang_format_target(${target} ${sources} ${headers})


#
# Deployment
#

# Executable
install(TARGETS ${target}
    RUNTIME DESTINATION ${INSTALL_BIN} COMPONENT runtime
    BUNDLE  DESTINATION ${INSTALL_BIN} COMPONENT runtime
)
