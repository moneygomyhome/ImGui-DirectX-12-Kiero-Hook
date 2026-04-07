cmake_minimum_required(VERSION 3.15)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(SOURCE_FILES
    esp_main.cpp
    esp_hud.cpp

        SDK/Engine_functions.cpp
        SDK/CoreUObject_functions.cpp
        SDK/Basic.cpp
)

foreach(SOURCE_FILE ${SOURCE_FILES})
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${SOURCE_FILE}")
        message(FATAL_ERROR "Source file not found: ${SOURCE_FILE}")
    endif()
endforeach()

include_directories(
    SDK
)

add_library("ESPMod" STATIC ${SOURCE_FILES})

if(MSVC)
    add_compile_options( /bigobj)
endif()