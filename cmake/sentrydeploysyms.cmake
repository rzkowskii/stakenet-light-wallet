if(NOT DEFINED SYMBOLS_DIR)
    message(FATAL_ERROR "No deployenv specified, use -DSYMBOLS_DIR")
endif()

set(ENV{SENTRY_AUTH_TOKEN} "ccb1c5360696475480cc1b028f52be8d9f8da2fae5a741b28e2c867bfe6bd6e5")

find_program(SENTRY_CLI NAMES sentry-cli PATHS "${DEPLOY_DIR}/tools/bin" ${SENTRY_CLI_HINT_PATH})

if(NOT SENTRY_CLI)
    message(FATAL_ERROR "Did not find sentry-cli")
endif()

SET(PROJECT_NAME "stakenet-wallet-native-cc" CACHE STRING "Project name" FORCE)

message(STATUS "Uploading symbols for: ${PROJECT_NAME}")

execute_process(COMMAND ${SENTRY_CLI} upload-dif -o alexis-hernandez -p ${PROJECT_NAME} ${SYMBOLS_DIR}
    RESULT_VARIABLE UPLOADED)

if(NOT (UPLOADED EQUAL 0))
    message(FATAL_ERROR "Failed to upload from ${SYMBOLS_DIR}")
endif()
