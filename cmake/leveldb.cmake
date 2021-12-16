
#set(LEVELDB_INSTALL_LOCATION "${MODULES_INSTALL_DIR}/leveldb")
#set(leveldb_DIR "${LEVELDB_INSTALL_LOCATION}/lib/cmake/leveldb")
#set(GTEST_ROOT "${LEVELDB_INSTALL_LOCATION}/lib/cmake/GTest")
#option(BUILD_leveldb "Build leveldb as part of build process" ON)

if(BUILD_EXTERNAL AND BUILD_leveldb)
    ExternalProject_Add(leveldb
      PREFIX leveldb
      GIT_REPOSITORY https://github.com/google/leveldb.git
      CMAKE_CACHE_ARGS
            -DCMAKE_BUILD_TYPE:STRING=Release
            -DCMAKE_INSTALL_PREFIX:PATH=${LEVELDB_INSTALL_LOCATION}
    )

    ExternalProject_Get_property(leveldb SOURCE_DIR)

    ExternalProject_Add_Step(leveldb helpers
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${SOURCE_DIR}/helpers include/helpers
        COMMENT "Copy sources"
        WORKING_DIRECTORY ${LEVELDB_INSTALL_LOCATION}
        ALWAYS            TRUE
        DEPENDEES install)
else()
    find_package(leveldb REQUIRED)
endif()
