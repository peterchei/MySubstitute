cmake_minimum_required(VERSION 3.20)

# Virtual Camera Module
set(VIRTUAL_CAMERA_SOURCES
    virtual_camera_filter.cpp
    virtual_camera_directshow.cpp
    class_factory.cpp
    virtual_camera_manager.cpp
    virtual_camera_loader.cpp
    camera_interceptor.cpp
    directshow_filter.cpp
    output_pin.cpp
)

set(VIRTUAL_CAMERA_HEADERS
    virtual_camera_filter.h
    virtual_camera_directshow.h
    class_factory.h
    virtual_camera_manager.h
    virtual_camera_loader.h
    camera_interceptor.h
    directshow_filter.h
    output_pin.h
)

add_library(VirtualCamera STATIC
    ${VIRTUAL_CAMERA_SOURCES}
    ${VIRTUAL_CAMERA_HEADERS}
)

target_include_directories(VirtualCamera PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

if(WIN32)
    target_link_libraries(VirtualCamera
        ole32
        oleaut32
        uuid
        strmiids
        winmm
    )
    
    # Add DirectShow base classes if available
    if(DIRECTSHOW_INCLUDE_DIR)
        target_include_directories(VirtualCamera PRIVATE ${DIRECTSHOW_INCLUDE_DIR})
        
        # Try to find the built base classes library
        find_library(DIRECTSHOW_BASE_LIB
            NAMES strmbase strmbasd
            HINTS ${DIRECTSHOW_INCLUDE_DIR}/retail ${DIRECTSHOW_INCLUDE_DIR}/debug
        )
        
        if(DIRECTSHOW_BASE_LIB)
            target_link_libraries(VirtualCamera ${DIRECTSHOW_BASE_LIB})
        else()
            message(WARNING "DirectShow base classes library not found - you may need to build it manually")
        endif()
    endif()
endif()