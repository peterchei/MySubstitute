cmake_minimum_required(VERSION 3.20)

# Camera Capture Module
set(CAPTURE_SOURCES
    camera_capture.cpp
    directshow_capture.cpp
)

set(CAPTURE_HEADERS
    camera_capture.h
    directshow_capture.h
    frame.h
)

add_library(CaptureModule STATIC
    ${CAPTURE_SOURCES}
    ${CAPTURE_HEADERS}
)

target_include_directories(CaptureModule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(CaptureModule
    ${OpenCV_LIBS}
)

if(WIN32)
    target_link_libraries(CaptureModule
        ole32
        oleaut32
        uuid
        strmiids
        mf
        mfplat
        mfuuid
    )
endif()