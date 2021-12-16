
set(BOOST_INSTALL_LOCATION "${MODULES_INSTALL_DIR}/boost")
option(BUILD_boost "Build boost as part of build process" ON)

if(BUILD_EXTERNAL AND BUILD_boost)
    
    if(APPLE)
        set(BOOST_TOOLSET "clang")
    else()
        set(BOOST_TOOLSET "gcc")
    endif()

    list(APPEND BOOST_BUILD_LIBS
        filesystem
        system
        chrono
        thread
        program_options
        regex
        date_time
        atomic)


    list(TRANSFORM BOOST_BUILD_LIBS PREPEND "--with-libraries=")

    ExternalProject_Add(boost
        PREFIX boost
        URL https://dl.bintray.com/boostorg/release/1.71.0/source/boost_1_71_0.tar.gz
        DOWNLOAD_NAME boost_1_71_0.tar.gz
        INSTALL_DIR ${BOOST_INSTALL_LOCATION}
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND ./bootstrap.sh ${BOOST_BUILD_LIBS}
        BUILD_COMMAND ./b2 -d+2 -j4 --toolset=${BOOST_TOOLSET} --prefix=<INSTALL_DIR> --reconfigure address-model=64 link=static variant=release threading=multi install
        INSTALL_COMMAND ""
      )

else()
    # boost, probably will need to be changed in future
    set(Boost_USE_STATIC_LIBS        ON)  # only find static libs
    set(Boost_USE_DEBUG_LIBS         OFF) # ignore debug libs and
    set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
    set(Boost_USE_MULTITHREADED      ON)
    set(Boost_USE_STATIC_RUNTIME     OFF)
    set(Boost_NO_SYSTEM_PATHS        ON)
    set(Boost_NO_BOOST_CMAKE         ON)

#    set(BOOST_INCLUDEDIR ${BOOST_INSTALL_LOCATION}/include)
#    set(BOOST_LIBRARYDIR ${BOOST_INSTALL_LOCATION}/lib)
#    set(BOOST_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../depends/boost/boost_1_70_0" CACHE PATH "Boost library path")
#    set(Boost_NO_SYSTEM_PATHS on CACHE BOOL "Do not search system for Boost")

    find_package(Boost 1.70.0 REQUIRED COMPONENTS chrono filesystem system thread date_time atomic regex)

endif()
