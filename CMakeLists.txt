cmake_minimum_required(VERSION 3.10)



# Project setup

project(phosg-audio)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -Wno-deprecated)
endif()

include_directories("/usr/local/include")
link_directories("/usr/local/lib")



# Library definitions

find_package(OpenAL REQUIRED)

add_library(phosg-audio src/Constants.cc src/File.cc src/Convert.cc src/Capture.cc src/Sound.cc src/Stream.cc src/FourierTransform.cc)
target_include_directories(phosg-audio PUBLIC ${OPENAL_INCLUDE_DIR})
target_link_libraries(phosg-audio phosg ${OPENAL_LIBRARY})



# Executable definitions

add_executable(audiocat src/Audiocat.cc)
target_link_libraries(audiocat phosg phosg-audio)



# Installation configuration

install(TARGETS audiocat DESTINATION bin)

file(GLOB Headers ${CMAKE_SOURCE_DIR}/src/*.hh)
install(TARGETS phosg-audio DESTINATION lib)
install(FILES ${Headers} DESTINATION include/phosg-audio)
