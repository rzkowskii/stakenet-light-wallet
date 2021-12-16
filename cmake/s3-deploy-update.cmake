if(NOT DEFINED DEPLOY_ENV)
    message(FATAL_ERROR "No deployenv specified, use -DDEPLOY_ENV")
endif()

if(NOT DEFINED PLATFORM)
    message(FATAL_ERROR "No platform specified, use -DPLATFORM")
endif()

if(NOT DEFINED UPDATE_DIR)
    message(FATAL_ERROR "No update dir specified, use -DUPDATE_DIR")
endif()

get_filename_component(BUILD_HASH "${UPDATE_DIR}" NAME)

set(_ALLOWED_DEPLOY_VALUES "staging" "production")
set(_ALLOWED_PLATFORMS "win" "linux" "osx")
list(FIND _ALLOWED_DEPLOY_VALUES ${DEPLOY_ENV}  VALID_DEPLOY)
list(FIND _ALLOWED_PLATFORMS ${PLATFORM}  VALID_PLATFORM)

if(VALID_DEPLOY EQUAL -1)
    message(FATAL_ERROR "Invalid deploy_env use one of: ${_ALLOWED_DEPLOY_VALUES}")
endif()

if(VALID_PLATFORM EQUAL -1)
    message(FATAL_ERROR "Invalid platform use one of: ${_ALLOWED_PLATFORMS}")
endif()

find_program(AWS_CLI NAMES aws PATHS ${AWS2_HINT_PATH})

if(NOT AWS_CLI)
    message(FATAL_ERROR "Did not find aws2 executable")
endif()

if(DEPLOY_ENV EQUAL "staging")
    set(IS_PRODUCTION OFF)
else()
    set(IS_PRODUCTION ON)
endif()

set(BUCKET "s3://auto-updater-wallet-test/light-wallet/${DEPLOY_ENV}/${PLATFORM}")

if(IS_PRODUCTION)
    execute_process(COMMAND ${AWS_CLI} s3 cp --acl public-read ${UPDATE_DIR}/${BUILD_HASH}.zip ${BUCKET}/builds/
        RESULT_VARIABLE UPLOADED
        OUTPUT_VARIABLE UPLOAD_OUTPUT_VARIABLE
        ERROR_VARIABLE  UPLOAD_OUTPUT_VARIABLE)

    if(NOT (UPLOADED EQUAL 0))
        message(FATAL_ERROR "Failed to upload ${BUILD_HASH}.zip, ${UPLOAD_OUTPUT_VARIABLE}")
    endif()
endif()

#execute_process(COMMAND ${AWS_CLI} s3 cp --acl public-read ${UPDATE_DIR}/${BUILD_HASH}-symbols.zip ${BUCKET}/symbols
#    RESULT_VARIABLE UPLOADED)  

#if(NOT (UPLOADED EQUAL 0))
#    message(FATAL_ERROR "Failed to upload ${BUILD_HASH}-symbols.zip")
#endif()

if(IS_PRODUCTION)
    execute_process(COMMAND ${AWS_CLI} s3 cp --acl public-read ${UPDATE_DIR}/update.json ${BUCKET}/
        RESULT_VARIABLE UPLOADED
        OUTPUT_VARIABLE UPLOAD_OUTPUT_VARIABLE
        ERROR_VARIABLE  UPLOAD_OUTPUT_VARIABLE)

    if(NOT (UPLOADED EQUAL 0))
        message(FATAL_ERROR "Failed to uploaded update.json, ${UPLOAD_OUTPUT_VARIABLE}")
    endif()
endif()

if(INSTALLER)
    execute_process(COMMAND ${AWS_CLI} s3 cp --acl public-read ${INSTALLER} ${BUCKET}/installer/
        RESULT_VARIABLE UPLOADED)
endif()
