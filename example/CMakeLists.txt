add_executable(arcfour_example example.c)

if(ARCFOUR_STATIC_ONLY)
    target_link_libraries(arcfour_example PRIVATE arcfour_static)
    target_compile_definitions(arcfour_example PRIVATE ARCFOUR_STATIC ARCFOUR_STATIC_ONLY)
else()
    target_link_libraries(arcfour_example PRIVATE arcfour_shared)
    
    if(WIN32)
        add_custom_command(TARGET arcfour_example POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:arcfour_shared>
            $<TARGET_FILE_DIR:arcfour_example>)
    else()
        set_target_properties(arcfour_example PROPERTIES
            INSTALL_RPATH "$ORIGIN/../lib"
            BUILD_WITH_INSTALL_RPATH ON)
    endif()
endif()

install(TARGETS arcfour_example
        RUNTIME DESTINATION bin)