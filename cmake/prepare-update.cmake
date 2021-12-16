set(STAKENET_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/Stakenet")

message(STATUS "Starting preparing update")

if(WIN32)
    set(UPDATE_CONTENT_DIR ${STAKENET_INSTALL_PREFIX})
elseif(APPLE)
    set(STAKENET_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")
    # app exe is bundle on apple
    set(UPDATE_CONTENT_DIR "${APP_EXE}")
    set(APP_EXE "${APP_EXE}/Contents/MacOS/Stakenet")
else()
    set(UPDATE_CONTENT_DIR ${STAKENET_INSTALL_PREFIX}/usr)
endif()

function(create_zip output_file working_dir)
    message(STATUS "Creating zip at location: ${output_file}")
    if(APPLE)
        execute_process(COMMAND "ditto" -ck --rsrc --sequesterRsrc ./ ${output_file}
            WORKING_DIRECTORY ${working_dir}
            RESULT_VARIABLE CREATED
            OUTPUT_VARIABLE CREATED_OUTPUT_VARIABLE
            ERROR_VARIABLE CREATED_OUTPUT_VARIABLE)
    else()
        file(GLOB ZIP_FILES "${working_dir}/*")
        execute_process(COMMAND ${CMAKE_COMMAND} -E tar c ${output_file} --format=zip ${ZIP_FILES}
            WORKING_DIRECTORY ${working_dir}
            RESULT_VARIABLE CREATED
            OUTPUT_VARIABLE CREATED_OUTPUT_VARIABLE
            ERROR_VARIABLE CREATED_OUTPUT_VARIABLE)
        if(NOT (CREATED EQUAL 0))
            message(FATAL_ERROR "Failed to create ${output_file}, ${CREATED_OUTPUT_VARIABLE}")
        endif()
    endif()

endfunction()

if(NOT WIN32)
    execute_process(COMMAND ${STRIP_TOOL} ${APP_EXE})
endif()

if(APPLE)
    set(STAKENET_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}/Stakenet")
    # we need to do this in order to get proper zip
    execute_process(COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_INSTALL_PREFIX}/Stakenet.app ${STAKENET_INSTALL_PREFIX}/Stakenet.app)
endif()

create_zip(${CMAKE_INSTALL_PREFIX}/Stakenet.zip ${STAKENET_INSTALL_PREFIX})
create_zip(${CMAKE_INSTALL_PREFIX}/update/${GIT_SHA1}/${GIT_SHA1}.zip ${UPDATE_CONTENT_DIR})

if(APPLE)
    execute_process(COMMAND ${CMAKE_COMMAND} -E rm -r ${STAKENET_INSTALL_PREFIX})
endif()

execute_process(COMMAND ${APP_EXE} --version OUTPUT_VARIABLE APP_VERSION)
execute_process(COMMAND ${CHECKSUM_TOOL} ${UPDATE_CONTENT_DIR} OUTPUT_VARIABLE APP_CHECKSUM)
file(WRITE update.json "{\"version\":${APP_VERSION}, \"remove\":[], \"name\":\"${GIT_SHA1}.zip\", \"checksum\": \"${APP_CHECKSUM}\"}" )

message(STATUS "Finished preparing update")
