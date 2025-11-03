cmake_minimum_required(VERSION 3.20)

# AI Processing Module
set(AI_SOURCES
    ai_processor.cpp
    passthrough_processor.cpp
    face_filter_processor.cpp
    cartoon_filter_processor.cpp
    cartoon_buffered_filter_processor.cpp
)

set(AI_HEADERS
    ai_processor.h
    passthrough_processor.h
    face_filter_processor.h
    cartoon_filter_processor.h
    cartoon_buffered_filter_processor.h
)

add_library(AIProcessor STATIC
    ${AI_SOURCES}
    ${AI_HEADERS}
)

target_include_directories(AIProcessor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Configure preprocessor definitions and OpenCV linking
if(HAVE_OPENCV)
    target_compile_definitions(AIProcessor PRIVATE HAVE_OPENCV=1)
    target_link_libraries(AIProcessor ${OpenCV_LIBS})
else()
    target_compile_definitions(AIProcessor PRIVATE HAVE_OPENCV=0)
endif()