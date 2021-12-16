set(ZMQ_INSTALL_LOCATION "${MODULES_INSTALL_DIR}/libzmq")
option(BUILD_zmq "Build zmq as part of build process" ON)

if(WIN32)
    set(ZeroMQ_DIR "${ZMQ_INSTALL_LOCATION}/CMake")
else()
    set(ZeroMQ_DIR "${ZMQ_INSTALL_LOCATION}/share/cmake/ZeroMQ")
endif()


if(BUILD_EXTERNAL AND BUILD_zmq)
    ExternalProject_Add(zmq
      PREFIX zmq
      GIT_REPOSITORY https://github.com/zeromq/libzmq.git
      GIT_TAG v4.3.2
      CMAKE_CACHE_ARGS
            -DCMAKE_BUILD_TYPE:STRING=Release
            -DCMAKE_INSTALL_PREFIX:PATH=${ZMQ_INSTALL_LOCATION}
    )
else()
    find_package(ZeroMQ REQUIRED)
endif()
