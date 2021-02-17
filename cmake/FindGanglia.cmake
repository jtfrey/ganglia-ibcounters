# FindGanglia
# -------
#
# Find the Ganglia monitoring installation.
#
# This will define the following variables::
#
#   GANGLIA_FOUND               - True if the system has the libraries
#   GANGLIA_INCLUDE_DIRS        - where to find the headers
#   GANGALI_LIBRARIES           - where to find the libraries
#   GANGLIAMETRIC_FOUND         - True if the system has metrics build infrastructure
#   GANGLIAMETRIC_INCLUDE_DIRS  - where to find the metric library
#   GANGLIAMETRIC_LIBRARY       - where to find the metric library
#
# Hints:
# Set ``GANGLIA_ROOT_DIR`` to the root directory of an installation.
# Set ``GANGLIA_BUILD_ROOT_DIR`` to the root directory in which Ganglia was built.
#
include(FindPackageHandleStandardArgs)

find_path(GANGLIA_INCLUDE_DIR ganglia.h
	HINTS
		${GANGLIA_ROOT_DIR}/include
		${GANGLIA_ROOT_INCLUDE_DIRS}
	PATHS
		/usr/local/include
		/usr/include
)

find_library(GANGLIA_LIBRARY
  NAMES ganglia ${GANGLIA_NAMES}
  HINTS
	${GANGLIA_ROOT_DIR}/lib
	${GANGLIA_ROOT_LIBRARY_DIRS}
  PATHS
	/usr/lib
	/usr/local/lib
  )

find_package_handle_standard_args(Ganglia
  FOUND_VAR GANGLIA_FOUND
  REQUIRED_VARS
	GANGLIA_INCLUDE_DIR
	GANGLIA_LIBRARY
)

if(GANGLIA_FOUND)
  set(GANGLIA_LIBRARIES ${GANGLIA_LIBRARY})
  set(GANGLIA_INCLUDE_DIRS ${GANGLIA_INCLUDE_DIR})
endif()

#
# Metrics build infrastructure:
#

find_path(GANGLIAMETRIC_INCLUDE_DIR libmetrics.h
	HINTS
		${GANGLIA_BUILD_ROOT_DIR}/libmetrics
		${GANGLIA_ROOT_DIR}/src/libmetrics
		${GANGLIA_BUILD_ROOT_INCLUDE_DIRS}
	PATHS
		/usr/local/include
		/usr/include
)

find_library(GANGLIAMETRIC_LIBRARY
  NAMES metrics ${GANGLIAMETRIC_NAMES}
  HINTS
    ${GANGLIA_BUILD_ROOT_DIR}/libmetrics/.libs
    ${GANGLIA_ROOT_DIR}/src/libmetrics/.libs
	${GANGLIA_ROOT_LIBRARY_DIRS}
  PATHS
	/usr/lib
	/usr/local/lib
  )

find_package_handle_standard_args(GangliaMetric
  FOUND_VAR GANGLIAMETRIC_FOUND
  REQUIRED_VARS
	GANGLIAMETRIC_INCLUDE_DIR
	GANGLIAMETRIC_LIBRARY
)

if(GANGLIAMETRIC_FOUND)
  set(GANGLIAMETRIC_LIBRARIES ${GANGLIAMETRIC_LIBRARY})
  set(GANGLIAMETRIC_INCLUDE_DIRS ${GANGLIAMETRIC_INCLUDE_DIR})
endif()

#

mark_as_advanced(
  GANGLIA_LIBRARY
  GANGLIA_INCLUDE_DIR
  GANGLIAMETRIC_LIBRARY
  GANGLIAMETRIC_INCLUDE_DIR
)