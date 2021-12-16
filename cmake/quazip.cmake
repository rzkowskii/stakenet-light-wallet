set(QUAZIP_INSTALL_LOCATION "${MODULES_INSTALL_DIR}/quazip")
option(BUILD_quazip "Build QUAZIP as part of build process" ON)

if(BUILD_EXTERNAL AND BUILD_quazip)
    ExternalProject_Add(quazip
        PREFIX quazip
        GIT_REPOSITORY https://github.com/stachenov/quazip.git
        UPDATE_COMMAND ""
        CMAKE_CACHE_ARGS
            -DZLIB_ROOT:PATH=${ZLIB_ROOT}
            -DCMAKE_PREFIX_PATH:PATH=${CMAKE_PREFIX_PATH}
            -DCMAKE_BUILD_TYPE:STRING=Release
            -DCMAKE_INSTALL_PREFIX:PATH=${QUAZIP_INSTALL_LOCATION}
        DEPENDS zlib
        )

    if(NOT MSVC)
        ExternalProject_Add_Step(quazip patch-sources
            COMMAND patch -N -r - -d <SOURCE_DIR>/quazip < ${CMAKE_CURRENT_LIST_DIR}/patches/quazip/cmake-config-file.patch || true
            COMMENT "Patch sources"
            DEPENDEES download
            DEPENDERS configure)
    endif()
else()
#    set(quazip_INCLUDE_DIRS "${QUAZIP_INSTALL_LOCATION}/include/quazip5")

#    if(WIN32)
#        set(_QUAZIP_LIB_NAME "quazip5.lib")
#    else()
#        set(_QUAZIP_LIB_NAME "libquazip5.a")
#    endif()

#    if(quazip_INCLUDE_DIRS AND NOT TARGET quazip_static)
#        add_library(quazip_static UNKNOWN IMPORTED)
#        set_target_properties(quazip_static PROPERTIES
#            INTERFACE_INCLUDE_DIRECTORIES "${quazip_INCLUDE_DIRS}"
#            IMPORTED_LOCATION "${QUAZIP_INSTALL_LOCATION}/lib/${_QUAZIP_LIB_NAME}"
#            )
#    endif()
endif()


