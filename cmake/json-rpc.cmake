FetchContent_Declare(
    jcon-cpp
    GIT_REPOSITORY https://github.com/durkmurder/jcon-cpp.git
    GIT_TAG daa945e6a3cacda37983c2e086e6300723f2a291
    )
FetchContent_GetProperties(jcon-cpp)
if(NOT jcon-cpp_POPULATED)
    FetchContent_Populate(jcon-cpp)
endif()

set(JCON_DIR "${jcon-cpp_SOURCE_DIR}/src/jcon")

file(GLOB jcon_headers ${JCON_DIR}/*.h)
file(GLOB jcon_sources ${JCON_DIR}/*.cpp)

add_library(jcon STATIC ${jcon_headers} ${jcon_sources})
set_target_properties(jcon PROPERTIES PUBLIC_HEADER "${jcon_headers}")
target_link_libraries(jcon Qt5::Core Qt5::Network Qt5::WebSockets)
target_include_directories(jcon PUBLIC $<BUILD_INTERFACE:${JCON_DIR}>)
