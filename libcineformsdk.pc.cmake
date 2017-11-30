prefix=${CMAKE_INSTALL_PREFIX}
exec_prefix=${EXEC_INSTALL_PREFIX}
libdir=${LIB_INSTALL_DIR}
includedir=${INCLUDE_INSTALL_DIR}

Name: ${PROJECT_NAME}
Description: CineForm SDK libraries
URL: https://github.com/gopro/cineform-sdk
Version: ${PROJECT_VERSION}
Libs: -L${LIB_INSTALL_DIR} -lCFHDCodec ${ADDITIONAL_LIBS}
Cflags: -I${INCLUDE_INSTALL_DIR}