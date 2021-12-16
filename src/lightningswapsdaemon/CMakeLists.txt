project(lssd VERSION 0.4.4.0 LANGUAGES CXX)

# get revision
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

file(GLOB_RECURSE SOURCES *.cpp *.cc)
file(GLOB_RECURSE HEADERS *.h *.hpp)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/LssdConfig.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/LssdConfig.cpp" @ONLY)
list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/LssdConfig.cpp")

add_executable(lssd ${SOURCES} ${HEADERS})

target_link_libraries(lssd
    crashreporting
    lightningswaps)

set_target_properties(lssd
    PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/bin"
)

if(WIN32)
   set(LSSD_INSTALL_PROTO_DESTINATION "lssd/protos")
   set(LSSD_INSTALL_RUNTIME_DESTINATION "lssd")
else()
   set(LSSD_INSTALL_PROTO_DESTINATION "lssd/usr/include")
   set(LSSD_INSTALL_RUNTIME_DESTINATION "lssd/usr/bin")
endif()

install(TARGETS lssd
    RUNTIME DESTINATION ${LSSD_INSTALL_RUNTIME_DESTINATION}
    COMPONENT lssd)

install(DIRECTORY Protos/ DESTINATION ${LSSD_INSTALL_PROTO_DESTINATION}/
    COMPONENT lssd
    FILES_MATCHING PATTERN "*.proto")

if(WIN32)
    set(WIN_MODULES_DIR "${CMAKE_SOURCE_DIR}/../modules/win")
    #file(GLOB OPENSSL_DLLS "${WIN_MODULES_DIR}/openssl/bin/lib*.dll")
    #install(FILES ${OPENSSL_DLLS} ${ZLIB_DLL} DESTINATION ${LSSD_INSTALL_RUNTIME_DESTINATION}
    #COMPONENT lssd)
endif()

install(CODE "execute_process(COMMAND ${CMAKE_COMMAND}
    -DQMAKE=${QT_QMAKE_EXECUTABLE}
    -DAPP_EXE=${CMAKE_INSTALL_PREFIX}/${LSSD_INSTALL_RUNTIME_DESTINATION}/$<TARGET_FILE_NAME:lssd>
    -DDEPLOY_TOOL=${DEPLOY_TOOL}
    -P ${CMAKE_SOURCE_DIR}/../cmake/platformdeployqt.cmake)"
COMPONENT lssd)

install(CODE "execute_process(COMMAND ${CMAKE_COMMAND} -E tar c ${CMAKE_INSTALL_PREFIX}/lssd.zip --format=zip .
    WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/lssd)"
COMPONENT lssd)
