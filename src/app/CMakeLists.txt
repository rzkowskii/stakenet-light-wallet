# get revision
include(GetGitRevisionDescription)
get_git_head_revision(GIT_REFSPEC GIT_SHA1)

file(GLOB SOURCES *.cpp)
file(GLOB_RECURSE HEADERS *.hpp)
file(GLOB_RECURSE QRC_FILES *.qrc)
file(GLOB_RECURSE QML_FILES *.qml)

add_custom_target(StakenetQML SOURCES ${QML_FILES})

option(DEFAULT_USE_STAGING "Use staging as default environment" OFF)

configure_file("${CMAKE_CURRENT_SOURCE_DIR}/StakenetConfig.cpp.in" "${CMAKE_CURRENT_BINARY_DIR}/StakenetConfig.cpp" @ONLY)
list(APPEND SOURCES "${CMAKE_CURRENT_BINARY_DIR}/StakenetConfig.cpp")

if(CMAKE_BUILD_TYPE MATCHES Release OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    find_package(Qt5QuickCompiler REQUIRED)
    file(GLOB_RECURSE CONNEXT_QRC connext_browser_node/*.qrc)
    list(REMOVE_ITEM QRC_FILES ${CONNEXT_QRC})
    qtquick_compiler_add_resources(RESOURCES ${QRC_FILES})
    qt5_add_resources(RESOURCES ${CONNEXT_QRC})
else()
    qt5_add_resources(RESOURCES ${QRC_FILES})
endif()

if(WIN32)
    list(APPEND RESOURCES assets/winResourse.rc)
    set(GUI_TYPE WIN32)
elseif(APPLE)
    set(GUI_TYPE MACOSX_BUNDLE)
endif()

add_executable(Stakenet ${GUI_TYPE} ${SOURCES} ${HEADERS} ${RESOURCES})

target_link_libraries(Stakenet
    crashreporting
    core
    Qt5::Quick
    Qt5::WebEngine
    Qt5::Svg
    CONAN_PKG::quazip)

target_compile_options(Stakenet PRIVATE
    $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
    -Wall>
    $<$<CXX_COMPILER_ID:MSVC>:
    /W4>)

set_target_properties(Stakenet
    PROPERTIES
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/assets/Info.plist"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/bin"
    )

set(STAKENET_BUNDLE_NAME "Stakenet.app")

file(GLOB LND_EXECUTABLES "${CMAKE_BINARY_DIR}/bin/lnd_*")
file(GLOB CRASH_HANDLER "${CMAKE_BINARY_DIR}/bin/crashpad_handler*")
file(GLOB STAKENET_UPDATER "${CMAKE_BINARY_DIR}/bin/updater*")
file(GLOB CHECKSUM_TOOL "${CMAKE_BINARY_DIR}/bin/checksum*")
file(GLOB RUNTIME_DLLS "${CMAKE_BINARY_DIR}/bin/runtime/*.dll")

if(WIN32)
    set(STAKENET_INSTALL_RUNTIME_DESTINATION "Stakenet")
    file(COPY ${LND_EXECUTABLES} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/bin")
elseif(APPLE)
    set(STAKENET_INSTALL_RUNTIME_DESTINATION "${STAKENET_BUNDLE_NAME}/Contents/MacOS")
    file(COPY ${LND_EXECUTABLES} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/${STAKENET_BUNDLE_NAME}/Contents/MacOS")
else()
    set(STAKENET_INSTALL_RUNTIME_DESTINATION "Stakenet/usr/bin")
    file(COPY ${LND_EXECUTABLES} DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/appdir/usr/bin")
endif()

install(TARGETS Stakenet
    BUNDLE DESTINATION "." COMPONENT Stakenet
    RUNTIME DESTINATION "${STAKENET_INSTALL_RUNTIME_DESTINATION}" COMPONENT Stakenet)

install(PROGRAMS ${LND_EXECUTABLES} ${CRASH_HANDLER} ${STAKENET_UPDATER}
    DESTINATION ${STAKENET_INSTALL_RUNTIME_DESTINATION}
    COMPONENT Stakenet)

if(WIN32)
    file(GLOB DLLS "${CMAKE_BINARY_DIR}/bin/lib*.dll")
    file(GLOB RUNTIME_DLLS "${CMAKE_BINARY_DIR}/bin/runtime/*.dll")
    #set(ZLIB_DLL "${WIN_MODULES_DIR}/zlib/bin/zlib.dll")
    install(FILES ${DLLS} ${RUNTIME_DLLS} DESTINATION ${STAKENET_INSTALL_RUNTIME_DESTINATION}
        COMPONENT Stakenet)
endif()

set(STAKENET_INSTALLED_EXE_PREFIX "${STAKENET_INSTALL_RUNTIME_DESTINATION}/$<TARGET_FILE_NAME:Stakenet>")

if(APPLE)
    set(STAKENET_INSTALLED_EXE_PREFIX "${STAKENET_BUNDLE_NAME}")
    install(FILES "${CMAKE_CURRENT_SOURCE_DIR}/assets/Stakenet_Icon_128.icns"
        DESTINATION ${STAKENET_INSTALL_RUNTIME_DESTINATION}/../Resources/
        COMPONENT Stakenet)
    #install(PROGRAMS ${CMAKE_BINARY_DIR}/bin/crashpad_handler DESTINATION ${STAKENET_BUNDLE_NAME}/Contents/MacOS
    #        COMPONENT Stakenet)
endif()

install(CODE "execute_process(COMMAND ${CMAKE_COMMAND}
    -DQMAKE=${QT_QMAKE_EXECUTABLE}
    -DAPP_EXE=\${CMAKE_INSTALL_PREFIX}/${STAKENET_INSTALLED_EXE_PREFIX}
    -DQML_DIR=${CMAKE_CURRENT_SOURCE_DIR}/qml
    -DDEPLOY_TOOL=${DEPLOY_TOOL}
    -P ${CMAKE_SOURCE_DIR}/../cmake/platformdeployqt.cmake)"
COMPONENT Stakenet)

set(UPDATE_DIR "${CMAKE_INSTALL_PREFIX}/update/${GIT_SHA1}")

install(CODE "execute_process(COMMAND ${CMAKE_COMMAND}
    -E make_directory ${UPDATE_DIR})"
COMPONENT Stakenet)

if(NOT WIN32 AND NOT APPLE)
    set(APP_EXE ${CMAKE_INSTALL_PREFIX}/${STAKENET_INSTALLED_EXE_PREFIX})
    install(CODE "include(${CMAKE_SOURCE_DIR}/../cmake/generate-symbols.cmake)
        generate_symbols(${UPDATE_DIR} ${BREAKPAD_DUMP_SYMS_EXE} ${APP_EXE})"
    COMPONENT Stakenet)
endif()

install(CODE "execute_process(COMMAND ${CMAKE_COMMAND}
    -DCMAKE_INSTALL_PREFIX=${CMAKE_INSTALL_PREFIX}
    -DGIT_SHA1=${GIT_SHA1}
    -DCHECKSUM_TOOL=${CHECKSUM_TOOL}
    -DAPP_EXE=${CMAKE_INSTALL_PREFIX}/${STAKENET_INSTALLED_EXE_PREFIX}
    -DDUMP_SYMS=${BREAKPAD_DUMP_SYMS_EXE}
    -DSTRIP_TOOL=${CMAKE_STRIP}
    -P ${CMAKE_SOURCE_DIR}/../cmake/prepare-update.cmake
    WORKING_DIRECTORY ${UPDATE_DIR})"
COMPONENT Stakenet)

if(APPLE)
    # since we use cpack to create dmg we need to make sure that we have strip our symbols
    install(CODE "execute_process(COMMAND ${CMAKE_STRIP} \${CMAKE_INSTALL_PREFIX}/${STAKENET_INSTALL_RUNTIME_DESTINATION}/Stakenet)"
        COMPONENT Stakenet)

    install(CODE "execute_process(COMMAND ${CMAKE_STRIP} \${CMAKE_INSTALL_PREFIX}/${UPDATER_INSTALLED_EXE})"
        COMPONENT Stakenet)

    set(CPACK_PACKAGE_NAME "Stakenet")
    set(CPACK_PACKAGE_VERSION_MAJOR "${Stakenet_VERSION_MAJOR}")
    set(CPACK_PACKAGE_VERSION_MINOR "${Stakenet_VERSION_MINOR}")
    set(CPACK_PACKAGE_VERSION_PATCH "${Stakenet_VERSION_PATCH}")
    set(CPACK_PACKAGE_VERSION "${Stakenet_VERSION}")
    set(CPACK_GENERATOR "DragNDrop")
    set(CPACK_DMG_FORMAT "UDBZ")
    set(CPACK_DMG_VOLUME_NAME "Stakenet")
    set(CPACK_SYSTEM_NAME "OSX")
    set(CPACK_COMPONENTS_ALL "Stakenet")
    set(CPACK_PACKAGE_FILE_NAME "Stakenet")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/assets/Stakenet_Icon_128.icns")
    #    set(CPACK_DMG_DS_STORE "${ICONS_DIR}/CharmDSStore")
    #    set(CPACK_DMG_BACKGROUND_IMAGE "${ICONS_DIR}/CharmDMGBackground.png")
endif()

include(CPack)
