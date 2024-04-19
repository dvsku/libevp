INCLUDE_DIRECTORIES("${EVP_ROOT}")
INCLUDE_DIRECTORIES("${EVP_HEADERS}")

LINK_LIBRARIES(libevp)

ADD_EXECUTABLE(test_v1_packing v1/test_packing.cpp)

ADD_TEST(NAME v1_packing_single_file COMMAND test_v1_packing 0 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_packing_folder         COMMAND test_v1_packing 1 ${CMAKE_SOURCE_DIR})

ADD_EXECUTABLE(test_v1_unpacking v1/test_unpacking.cpp)

ADD_TEST(NAME v1_unpacking_single_file      COMMAND test_v1_unpacking 0 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_unpacking_folder          COMMAND test_v1_unpacking 1 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_get_file_from_evp          COMMAND test_v1_unpacking 2 ${CMAKE_SOURCE_DIR})
ADD_TEST(NAME v1_get_file_from_evp_stream COMMAND test_v1_unpacking 3 ${CMAKE_SOURCE_DIR})

ADD_EXECUTABLE(test_misc v1/test_misc.cpp)

ADD_TEST(NAME v1_get_evp_file_list COMMAND test_misc 0 ${CMAKE_SOURCE_DIR})
