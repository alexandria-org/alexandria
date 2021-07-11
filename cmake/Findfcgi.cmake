# CMake module to search for FastCGI headers
#
# If it's found it sets FCGI_FOUND to TRUE
# and following variables are set:
#    FCGI_INCLUDE_DIR
#    FCGI_LIBRARY
FIND_PATH(FCGI_INCLUDE_DIR
  fcgio.h
  PATHS
  /usr/include
  /usr/local/include
  /usr/include/fastcgi
  "$ENV{LIB_DIR}/include"
  $ENV{INCLUDE}
  )

FIND_LIBRARY(FCGI_LIBRARY NAMES fcgi libfcgi PATHS 
  /usr/local/lib 
  /usr/lib 
  "$ENV{LIB_DIR}/lib"
  "$ENV{LIB}"
  )
FIND_LIBRARY(FCGI_LIBRARYCPP NAMES libfcgi++.so PATHS 
  /usr/local/lib 
  /usr/lib 
  "$ENV{LIB_DIR}/lib"
  "$ENV{LIB}"
  )

IF (FCGI_INCLUDE_DIR AND FCGI_LIBRARY)
   SET(FCGI_FOUND TRUE)
ENDIF (FCGI_INCLUDE_DIR AND FCGI_LIBRARY)

IF (FCGI_FOUND)
   IF (NOT FCGI_FIND_QUIETLY)
      MESSAGE(STATUS "Found FCGI: ${FCGI_LIBRARY}")
      MESSAGE(STATUS "Found FCGI: ${FCGI_LIBRARYCPP}")
   ENDIF (NOT FCGI_FIND_QUIETLY)
ELSE (FCGI_FOUND)
   IF (FCGI_FIND_REQUIRED)
      MESSAGE(FATAL_ERROR "Could not find FCGI")
   ENDIF (FCGI_FIND_REQUIRED)
ENDIF (FCGI_FOUND)
