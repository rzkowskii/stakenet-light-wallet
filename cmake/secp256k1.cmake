
set(SECP256K1_INSTALL_LOCATION "${MODULES_INSTALL_DIR}/libsecp256k1")
option(BUILD_secp256k1 "Build secp256k1 as part of build process" ON)

if(BUILD_EXTERNAL AND BUILD_secp256k1)
    ExternalProject_Add(secp256k1
        PREFIX secp256k1
        GIT_REPOSITORY https://github.com/bitcoin-core/secp256k1.git
        GIT_TAG 5df77a0eda6e902a1aa9c6249cdeaec197b1e0cd
        INSTALL_DIR "${SECP256K1_INSTALL_LOCATION}"
        BUILD_IN_SOURCE TRUE
        CONFIGURE_COMMAND ./autogen.sh && ./configure --prefix=<INSTALL_DIR> --disable-tests --disable-shared --enable-static --enable-module-recovery
        BUILD_COMMAND make
        INSTALL_COMMAND make install
    )
else()
    set(secp256k1_INCLUDE_DIRS "${SECP256K1_INSTALL_LOCATION}/include")
    set(secp256k1_LIBRARY "${SECP256K1_INSTALL_LOCATION}/lib")

    if(secp256k1_INCLUDE_DIRS AND NOT TARGET secp256k1)
       add_library(secp256k1 UNKNOWN IMPORTED)
       if(WIN32)
           set_target_properties(secp256k1 PROPERTIES
              INTERFACE_INCLUDE_DIRECTORIES "${secp256k1_INCLUDE_DIRS}"
              IMPORTED_LOCATION "${secp256k1_LIBRARY}/secp256k1-x64-v141-mt-s-0_1_0_15.lib"
           )
       else()
           if(APPLE)
               set_target_properties(secp256k1 PROPERTIES
                  INTERFACE_INCLUDE_DIRECTORIES "${secp256k1_INCLUDE_DIRS}"
                  IMPORTED_LOCATION "${secp256k1_LIBRARY}/libsecp256k1.a"
               )
           else()
               set_target_properties(secp256k1 PROPERTIES
                  INTERFACE_INCLUDE_DIRECTORIES "${secp256k1_INCLUDE_DIRS}"
                  IMPORTED_LOCATION "${secp256k1_LIBRARY}/libsecp256k1.a"
                  INTERFACE_LINK_LIBRARIES "gmp"
               )
           endif()
       endif()
    endif()
endif()
