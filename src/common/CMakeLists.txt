#file(GLOB HEADER_LIST CONFIGURE_DEPENDS "${ModernCMakeExample_SOURCE_DIR}/include/modern/*.hpp")

#find_package(Qt5 REQUIRED Core)

file(GLOB SOURCES Utils/*.cpp)
file(GLOB HEADERS Utils/*.hpp)

add_library(common STATIC ${SOURCES} ${HEADERS})

# We need this directory, and users of our library will need it too
target_include_directories(common PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(common
    Boost::boost
    qtpromise
    Qt5::Core)

target_compile_options(common PRIVATE
     $<$<OR:$<CXX_COMPILER_ID:Clang>,$<CXX_COMPILER_ID:AppleClang>,$<CXX_COMPILER_ID:GNU>>:
          -Wall>
     $<$<CXX_COMPILER_ID:MSVC>:
          /W4>)
