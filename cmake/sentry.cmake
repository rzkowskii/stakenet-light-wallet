
# set(sentry_INSTALL_LOCATION "${MODULES_INSTALL_DIR}/sentry")
# set(sentry_DIR "${sentry_INSTALL_LOCATION}/lib/cmake/sentry")
# option(BUILD_sentry "Build sentry as part of build process" ON)

if(BUILD_EXTERNAL AND BUILD_sentry)
    ExternalProject_Add(sentry
      PREFIX sentry
      GIT_REPOSITORY https://github.com/getsentry/sentry-native.git
      GIT_TAG 0.2.2
      CMAKE_CACHE_ARGS
            -DCMAKE_BUILD_TYPE:STRING=Release
            -DCMAKE_INSTALL_PREFIX:PATH=${sentry_INSTALL_LOCATION}
    )
else()
    find_package(sentry REQUIRED)
endif()
