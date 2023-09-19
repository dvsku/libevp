CMAKE_MINIMUM_REQUIRED (VERSION 3.8)

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/libevp")

IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	SET(CMAKE_CXX_STANDARD 20)
	SET(CMAKE_CXX_FLAGS			"-m32 -ldl -pthread -lstdc++fs -Wall -fpic")
	SET(CMAKE_CXX_FLAGS_DEBUG	"-O0")
	SET(CMAKE_CXX_FLAGS_RELEASE "-O2")

	LINK_LIBRARIES(libevp)
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
	SET(CMAKE_CXX_FLAGS			"/std:c++20 /DWIN32 /D_WINDOWS /W3 /EHsc /GR /GL-")
	SET(CMAKE_CXX_FLAGS_DEBUG	"/Od /Ob0 /RTC1 /Zi")
	SET(CMAKE_CXX_FLAGS_RELEASE "/O2 /Ob2 /DNDEBUG")

	LINK_LIBRARIES(libevp_md)
ENDIF()

ADD_EXECUTABLE(test_v1_packing v1/test_packing.cpp)

ADD_TEST(NAME v1_packing_single_file			COMMAND test_v1_packing 0 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_packing_single_file_encrypted	COMMAND test_v1_packing 1 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_packing_folder					COMMAND test_v1_packing 2 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_packing_folder_encrypted		COMMAND test_v1_packing 3 ${CMAKE_SOURCE_DIR})

ADD_EXECUTABLE(test_v1_unpacking v1/test_unpacking.cpp)

ADD_TEST(NAME v1_unpacking_single_file				COMMAND test_v1_unpacking 0 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_unpacking_single_file_encrypted	COMMAND test_v1_unpacking 1 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_unpacking_folder					COMMAND test_v1_unpacking 2 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_unpacking_folder_encrypted			COMMAND test_v1_unpacking 3 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_get_file_from_evp					COMMAND test_v1_unpacking 4 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_get_file_from_evp_encrypted		COMMAND test_v1_unpacking 5 ${CMAKE_SOURCE_DIR})

ADD_EXECUTABLE(test_misc v1/test_misc.cpp)

ADD_TEST(NAME v1_get_evp_file_list				COMMAND test_misc 0 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_get_evp_file_list_encrypted	COMMAND test_misc 1 ${CMAKE_SOURCE_DIR})