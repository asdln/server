TEMPLATE = app
CONFIG += console
CONFIG -= app_bundle
CONFIG -= qt

QMAKE_CXXFLAGS += -std=c++17

SOURCES += main.cpp \
    application.cpp \
    buffer.cpp \
    CJsonObject.cpp \
    compress.cpp \
    coordinate_transformation.cpp \
    dataset.cpp \
    dataset_factory.cpp \
    etcd_storage.cpp \
    handler.cpp \
    handle_result.cpp \
    handler_mapping.cpp \
    histogram.cpp \
    histogram_equalize_stretch.cpp \
    jpg_buffer.cpp \
    listener.cpp \
    min_max_stretch.cpp \
    percent_min_max_stretch.cpp \
    png_buffer.cpp \
    png_compress.cpp \
    resource_pool.cpp \
    session.cpp \
    standard_deviation_stretch.cpp \
    stretch.cpp \
    style.cpp \
    style_manager.cpp \
    tiff_dataset.cpp \
    tile_processor.cpp \
    true_color_style.cpp \
    url.cpp \
    utility.cpp \
    webp_compress.cpp \
    wms_handler.cpp \
    wmts_handler.cpp \
    cJSON.c \
    image_group_manager.cpp \
    storage_manager.cpp \
    etcd_v3.cpp \
    etcd_v2.cpp

HEADERS += \
    application.h \
    buffer.h \
    cJSON.h \
    CJsonObject.hpp \
    compress.h \
    concurrentqueue.h \
    coordinate_transformation.h \
    dataset.h \
    dataset_factory.h \
    etcd_storage.h \
    handler.h \
    handle_result.h \
    handler_mapping.h \
    histogram.h \
    histogram_equalize_stretch.h \
    jpg_buffer.h \
    listener.h \
    min_max_stretch.h \
    percent_min_max_stretch.h \
    png_buffer.h \
    png_compress.h \
    resource_pool.h \
    session.h \
    standard_deviation_stretch.h \
    stretch.h \
    style.h \
    style_manager.h \
    tiff_dataset.h \
    tile_processor.h \
    true_color_style.h \
    url.h \
    utility.h \
    webp_compress.h \
    wms_handler.h \
    wmts_handler.h \
    image_group_manager.h \
    storage_manager.h \
    etcd_v3.h \
    etcd_v2.h

QMAKE_CFLAGS_ISYSTEM = -I

LIBS += -lpthread

unix:!macx: LIBS += -L$$PWD/../third_party/install/lib/ -lglog

INCLUDEPATH += $$PWD/../third_party/install/include
DEPENDPATH += $$PWD/../third_party/install/include

unix:!macx: PRE_TARGETDEPS += $$PWD/../third_party/install/lib/libglog.a

unix:!macx: LIBS += -L$$PWD/../third_party/install/lib/ -lgdal

unix:!macx: LIBS += -L$$PWD/../third_party/install/lib/ -lboost_regex

unix:!macx: LIBS += -L$$PWD/../third_party/install/lib/ -lboost_filesystem

unix:!macx: LIBS += -L$$PWD/../../../../usr/lib64/ -lpng

INCLUDEPATH += $$PWD/../../../../usr/include
DEPENDPATH += $$PWD/../../../../usr/include

unix:!macx: LIBS += -L$$PWD/../../../../usr/lib64/ -lwebp


unix:!macx: LIBS += -L$$PWD/../third_party/install/lib/ -letcd-cpp-api

INCLUDEPATH += $$PWD/../third_party/install/include
DEPENDPATH += $$PWD/../third_party/install/include

INCLUDEPATH += $$PWD/../../../../usr/local/include
DEPENDPATH += $$PWD/../../../../usr/local/include

unix:!macx: LIBS += -L$$PWD/../../../../usr/local/lib/ -lcpprest

