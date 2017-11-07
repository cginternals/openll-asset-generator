
#
# Configuration for all sub-projects
#

# Generate version-header
configure_file(version.h.in ${CMAKE_CURRENT_BINARY_DIR}/include/${META_PROJECT_NAME}/${META_PROJECT_NAME}-version.h)


#
# Sub-projects
#

# Libraries
set(IDE_FOLDER "")
add_subdirectory(llassetgen)

# Applications
set(IDE_FOLDER "Applications")
add_subdirectory(llassetgen-cmd)
# add_subdirectory(llassetgen-server)
# add_subdirectory(llassetgen-client)

# Tests
set(IDE_FOLDER "Tests")
add_subdirectory(tests)


#
# Deployment
#

# Deploy generated headers
install(DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/include/${META_PROJECT_NAME} DESTINATION include COMPONENT dev)
