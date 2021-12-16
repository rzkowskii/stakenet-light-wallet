function(generate_symbols working_dir dump_syms app_exe)

    #    function(create_zip output_file working_dir)
    #        file(GLOB ZIP_FILES "${working_dir}/*")
    #        execute_process(COMMAND ${CMAKE_COMMAND} -E tar c ${output_file} --format=zip ${ZIP_FILES}
    #            WORKING_DIRECTORY ${working_dir})
    #    endfunction()

    #    get_filename_component(app_exe_name ${app_exe} NAME_WE)

    #    set(BREAKPAD_SYMBOLS "${app_exe_name}.sym")
    #    execute_process(COMMAND ${dump_syms} ${app_exe} OUTPUT_FILE ${working_dir}/${BREAKPAD_SYMBOLS})
    #    file(STRINGS ${working_dir}/${BREAKPAD_SYMBOLS} BREAKPAD_INFO LIMIT_COUNT 1)
    #    separate_arguments(BREAKPAD_INFO)
    #    list(GET BREAKPAD_INFO 3 BREAKPAD_UID)
    #    list(GET BREAKPAD_INFO 4 BREAKPAD_TARGET)
    #    set(BREAKPAD_STRUCTURE ${working_dir}/symbols/${BREAKPAD_TARGET}/${BREAKPAD_UID})
    #    file(MAKE_DIRECTORY ${BREAKPAD_STRUCTURE})
    #    file(RENAME ${working_dir}/${BREAKPAD_SYMBOLS} ${BREAKPAD_STRUCTURE}/${BREAKPAD_SYMBOLS})

    #    create_zip(${working_dir}/symbols.zip ${UPDATE_DIR}/symbols)
    #    file(REMOVE_RECURSE ${working_dir}/symbols)

    get_filename_component(app_exe_name ${app_exe} NAME_WE)
    set(BREAKPAD_SYMBOLS "${app_exe_name}.sym")
    execute_process(COMMAND ${dump_syms} ${app_exe} OUTPUT_FILE ${working_dir}/${BREAKPAD_SYMBOLS})

endfunction()
