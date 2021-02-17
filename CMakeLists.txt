CMAKE_MINIMUM_REQUIRED (VERSION 2.6)
PROJECT (modibcounters C)

# Append our module directory to CMake
LIST(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

#
# For finding packages:
#
INCLUDE(CheckIncludeFiles)
INCLUDE(FindPackageHandleStandardArgs)

IF (NOT GANGLIA_ROOT_DIR)
    SET (GANGLIA_ROOT_DIR "/usr/local" CACHE PATH "Directory in which Ganglia was installed.")
ENDIF (NOT GANGLIA_ROOT_DIR)
IF (NOT GANGLIA_BUILD_ROOT_DIR)
    SET (GANGLIA_BUILD_ROOT_DIR "/usr/src/ganglia" CACHE PATH "Directory in which Ganglia was built.")
ENDIF (NOT GANGLIA_BUILD_ROOT_DIR)

#
# Need access to Apache Portable Runtime:
#
FIND_PACKAGE(APR REQUIRED)
IF (NOT APR_FOUND)
    MESSAGE(FATAL_ERROR "APR library could not be found")
ENDIF ()

#
# Need access to Confuse configuration file library:
#
FIND_PACKAGE(libconfuse REQUIRED)
IF (NOT LIBCONFUSE_FOUND)
    MESSAGE(FATAL_ERROR "Confuse library could not be found")
ENDIF ()

#
# Find Ganglia and its metrics library:
#
FIND_PACKAGE(Ganglia REQUIRED)
IF (NOT GANGLIA_FOUND)
    MESSAGE(FATAL_ERROR "Ganglia install could not be found")
ENDIF ()
IF (NOT GANGLIAMETRIC_FOUND)
    MESSAGE(FATAL_ERROR "Ganglia metric build infrastructure could not be found")
ENDIF ()
FIND_PATH(GANGLIA_MODULES_DIR
    NAMES
        modsys.so
    HINTS
        ${GANGLIA_ROOT_DIR}/lib/ganglia
        ${GANGLIA_ROOT_DIR}/lib64/ganglia
    DOC "Directory containing Ganglia metric modules."
    REQUIRED
)

#
# The module is a shared-library target:
#
ADD_LIBRARY(modibcounters SHARED ibcounters_module.c)
SET_TARGET_PROPERTIES(modibcounters PROPERTIES PREFIX "")
TARGET_COMPILE_OPTIONS(modibcounters PUBLIC ${APR_DEFINITIONS})
TARGET_INCLUDE_DIRECTORIES(modibcounters PUBLIC ${APR_INCLUDE_DIRS} ${LIBCONFUSE_INCLUDE_DIRS} ${GANGLIA_INCLUDE_DIRS} ${GANGLIAMETRIC_INCLUDE_DIRS})
TARGET_LINK_LIBRARIES(modibcounters ${APR_LIBRARY} ${LIBCONFUSE_LIBRARY} ${GANGLIAMETRIC_LIBRARIES})
INSTALL (TARGETS modibcounters DESTINATION ${GANGLIA_MODULES_DIR})