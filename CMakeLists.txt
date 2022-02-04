cmake_minimum_required (VERSION 3.5)

project(SimpleLUT)

file(GLOB SRC "${CMAKE_CURRENT_LIST_DIR}/*.cpp")

add_library(SimpleLUT SHARED ${SRC})
target_compile_features(SimpleLUT PRIVATE cxx_std_14)

if (WIN32)
    target_compile_definitions(SimpleLUT PRIVATE
        _CRT_NONSTDC_NO_WARNINGS
        _CRT_SECURE_NO_WARNINGS
    )
endif()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(SimpleLUT PRIVATE
        /Zc:__cplusplus
        /Zc:inline
    )
endif()

find_path(_AVISYNTH_INCLUDE "avisynth/avisynth.h")
find_path(AVISYNTH_INCLUDE "avisynth.h" HINTS "${_AVISYNTH_INCLUDE}/avisynth")
if (AVISYNTH_INCLUDE)
    message(STATUS "Found AviSynth headers: ${AVISYNTH_INCLUDE}")
    target_include_directories(SimpleLUT PRIVATE ${AVISYNTH_INCLUDE})
else()
    message(FATAL_ERROR "Cannot find AviSynth headers. You may specify their location in the CMAKE_INCLUDE_PATH variable.")
endif()
