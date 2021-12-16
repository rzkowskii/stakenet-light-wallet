if((NOT DEFINED DEPLOY_TOOL) OR (DEPLOY_TOOL STREQUAL ""))
    message(FATAL_ERROR "No deploy tool specified, use -DDEPLOY_TOOL")
endif()

if(NOT DEFINED QMAKE)
    message(FATAL_ERROR "No qmake specified, use -DQMAKE")
endif()

if(NOT DEFINED APP_EXE)
    message(FATAL_ERROR "No executable specified, use -DAPP_EXE")
endif()

if(WIN32)
    if(NOT DEFINED QML_DIR)
        execute_process(COMMAND ${DEPLOY_TOOL} --release --compiler-runtime "${APP_EXE}")
    else()
        execute_process(COMMAND ${DEPLOY_TOOL} --qmldir=${QML_DIR} --release --compiler-runtime "${APP_EXE}")
    endif()
elseif(APPLE)
    if(NOT DEFINED QML_DIR)
        message(STATUS "${DEPLOY_TOOL} ${APP_EXE}")
        execute_process(COMMAND ${DEPLOY_TOOL} ${APP_EXE})
    else()
        execute_process(COMMAND ${DEPLOY_TOOL} ${APP_EXE} -qmldir=${QML_DIR})
    endif()
else()
    set(EXCLUDE_LIST "libnspr4.so,libnss3.so,libnssutil3.so,libsmime3")
    if(NOT DEFINED QML_DIR)
        execute_process(COMMAND ${DEPLOY_TOOL} ${APP_EXE} -qmake=${QMAKE} -appimage -unsupported-allow-new-glibc -exclude-libs=${EXCLUDE_LIST})
    else()
        execute_process(COMMAND ${DEPLOY_TOOL} ${APP_EXE} -qmake=${QMAKE} -appimage -qmldir=${QML_DIR} -unsupported-allow-new-glibc -exclude-libs=${EXCLUDE_LIST})
    endif()
endif()