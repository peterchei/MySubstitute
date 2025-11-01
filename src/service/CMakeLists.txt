cmake_minimum_required(VERSION 3.20)

# Background Service Module  
set(SERVICE_SOURCES
    background_service.cpp
    windows_service.cpp
    system_tray.cpp
)

set(SERVICE_HEADERS
    background_service.h
    windows_service.h
    system_tray.h
)

add_library(ServiceModule STATIC
    ${SERVICE_SOURCES}
    ${SERVICE_HEADERS}
)

target_include_directories(ServiceModule PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

if(WIN32)
    target_link_libraries(ServiceModule
        advapi32
        shell32
        user32
        ole32
    )
endif()