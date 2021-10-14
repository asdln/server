cmake_minimum_required(VERSION 3.9)
set(CMAKE_CXX_STANDARD 11)

project(aws-tile)

find_package(aws-lambda-runtime)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF()

MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

INCLUDE_DIRECTORIES(~/aws-install/include)
INCLUDE_DIRECTORIES(~/lambda-install/include)
LINK_DIRECTORIES(~/aws-install/lib)
LINK_DIRECTORIES(~/lambda-install/lib)

SET(SRC_LIST resource_pool.cpp
    task_record.cpp
    resource_pool.cpp
    CJsonObject.cpp
    cJSON.c
    apps3.cpp
    histogram.cpp
    memory_pool.cpp
    s3_dataset.cpp
    tiff_dataset.cpp
    type_def.cpp
    )

ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_LIST})
target_link_libraries(${PROJECT_NAME} PRIVATE AWS::aws-lambda-runtime)
target_link_libraries(${PROJECT_NAME} PRIVATE -lgdal)
target_link_libraries(${PROJECT_NAME} PRIVATE -lpthread)
target_link_libraries(${PROJECT_NAME} PRIVATE -laws-cpp-sdk-core)
target_link_libraries(${PROJECT_NAME} PRIVATE -laws-cpp-sdk-s3)

target_compile_features(${PROJECT_NAME} PRIVATE "cxx_std_11")
target_compile_options(${PROJECT_NAME} PRIVATE "-Wall" "-Wextra")


aws_lambda_package_target(${PROJECT_NAME})
