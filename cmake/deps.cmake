set(SYSTEM_SHORT_NAME "linux")

if(WIN32)
    set(SYSTEM_SHORT_NAME "win")
endif()
if(APPLE)
    set(SYSTEM_SHORT_NAME "osx")
endif()

set(MODULES_INSTALL_DIR "${CMAKE_CURRENT_LIST_DIR}/../modules/${SYSTEM_SHORT_NAME}" CACHE PATH "Path where modules will be installed")
message(STATUS "Modules install dir: ${MODULES_INSTALL_DIR}")


include(${CMAKE_BINARY_DIR}/conanbuildinfo.cmake)
conan_basic_setup(TARGETS NO_OUTPUT_DIRS KEEP_RPATHS)

if (TARGET CONAN_PKG::zeromq)
    add_library(CONAN_PKG::ZeroMQ ALIAS CONAN_PKG::zeromq)
endif()

find_program(BREAKPAD_DUMP_SYMS_EXE NAMES dump_syms PATHS "${CMAKE_BINARY_DIR}/bin" NO_DEFAULT_PATH)
find_package(OpenSSL REQUIRED)

include(sentry)
include(grpc)
include(leveldb)
#include(berkleydb)
#include(zmq)
#include(quazip)
#include(breakpad)
#include(secp256k1)
include(boost)

