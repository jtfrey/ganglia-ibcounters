# Findlibconfuse
# -------
#
# Find the Confuse configuration file library.
#
# This will define the following variables::
#
#   LIBCONFUSE_FOUND           - True if the system has the libraries
#   LIBCONFUSE_INCLUDE_DIRS    - where to find the headers
#   LIBCONFUSE_LIBRARIES       - where to find the libraries
#
# Hints:
# Set ``LIBCONFUSE_ROOT_DIR`` to the root directory of an installation.
#
include(FindPackageHandleStandardArgs)

find_path(LIBCONFUSE_INCLUDE_DIR confuse.h
	HINTS
		${LIBCONFUSE_ROOT_DIR}/include
		${LIBCONFUSE_ROOT_INCLUDE_DIRS}
	PATHS
		/usr/local/include
		/usr/include
)

find_library(LIBCONFUSE_LIBRARY
  NAMES confuse ${LIBCONFUSE_NAMES}
  HINTS
	${LIBCONFUSE_ROOT_DIR}/lib
	${LIBCONFUSE_ROOT_LIBRARY_DIRS}
  PATHS
	/usr/lib
	/usr/local/lib
  )

find_package_handle_standard_args(libconfuse
  FOUND_VAR LIBCONFUSE_FOUND
  REQUIRED_VARS
	LIBCONFUSE_INCLUDE_DIR
	LIBCONFUSE_LIBRARY
)

if(LIBCONFUSE_FOUND)
  set(LIBCONFUSE_LIBRARIES ${LIBCONFUSE_LIBRARY})
  set(LIBCONFUSE_INCLUDE_DIRS ${LIBCONFUSE_INCLUDE_DIR})
endif()

mark_as_advanced(
  LIBCONFUSE_LIBRARY
  LIBCONFUSE_INCLUDE_DIR
)