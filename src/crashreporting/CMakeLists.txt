# get revision

file(GLOB_RECURSE HEADERS *.hpp)

if(WIN32)
    list(APPEND SOURCES CrashReporting/CrashReportingWin32.cpp)
else()
    if(APPLE)
        list(APPEND SOURCES CrashReporting/CrashReportingOSX.cpp)
    else()
        list(APPEND SOURCES CrashReporting/CrashReportingLinux.cpp)
    endif()
endif()

add_library(crashreporting ${GUI_TYPE} ${SOURCES} ${HEADERS} ${RESOURCES})

target_include_directories(crashreporting PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>)
target_link_libraries(crashreporting
    sentry::sentry
    Qt5::Core)
