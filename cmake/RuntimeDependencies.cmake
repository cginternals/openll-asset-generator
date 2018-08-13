
# 
# Default dependencies for the runtime-package
# 

# Install 3rd-party runtime dependencies into runtime-component
# install(FILES ... COMPONENT runtime)


# 
# Full dependencies for self-contained packages
# 

if(OPTION_SELF_CONTAINED)

    # Install 3rd-party runtime dependencies into runtime-component
    # install(FILES ... COMPONENT runtime)
    
    find_file(DLLS_CPPLOCATE cpplocate.dll)
    find_file(DLLS_GLBINDING glbinding.dll)
    find_file(DLLS_GLBINDING_AUX glbinding-aux.dll)
    find_file(DLLS_GLOBJECTS globjects.dll)
    find_file(DLLS_LIBPNG libpng16.dll)
    find_file(DLLS_ZLIB zlib.dll)
    find_file(DLLS_QT5CORE qt5core.dll)
    find_file(DLLS_QT5GUI qt5gui.dll)
    find_file(DLLS_QT5WIDGETS qt5widgets.dll)
    find_file(DLLS_MSVCRT msvcrt.dll)
    find_file(DLLS_MSVCP114 msvcp140.dll) 
    find_file(DLLS_VCRUNTIME vcruntime140.dll)


    set(OPTION_DEPLOY_DEBUG_DLLS FALSE CACHE BOOL "if set, debug versions of certain dlls will be packaged")
    if (OPTION_DEPLOY_DEBUG_DLLS)
        find_file(DLLS_MSVCP114D msvcp114d.dll)
        find_file(DLLS_MSVCR114D msvcr114d.dll)
    else()
    endif()

    set(DLLS
        ${DLLS_CPPLOCATE}
        ${DLLS_GLBINDING}
        ${DLLS_GLBINDING_AUX}
        ${DLLS_GLOBJECTS}
        ${DLLS_LIBPNG}
        ${DLLS_ZLIB}
        ${DLLS_QT5CORE}
        ${DLLS_QT5GUI}
        ${DLLS_QT5WIDGETS}
        ${DLLS_MSVCRT}	
        ${DLLS_MSVCP114}
        ${DLLS_MSVCP114D}
        ${DLLS_VCRUNTIME}
    )

    install(FILES ${DLLS} DESTINATION ${INSTALL_BIN} COMPONENT runtime)

endif()
