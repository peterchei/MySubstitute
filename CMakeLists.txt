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

# Add subdirectories
add_subdirectory(src/ai)

# DirectShow base classes (usually in Windows SDK)
if(WIN32)
    # Try common DirectShow base class locations
    set(DIRECTSHOW_BASE_PATHS
        "C:/Program Files (x86)/Microsoft SDKs/Windows/v7.1A/Samples/multimedia/directshow/baseclasses"
        "C:/Program Files/Microsoft SDKs/Windows/v7.1A/Samples/multimedia/directshow/baseclasses"
        "C:/Program Files (x86)/Windows Kits/10/Samples/multimedia/directshow/baseclasses"
        "$ENV{WindowsSdkDir}/Samples/multimedia/directshow/baseclasses"
    )
    
    foreach(path ${DIRECTSHOW_BASE_PATHS})
        if(EXISTS "${path}")
            include_directories("${path}")
            message(STATUS "Found DirectShow base classes at: ${path}")
            break()
        endif()
    endforeach()
endif()

# Create a simple executable with all sources in one place
set(SOURCES
    src/main.cpp
    src/capture/camera_capture.cpp
    src/capture/frame.cpp
    src/virtual_camera/virtual_camera_filter.cpp
    src/virtual_camera/virtual_camera_directshow.cpp
    src/virtual_camera/class_factory.cpp
    src/virtual_camera/virtual_camera_manager.cpp
    src/virtual_camera/camera_diagnostics.cpp
    src/virtual_camera/simple_registry_virtual_camera.cpp
    src/virtual_camera/media_foundation_camera.cpp
    src/virtual_camera/directshow_virtual_camera_manager.cpp
    src/virtual_camera/virtual_camera_registry.cpp
    src/virtual_camera/directshow_source_filter.cpp
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
        AIProcessor   # AI processing library
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
        strmbase      # DirectShow base classes
        quartz        # DirectShow quartz
        gdi32         # GDI for drawing
        gdiplus       # GDI+ for advanced drawing
    )
endif()

# Set target properties
set_target_properties(MySubstitute PROPERTIES
    WIN32_EXECUTABLE TRUE
    DEBUG_POSTFIX "_d"
)

#
# DirectShow Virtual Camera DLL Target
#
add_library(MySubstituteVirtualCameraDLL SHARED
    src/virtual_camera/virtual_camera_directshow.cpp
    src/virtual_camera/virtual_camera_directshow.h
    src/virtual_camera/directshow_dll_main.cpp
    src/capture/frame.cpp
    src/capture/frame.h
)

# Set DLL output name
set_target_properties(MySubstituteVirtualCameraDLL PROPERTIES
    OUTPUT_NAME "MySubstituteVirtualCamera"
    SUFFIX ".dll"
    DEBUG_POSTFIX "_d"
)

# Include directories for DLL
target_include_directories(MySubstituteVirtualCameraDLL PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link libraries for DLL
target_link_libraries(MySubstituteVirtualCameraDLL
    strmiids
    ole32
    oleaut32
    uuid
    winmm
)

# OpenCV for DLL - needed for video processing
if(HAVE_OPENCV)
    target_link_libraries(MySubstituteVirtualCameraDLL ${OpenCV_LIBS})
    target_compile_definitions(MySubstituteVirtualCameraDLL PRIVATE HAVE_OPENCV=1)
else()
    target_compile_definitions(MySubstituteVirtualCameraDLL PRIVATE HAVE_OPENCV=0)
endif()

# DLL preprocessor definitions
target_compile_definitions(MySubstituteVirtualCameraDLL PRIVATE
    WIN32_LEAN_AND_MEAN
    _CRT_SECURE_NO_WARNINGS
    MYSUBSTITUTE_DLL_EXPORTS
)

# Use static runtime to eliminate dependency issues
target_compile_options(MySubstituteVirtualCameraDLL PRIVATE
    $<$<CONFIG:Release>:/MT>
    $<$<CONFIG:Debug>:/MTd>
)

# Module definition file for DLL exports
set_target_properties(MySubstituteVirtualCameraDLL PROPERTIES
    LINK_FLAGS "/DEF:${CMAKE_SOURCE_DIR}/src/virtual_camera/MySubstituteVirtualCamera.def"
)

# Copy DLL to main executable directory with COM-safe handling
add_custom_command(TARGET MySubstituteVirtualCameraDLL POST_BUILD
    COMMAND powershell.exe -ExecutionPolicy Bypass -File 
        "${CMAKE_SOURCE_DIR}/scripts/copy_dll_safe.ps1"
        "$<TARGET_FILE:MySubstituteVirtualCameraDLL>"
        "$<TARGET_FILE_DIR:MySubstitute>/MySubstituteVirtualCamera.dll"
    COMMENT "Safely copying DirectShow DLL (handles COM registration)"
)

# Test executables
add_executable(test_face_filter
    test_face_filter.cpp
)

target_include_directories(test_face_filter PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_face_filter
    AIProcessor
    ${OpenCV_LIBS}
)

# Configure preprocessor definitions for test
if(HAVE_OPENCV)
    target_compile_definitions(test_face_filter PRIVATE HAVE_OPENCV=1)
else()
    target_compile_definitions(test_face_filter PRIVATE HAVE_OPENCV=0)
endif()

add_executable(test_filter_callback
    test_filter_callback.cpp
)

target_include_directories(test_filter_callback PRIVATE
    ${CMAKE_SOURCE_DIR}/src
)

target_link_libraries(test_filter_callback
    AIProcessor
    ${OpenCV_LIBS}
)

# Configure preprocessor definitions for test
if(HAVE_OPENCV)
    target_compile_definitions(test_filter_callback PRIVATE HAVE_OPENCV=1)
else()
    target_compile_definitions(test_filter_callback PRIVATE HAVE_OPENCV=0)
endif()