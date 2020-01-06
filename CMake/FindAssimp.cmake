# A stripped version of FindAssimp.camke which doesn't define the location of
# the include directories. Instead returns the following:
# ASSIMP_FOUND - system has Assimp
# ASSIMP_LIBRARIES - link these to use Assimp

FIND_LIBRARY(ASSIMP_LIBRARY assimp
	/usr/lib64
	/usr/lib
	/usr/local/lib
	/opt/local/lib
	${CMAKE_SOURCE_DIR}/lib
)

IF(ASSIMP_LIBRARY)
	SET(ASSIMP_FOUND TRUE)
ENDIF()

IF(ASSIMP_FOUND)
	IF(NOT ASSIMP_FIND_QUIETLY)
	MESSAGE(STATUS "Found ASSIMP: ${ASSIMP_LIBRARY}")
	ENDIF(NOT ASSIMP_FIND_QUIETLY)
ELSE(ASSIMP_FOUND)
	IF(ASSIMP_FIND_REQUIRED)
	MESSAGE(FATAL_ERROR "Could not find libASSIMP")
	ENDIF(ASSIMP_FIND_REQUIRED)
ENDIF(ASSIMP_FOUND)
