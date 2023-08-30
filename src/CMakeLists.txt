CMAKE_MINIMUM_REQUIRED (VERSION 3.8)

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/src/lib")

IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	ADD_LIBRARY(evp STATIC 
		"lib/md5/md5.cpp"
		"libevp.cpp"
	)
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	ADD_LIBRARY(libevp STATIC
		"lib/md5/md5.cpp"
		"libevp.cpp"
	)

	SET_TARGET_PROPERTIES(libevp PROPERTIES LINK_FLAGS "/LTCG-")

	IF (CMAKE_BUILD_TYPE STREQUAL "Debug")
		TARGET_LINK_LIBRARIES(libevp ${CMAKE_SOURCE_DIR}/src/lib/libdvsku_crypt/libdvsku_crypt_debug.lib)
	ELSE()
		TARGET_LINK_LIBRARIES(libevp ${CMAKE_SOURCE_DIR}/src/lib/libdvsku_crypt/libdvsku_crypt_release.lib)
	ENDIF()
ENDIF()

