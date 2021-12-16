option(BUILD_breakpad "Build breakpad as part of build process" ON)

set(BREAKPAD_INSTALL_LOCATION "${MODULES_INSTALL_DIR}/breakpad")

if(BUILD_EXTERNAL AND BUILD_breakpad)
    ExternalProject_Add(breakpad
        PREFIX breakpad
        GIT_REPOSITORY https://chromium.googlesource.com/breakpad/breakpad.git
        INSTALL_DIR "${BREAKPAD_INSTALL_LOCATION}"
        CONFIGURE_COMMAND <SOURCE_DIR>/configure --prefix=<INSTALL_DIR>
        BUILD_COMMAND make
        INSTALL_COMMAND make install
        )

    ExternalProject_Add_Step(breakpad clone-repo
        COMMAND git clone https://chromium.googlesource.com/linux-syscall-support <SOURCE_DIR>/src/third_party/lss || true
        COMMENT "Clone missing repo"
        DEPENDEES download
        DEPENDERS configure)

else()
    set(Breakpad_INCLUDE_DIRS "${BREAKPAD_INSTALL_LOCATION}/include" "${BREAKPAD_INSTALL_LOCATION}/include/breakpad")
    set(Breakpad_LIBRARY "${BREAKPAD_INSTALL_LOCATION}/lib")

    find_program(BREAKPAD_DUMP_SYMS_EXE NAMES dump_syms PATHS "${BREAKPAD_INSTALL_LOCATION}/bin")

    if(Breakpad_INCLUDE_DIRS AND NOT TARGET Breakpad::Breakpad)
        add_library(Breakpad::Breakpad UNKNOWN IMPORTED)
        if(WIN32)
            set_target_properties(Breakpad::Breakpad PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${Breakpad_INCLUDE_DIRS}"
                IMPORTED_LOCATION "${Breakpad_LIBRARY}/exception_handler.lib"
                INTERFACE_LINK_LIBRARIES "${Breakpad_LIBRARY}/crash_generation_client.lib;${Breakpad_LIBRARY}/common_windows_lib.lib"
                )
        else()
            if(APPLE)
                set_target_properties(Breakpad::Breakpad PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${Breakpad_INCLUDE_DIRS}"
                    IMPORTED_LOCATION "${Breakpad_LIBRARY}/libbreakpad.a"
                    )
            else()
                set_target_properties(Breakpad::Breakpad PROPERTIES
                    INTERFACE_INCLUDE_DIRECTORIES "${Breakpad_INCLUDE_DIRS}"
                    IMPORTED_LOCATION "${Breakpad_LIBRARY}/libbreakpad.a"
                    INTERFACE_LINK_LIBRARIES "${Breakpad_LIBRARY}/libbreakpad_client.a"
                    )
            endif()
        endif()
    endif()
endif()
