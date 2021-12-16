file(GLOB_RECURSE SOURCES *.cpp *.c)
file(GLOB_RECURSE HEADERS *.h*)

set_source_files_properties(${SOURCES} PROPERTIES LANGUAGE CXX)

add_library(walletcore STATIC ${SOURCES} ${HEADERS})

target_include_directories(walletcore
    PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(walletcore
    leveldb::leveldb
    CONAN_PKG::berkleydb
    Boost::boost
    Boost::thread
    Boost::filesystem
    Boost::regex
    Boost::disable_autolinking
    Qt5::Core
    CONAN_PKG::secp256k1
    OpenSSL::SSL
    OpenSSL::Crypto)

target_compile_options(walletcore PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4>)
