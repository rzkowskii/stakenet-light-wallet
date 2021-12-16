add_executable(checksum main.cpp)
target_link_libraries(checksum libupdater)

if(WIN32)
    install(CODE "execute_process(COMMAND ${CMAKE_COMMAND}
        -DQMAKE=${QT_QMAKE_EXECUTABLE}
        -DAPP_EXE=$<TARGET_FILE:checksum>
        -DDEPLOY_TOOL=${DEPLOY_TOOL}
        -P ${CMAKE_SOURCE_DIR}/../cmake/platformdeployqt.cmake)"
    COMPONENT Stakenet)
endif()
