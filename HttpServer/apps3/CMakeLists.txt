cmake_minimum_required(VERSION 2.8)

project(aws-tile)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF()

MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

INCLUDE_DIRECTORIES(../aws-install/include)
LINK_DIRECTORIES(../aws-install/lib)

SET(SRC_LIST resource_pool.cpp
    task_record.cpp
    CJsonObject.cpp
    cJSON.c
    apps3.cpp
    )

ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_LIST})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -lgdal)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -lpthread)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -laws-cpp-sdk-core)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -laws-cpp-sdk-s3)
