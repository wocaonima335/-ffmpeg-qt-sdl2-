cmake_minimum_required(VERSION 3.5)

project(NewFFmpegPlay LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -pthread")





set(TARGET "NewPlayFFmpeg")
set(TEMPLATE "app")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/../bin")


find_package(Qt5 COMPONENTS Widgets REQUIRED)
find_package(Qt5 COMPONENTS Multimedia REQUIRED)
find_package(OpenGL REQUIRED)
find_package(Threads REQUIRED)


FILE(GLOB_RECURSE FORMS "*.ui")
FILE(GLOB_RECURSE RESOURCES "*.qrc")
FILE(GLOB_RECURSE HEADERS "*.h")
FILE(GLOB_RECURSE SOURCES "*cpp")

SOURCE_GROUP("Form Files" FILES ${FORMS})
SOURCE_GROUP("ReSource Files" FILES ${RESOURCES})
SOURCE_GROUP("Head Files" FILES ${HEADERS})
SOURCE_GROUP("Source Files" FILES ${SOURCES})

set(ffmpeg_include ${CMAKE_CURRENT_SOURCE_DIR}/ffmpeg)
#include_directories(${ffmpeg_include})

add_definitions(-Dffmpeg3)

if(DEFINED ffmpeg3)
    set(path_main "ffmpeg3")
    message("ffmpeg3 已经定义")
else()
    set(path_main "ffmpeg4")
    message("ffmpeg4 定义")
endif()

if(QT_ARCH MATCHES "x86_64")
    set(path_lib "libwin64")
    message(("64位"))
else()
    set(path_lib "libwin32")
    message(("32位"))
endif()



set(path_include ${ffmpeg_include}/${path_main}/include)
include_directories(${path_include})

set(path_include ${ffmpeg_include}/${path_main}/include/include)
include_directories(${path_include})

if(ANDROID)
  add_library(NewFFmpegPlay SHARED
  ${FORMS}
  ${RESOURCES}
  ${HEADERS}
  ${SOURCES}
  )
else()
  add_executable(NewFFmpegPlay
  ${FORMS}
  ${RESOURCES}
  ${HEADERS}
  ${SOURCES}
    
  )
endif()


link_directories(${ffmpeg_include}/${path_main}/${path_lib})
target_link_libraries(NewFFmpegPlay PUBLIC
    ${ffmpeg_include}/${path_main}/${path_lib}/avformat.lib
    ${ffmpeg_include}/${path_main}/${path_lib}/avfilter.lib
    ${ffmpeg_include}/${path_main}/${path_lib}/avcodec.lib
    ${ffmpeg_include}/${path_main}/${path_lib}/swresample.lib
    ${ffmpeg_include}/${path_main}/${path_lib}/swscale.lib
    ${ffmpeg_include}/${path_main}/${path_lib}/avutil.lib
    ${ffmpeg_include}/${path_main}/${path_lib}/avdevice.lib
    ${ffmpeg_include}/${path_main}/${path_lib}/SDL2.lib
    ${ffmpeg_include}/${path_main}/${path_lib}/SDL2main.lib
    ${ffmpeg_include}/${path_main}/${path_lib}/SDL2test.lib
)

target_link_libraries(NewFFmpegPlay PRIVATE Qt5::Widgets)
target_link_libraries(NewFFmpegPlay PRIVATE 
Qt5::Multimedia)
target_link_libraries(NewFFmpegPlay PRIVATE OpenGL::GL)
target_link_libraries(NewFFmpegPlay PRIVATE Threads::Threads)
