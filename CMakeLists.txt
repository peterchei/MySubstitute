cmake_minimum_required(VERSION 3.20)
project(MySubstitute)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Platform specific settings
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
    
    # DirectShow and Windows Media Foundation
    find_path(DIRECTSHOW_INCLUDE_DIR streams.h
        HINTS
        "$ENV{WindowsSdkDir}Samples/multimedia/directshow/baseclasses"
        "C:/Program Files (x86)/Microsoft SDKs/Windows Kits/10/Samples/multimedia/directshow/baseclasses"
        "C:/Program Files/Microsoft SDKs/Windows Kits/10/Samples/multimedia/directshow/baseclasses"
    )
    
    if(NOT DIRECTSHOW_INCLUDE_DIR)
        message(WARNING "DirectShow base classes not found. Please install Windows SDK with samples.")
    endif()
    
endif()

# Find required packages
find_package(OpenCV REQUIRED)
find_package(Qt6 COMPONENTS Core Widgets QUIET)

# Include directories
include_directories(src)
include_directories(${OpenCV_INCLUDE_DIRS})

if(DIRECTSHOW_INCLUDE_DIR)
    include_directories(${DIRECTSHOW_INCLUDE_DIR})
endif()

# Add subdirectories
add_subdirectory(src/capture)
add_subdirectory(src/ai)
add_subdirectory(src/virtual_camera)
add_subdirectory(src/service)

if(Qt6_FOUND)
    add_subdirectory(src/ui)
    message(STATUS "Qt6 found - building UI components")
else()
    message(WARNING "Qt6 not found - skipping UI components")
endif()

# Main executable (for testing)
add_executable(MySubstitute
    src/main.cpp
)

target_link_libraries(MySubstitute
    CaptureModule
    AIProcessor
    VirtualCamera
    ServiceModule
    ${OpenCV_LIBS}
)

if(WIN32)
    target_link_libraries(MySubstitute
        ole32
        oleaut32
        uuid
        strmiids
    )
endif()

# Installation
install(TARGETS MySubstitute
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
)