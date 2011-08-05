cmake_minimum_required(VERSION 2.6.4)

project(vkontakteresource)

set(RESOURCE_VERSION 0.1)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
find_package(KDE4 REQUIRED)
include(KDE4Defaults)
include(MacroLibrary)

set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

find_program(XSLTPROC_EXECUTABLE xsltproc)
macro_log_feature(XSLTPROC_EXECUTABLE "xsltproc" "The command line XSLT processor from libxslt" "http://xmlsoft.org/XSLT/" TRUE)
set(QJSON_VERSION_REQUIRED 0.7)
find_package(QJSON ${QJSON_VERSION_REQUIRED})
macro_log_feature(QJSON_FOUND "QJSON" "Qt library for handling JSON data" "http://qjson.sourceforge.net/" TRUE "${QJSON_VERSION_REQUIRED}")

add_definitions(${QT_DEFINITIONS} ${KDE4_DEFINITIONS})

find_package(LibKVkontakte 0.1.0 REQUIRED)
macro_log_feature(LIBKVKONTAKTE_INCLUDES "libkvkontakte" "KDE C++ library implementing VKontakte.ru Open API" "http://" TRUE)

add_subdirectory(resource)

macro_display_feature_log()

