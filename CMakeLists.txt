﻿CMAKE_MINIMUM_REQUIRED (VERSION 3.8)

PROJECT (libevp C CXX)
ENABLE_TESTING()

IF(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    SET(OUTPUT_DIR "binaries/linux/${CMAKE_CXX_COMPILER_ID}/${CMAKE_BUILD_TYPE}")
    STRING(TOLOWER ${OUTPUT_DIR} OUTPUT_DIR_LOWER)
ELSEIF(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    SET(OUTPUT_DIR "binaries/windows/${CMAKE_CXX_COMPILER_ID}/${CMAKE_BUILD_TYPE}")
    STRING(TOLOWER ${OUTPUT_DIR} OUTPUT_DIR_LOWER)
ENDIF()

SET(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${OUTPUT_DIR_LOWER}")
SET(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${OUTPUT_DIR_LOWER}")
SET(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/${OUTPUT_DIR_LOWER}")

ADD_SUBDIRECTORY("src")
ADD_SUBDIRECTORY("test")
