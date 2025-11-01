cmake_minimum_required(VERSION 3.16)
project(MySubstitute VERSION 1.0.0)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set output directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# Windows specific settings
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
    add_definitions(-DNOMINMAX)
endif()

# Find OpenCV (make it optional for now)
find_package(OpenCV QUIET)
if(NOT OpenCV_FOUND)
    message(WARNING "OpenCV not found - some features will be disabled")
    set(HAVE_OPENCV FALSE)
else()
    message(STATUS "OpenCV found: ${OpenCV_VERSION}")
    set(HAVE_OPENCV TRUE)
endif()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src)
if(HAVE_OPENCV)
    include_directories(${OpenCV_INCLUDE_DIRS})
endif()

# Create a simple executable with all sources in one place
set(SOURCES
    src/main.cpp
    src/capture/camera_capture.cpp
    src/capture/frame.cpp
    src/ai/ai_processor.cpp
    src/ai/passthrough_processor.cpp
    src/virtual_camera/virtual_camera_filter.cpp
    src/service/background_service.cpp
    src/ui/system_tray_manager.cpp
    src/ui/preview_window_manager.cpp
)

# Resource files (optional, commented out for now)
# set(RESOURCES
#     src/resources.rc
# )

# Create executable
add_executable(MySubstitute ${SOURCES})

# Configure preprocessor definitions
if(HAVE_OPENCV)
    target_compile_definitions(MySubstitute PRIVATE HAVE_OPENCV=1)
    target_link_libraries(MySubstitute ${OpenCV_LIBS})
else()
    target_compile_definitions(MySubstitute PRIVATE HAVE_OPENCV=0)
endif()

# Link Windows libraries
if(WIN32)
    target_link_libraries(MySubstitute
        ole32         # COM
        oleaut32      # COM automation
        uuid          # GUID support
        strmiids      # DirectShow
        winmm         # Windows multimedia
        mfplat        # Media Foundation
        mf            # Media Foundation
        mfuuid        # Media Foundation UUIDs
        user32        # Windows user interface
        shell32       # Shell API (for system tray)
    )
endif()

# Set target properties
set_target_properties(MySubstitute PROPERTIES
    WIN32_EXECUTABLE TRUE
    DEBUG_POSTFIX "_d"
)