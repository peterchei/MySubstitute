cmake_minimum_required(VERSION 3.20)

# AI Processing Module
set(AI_SOURCES
    ai_processor.cpp
    passthrough_processor.cpp
    background_replacer.cpp
)

set(AI_HEADERS
    ai_processor.h
    passthrough_processor.h
    background_replacer.h
)

add_library(AIProcessor STATIC
    ${AI_SOURCES}
    ${AI_HEADERS}
)

target_include_directories(AIProcessor PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(AIProcessor
    ${OpenCV_LIBS}
)