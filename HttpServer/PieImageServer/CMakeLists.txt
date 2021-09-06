cmake_minimum_required(VERSION 2.8)

project(server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

IF(NOT CMAKE_BUILD_TYPE)
  SET(CMAKE_BUILD_TYPE Release)
ENDIF()

MESSAGE("Build type: " ${CMAKE_BUILD_TYPE})

SET(SRC_LIST main.cpp
    application.cpp
    benchmark.cpp
    buffer.cpp
    CJsonObject.cpp
    compress.cpp
    coordinate_transformation.cpp
    dataset.cpp
    dataset_factory.cpp
    etcd_storage.cpp
    file_cache.cpp
    handler.cpp
    handle_result.cpp
    handler_mapping.cpp
    histogram.cpp
    histogram_equalize_stretch.cpp
    jpg_buffer.cpp
    listener.cpp
    memory_pool.cpp
    min_max_stretch.cpp
    percent_min_max_stretch.cpp
    png_buffer.cpp
    png_compress.cpp
    jpg_compress.cpp
    resource_pool.cpp
    session.cpp
    standard_deviation_stretch.cpp
    stretch.cpp
    style.cpp
    style_manager.cpp
    style_map.cpp
    tiff_dataset.cpp
    tile_processor.cpp
    true_color_style.cpp
    url.cpp
    utility.cpp
    webp_compress.cpp
    wms_handler.cpp
    wmts_handler.cpp
    cJSON.c
    image_group_manager.cpp
    storage_manager.cpp
    etcd_v3.cpp
    etcd_v2.cpp
    amazon_s3.cpp
    s3_dataset.cpp)

ADD_EXECUTABLE(${PROJECT_NAME} ${SRC_LIST})
TARGET_LINK_LIBRARIES(${PROJECT_NAME} glog)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -lpthread)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -lboost_regex)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -lboost_filesystem)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -lgdal)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -lwebp)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -lpng)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -ljpeg)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -letcd-cpp-api)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -lcpprest)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -laws-cpp-sdk-core)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -laws-cpp-sdk-s3)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -laws-c-common)
TARGET_LINK_LIBRARIES(${PROJECT_NAME} -laws-c-event-stream)
