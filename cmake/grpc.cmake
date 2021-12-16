cmake_minimum_required(VERSION 3.5.1)

#if(WIN32)
#    set(Protobuf_DIR "${MODULES_INSTALL_DIR}/protobuf/cmake" CACHE PATH "Location of protobuf" FORCE)
#else()
#    set(Protobuf_DIR "${MODULES_INSTALL_DIR}/protobuf/lib/cmake/protobuf" CACHE PATH "Location of protobuf" FORCE)
#endif()

#set(GRPC_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../depends/grpc/grpc")
#set(gRPC_INSTALL_LOCATION "${MODULES_INSTALL_DIR}/grpc")

#set(c-ares_DIR "${MODULES_INSTALL_DIR}/c-ares/lib/cmake/c-ares")
##set(absl_DIR "${MODULES_INSTALL_DIR}/absl/lib/cmake/absl")
#set(ZLIB_ROOT "${MODULES_INSTALL_DIR}/zlib" CACHE PATH "ZLIB root")


#set(gRPC_DIR "${gRPC_INSTALL_LOCATION}/lib/cmake/grpc")
#option(BUILD_gRPC "Build gRPC as part of build process" ON)

if(BUILD_EXTERNAL AND BUILD_gRPC)
    # Note: For all external projects, instead of using checked-out code, one could
    # specify GIT_REPOSITORY and GIT_TAG to have cmake download the dependency directly,
    # without needing to add a submodule to your project.

    # Builds absl project from the git submodule.
    ExternalProject_Add(absl
      PREFIX absl
      SOURCE_DIR "${GRPC_SOURCE_DIR}/third_party/abseil-cpp"
      CMAKE_CACHE_ARGS
            -DCMAKE_BUILD_TYPE:STRING=Release
            -DCMAKE_POSITION_INDEPENDENT_CODE:BOOL=TRUE
            -DCMAKE_INSTALL_PREFIX:PATH=${MODULES_INSTALL_DIR}/absl
    )

    # Builds c-ares project from the git submodule.
    ExternalProject_Add(c-ares
      PREFIX c-ares
      SOURCE_DIR "${GRPC_SOURCE_DIR}/third_party/cares/cares"
      CMAKE_CACHE_ARGS
            -DCMAKE_BUILD_TYPE:STRING=Release
            -DCARES_SHARED:BOOL=OFF
            -DCARES_STATIC:BOOL=ON
            -DCARES_STATIC_PIC:BOOL=ON
            -DCMAKE_INSTALL_PREFIX:PATH=${MODULES_INSTALL_DIR}/c-ares
    )

    # Builds protobuf project from the git submodule.
    ExternalProject_Add(protobuf
      PREFIX protobuf
      SOURCE_DIR "${GRPC_SOURCE_DIR}/third_party/protobuf/cmake"
      CMAKE_CACHE_ARGS
            -DCMAKE_BUILD_TYPE:STRING=Release
            -Dprotobuf_BUILD_TESTS:BOOL=OFF
            -Dprotobuf_WITH_ZLIB:BOOL=OFF
            -Dprotobuf_MSVC_STATIC_RUNTIME:BOOL=OFF
            -DCMAKE_INSTALL_PREFIX:PATH=${MODULES_INSTALL_DIR}/protobuf
    )

    # Builds zlib project from the git submodule.
    ExternalProject_Add(zlib
      PREFIX zlib
      SOURCE_DIR "${GRPC_SOURCE_DIR}/third_party/zlib"
      CMAKE_CACHE_ARGS
            -DCMAKE_BUILD_TYPE:STRING=Release
            -DCMAKE_INSTALL_PREFIX:PATH=${MODULES_INSTALL_DIR}/zlib
    )

    # the location where protobuf-config.cmake will be installed varies by platform
    if (WIN32)
      set(_FINDPACKAGE_PROTOBUF_CONFIG_DIR "${MODULES_INSTALL_DIR}/protobuf/cmake")
    else()
      set(_FINDPACKAGE_PROTOBUF_CONFIG_DIR "${MODULES_INSTALL_DIR}/protobuf/lib/cmake/protobuf")
    endif()

    # if OPENSSL_ROOT_DIR is set, propagate that hint path to the external projects with OpenSSL dependency.
    set(_CMAKE_ARGS_OPENSSL_ROOT_DIR "")
    if (OPENSSL_ROOT_DIR)
      set(_CMAKE_ARGS_OPENSSL_ROOT_DIR "-DOPENSSL_ROOT_DIR:PATH=${OPENSSL_ROOT_DIR}")
    endif()

    ## Builds gRPC based on locally checked-out sources and set arguments so that all the dependencies
    ## are correctly located.
    ExternalProject_Add(grpc
      PREFIX grpc
      SOURCE_DIR "${GRPC_SOURCE_DIR}"
      CMAKE_CACHE_ARGS
            -DCMAKE_BUILD_TYPE:STRING=Release
            -DgRPC_INSTALL:BOOL=ON
            -DgRPC_BUILD_TESTS:BOOL=OFF
            -DgRPC_PROTOBUF_PROVIDER:STRING=package
            -DgRPC_PROTOBUF_PACKAGE_TYPE:STRING=CONFIG
            -DProtobuf_DIR:PATH=${_FINDPACKAGE_PROTOBUF_CONFIG_DIR}
            -DgRPC_ZLIB_PROVIDER:STRING=package
            -DZLIB_ROOT:STRING=${ZLIB_ROOT}
            -DgRPC_ABSL_PROVIDER:STRING=package
            -Dabsl_DIR:STRING=${absl_DIR}
            -DgRPC_CARES_PROVIDER:STRING=package
            -Dc-ares_DIR:PATH=${c-ares_DIR}
            -DgRPC_SSL_PROVIDER:STRING=package
            ${_CMAKE_ARGS_OPENSSL_ROOT_DIR}
            -DCMAKE_INSTALL_PREFIX:PATH=${gRPC_INSTALL_LOCATION}
      DEPENDS c-ares protobuf zlib absl
    )
else()
    find_package(absl REQUIRED)
    find_package(c-ares REQUIRED)
    find_package(Protobuf REQUIRED CONFIG)
    find_package(ZLIB REQUIRED)
    find_package(gRPC REQUIRED)
endif()

