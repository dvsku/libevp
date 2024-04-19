INCLUDE_DIRECTORIES("${EVP_ROOT}")
INCLUDE_DIRECTORIES("${EVP_HEADERS}")
INCLUDE_DIRECTORIES("${GTEST_INCLUDES}")

INCLUDE(GoogleTest)

ADD_COMPILE_DEFINITIONS("BASE_PATH=\"${EVP_ROOT}\"")
LINK_LIBRARIES(libevp)
LINK_LIBRARIES(GTest::gtest_main)

ADD_EXECUTABLE(test_v1_packing
	"v1/test_packing.cpp"
)

gtest_discover_tests(test_v1_packing)

ADD_EXECUTABLE(test_v1_unpacking
	"v1/test_unpacking.cpp"
)

gtest_discover_tests(test_v1_unpacking)

ADD_EXECUTABLE(test_misc 
	"v1/test_misc.cpp"
)

gtest_discover_tests(test_misc)
