
set(BDB_INSTALL_LOCATION "${MODULES_INSTALL_DIR}/libbdb")
option(BUILD_bdb "Build BerkleyDB as part of build process" ON)

if(BUILD_EXTERNAL AND BUILD_bdb)
    ExternalProject_Add(bdb
        PREFIX bdb
        DOWNLOAD_COMMAND ""
        INSTALL_DIR "${BDB_INSTALL_LOCATION}"
        CONFIGURE_COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/bdb/install_db4.sh <INSTALL_DIR>
        BUILD_COMMAND ""
        INSTALL_COMMAND ""
        )
else()
    set(BerkeleyDB_INCLUDE_DIRS "${BDB_INSTALL_LOCATION}/include")
    set(BerkeleyDB_LIBRARY "${BDB_INSTALL_LOCATION}/lib")
    list(APPEND BerkeleyDB_LIBRARIES "libdb-4.8.a" "libdb_cxx-4.8.a" "libdb_cxx.a")

    list(TRANSFORM BerkeleyDB_LIBRARIES PREPEND ${BerkeleyDB_LIBRARY}/)

    set(BerkleyDB_IMPORTED_LIBRARY "libdb.a")

    if(WIN32)
        set(BerkleyDB_IMPORTED_LIBRARY "libdb48s.lib")
    endif()

    if(BerkeleyDB_INCLUDE_DIRS AND BerkeleyDB_LIBRARIES AND NOT TARGET Oracle::BerkeleyDB)
        add_library(Oracle::BerkeleyDB UNKNOWN IMPORTED)
        if(WIN32)
            set_target_properties(Oracle::BerkeleyDB PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${BerkeleyDB_INCLUDE_DIRS}"
                IMPORTED_LOCATION "${BerkeleyDB_LIBRARY}/${BerkleyDB_IMPORTED_LIBRARY}"
            )
        else()
            set_target_properties(Oracle::BerkeleyDB PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${BerkeleyDB_INCLUDE_DIRS}"
                IMPORTED_LOCATION "${BerkeleyDB_LIBRARY}/${BerkleyDB_IMPORTED_LIBRARY}"
                INTERFACE_LINK_LIBRARIES "${BerkeleyDB_LIBRARIES}"
            )
        endif()

    endif()
endif()
