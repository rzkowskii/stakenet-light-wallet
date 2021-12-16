cmake_minimum_required(VERSION 3.15.6)
project(Stakenet VERSION 0.4.4.0 LANGUAGES CXX)

# Only do these if this is the main project, and not if it is included through add_subdirectory
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)

    # Optionally set things like CMAKE_CXX_STANDARD, CMAKE_POSITION_INDEPENDENT_CODE here
    
    # Let's ensure -std=c++xx instead of -std=g++xx
    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_STANDARD 17)

    # Let's nicely support folders in IDE's
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)

    if(WIN32)
        add_definitions(-DWIN32 -D_UNICODE -DUNICODE -D_WIN32_WINNT=0x0601 -DNOMINMAX)
        set(CMAKE_COMPILE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/pdb CACHE PATH "Compile output pdb dir" FORCE)
        set(CMAKE_PDB_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/pdb CACHE PATH "Linking output pdb dir" FORCE)
    endif()
    if(APPLE)
        add_definitions(-DMAC_OSX)
    endif()

    set(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/package)
    set(CMAKE_INSTALL_ALWAYS TRUE)

    # Testing only available if this is the main app
    # Note this needs to be done in the main CMakeLists
    # since it calls enable_testing, which must be in the
    # main CMakeLists.
    #include(CTest)

    # Docs only available if this is the main app
    find_package(Doxygen)
    if(Doxygen_FOUND)
        #add_subdirectory(docs)
    else()
        message(STATUS "Doxygen not found, not building docs")
    endif()
endif()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/../cmake")

include(FetchContent)
include(deps)

find_package(Qt5 5.15 COMPONENTS
    Core
    Network
    Concurrent
    Test
    Quick
    WebSockets REQUIRED)

# Instruct CMake to run moc automatically when needed.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(selectdeployqt)
include(json-rpc)

find_package(Qt5 5.15 COMPONENTS WebEngine Svg Qml)

## use, i.e. don't skip the full RPATH for the build tree
set(CMAKE_SKIP_BUILD_RPATH  FALSE)

## when building, don't use the install RPATH already
## (but later on when installing)
# set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)

#set(CMAKE_INSTALL_RPATH "${QT_LIBRARY_DIR}")

## add the automatically determined parts of the RPATH
## which point to directories outside the build tree to the install RPATH
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

FetchContent_Declare(
    qtpromise
    GIT_REPOSITORY https://github.com/simonbrunel/qtpromise.git
    GIT_TAG 7ee51de98702804a0230c47e4f786778aace5c90
    )
FetchContent_MakeAvailable(qtpromise)

FetchContent_Declare(
    cpp-httplib
    GIT_REPOSITORY https://github.com/yhirose/cpp-httplib.git
    GIT_TAG ca5a50d2c94a0d4315d96b830f16cfb9110993c9
    )
FetchContent_MakeAvailable(cpp-httplib)

# As moc files are generated in the binary dir, tell CMake
# to always look for includes there:
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# libraries
add_subdirectory(walletcore)
add_subdirectory(common)
add_subdirectory(core)
add_subdirectory(networking)
add_subdirectory(qgrpc)
add_subdirectory(lndtools)
add_subdirectory(lightningswaps)
add_subdirectory(crashreporting)
add_subdirectory(ethcore)
add_subdirectory(tradingbot)

# executables
add_subdirectory(lightningswapsdaemon)
add_subdirectory(lightningswapsclient)
add_subdirectory(lightwalletserviceclient)

if(Qt5WebEngine_FOUND)
    add_subdirectory(app)
endif()

option(ENABLE_TESTS "Build the tests" OFF)
if(ENABLE_TESTS)
    enable_testing()

    FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG        release-1.10.0
        )
    FetchContent_MakeAvailable(googletest)

    enable_testing()
    add_subdirectory(core_tests)
endif()
