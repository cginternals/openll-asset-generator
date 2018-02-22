#
# External dependencies
#

find_package(glm REQUIRED)
find_package(glbinding REQUIRED)
find_package(globjects REQUIRED)

find_package(Qt5Core 5.2 REQUIRED)
find_package(Qt5Gui 5.2 REQUIRED)
find_package(Qt5Widgets 5.2 REQUIRED)
find_package(Qt5OpenGL 5.2 REQUIRED)


#
# Executable name and options
#

# Target name
set(target llassetgen-rendering)

# Exit here if required dependencies are not met
message(STATUS "Application ${target}")


#
# Sources
#

set(CMAKE_AUTOMOC TRUE)

set(sources
    main.cpp
	
	WindowQt.cpp
)

set(headers
	WindowQt.h
)

#
# Create executable
#

# Build executable
add_executable(${target}
    MACOSX_BUNDLE
    ${sources}
	${headers}
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
)


#
# Libraries
#

target_link_libraries(${target}
    PRIVATE
    ${DEFAULT_LIBRARIES}
    ${META_PROJECT_NAME}::llassetgen
	Qt5::Core
	Qt5::OpenGL
	Qt5::Gui
	Qt5::Widgets
	
	PUBLIC
	glbinding::glbinding
	globjects::globjects
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
