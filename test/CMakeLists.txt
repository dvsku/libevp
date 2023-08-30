CMAKE_MINIMUM_REQUIRED (VERSION 3.8)

INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}")
INCLUDE_DIRECTORIES("${CMAKE_SOURCE_DIR}/libevp")
LINK_DIRECTORIES("${CMAKE_SOURCE_DIR}/libevp")

ADD_EXECUTABLE(test_v1_packing v1/test_packing.cpp)
TARGET_LINK_LIBRARIES(test_v1_packing libevp)
ADD_TEST(NAME v1_packing_single_file			COMMAND test_v1_packing 0 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_packing_single_file_encrypted	COMMAND test_v1_packing 1 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_packing_folder					COMMAND test_v1_packing 2 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_packing_folder_encrypted		COMMAND test_v1_packing 3 ${CMAKE_SOURCE_DIR})

ADD_EXECUTABLE(test_v1_unpacking v1/test_unpacking.cpp)
TARGET_LINK_LIBRARIES(test_v1_unpacking libevp)
ADD_TEST(NAME v1_unpacking_single_file				COMMAND test_v1_unpacking 0 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_unpacking_single_file_encrypted	COMMAND test_v1_unpacking 1 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_unpacking_folder					COMMAND test_v1_unpacking 2 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_unpacking_folder_encrypted			COMMAND test_v1_unpacking 3 ${CMAKE_SOURCE_DIR})