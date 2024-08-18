cmake_minimum_required(VERSION 3.10)



# Project setup

project(phosg-audio)
include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)
if (MSVC)
    add_compile_options(/W4 /WX)
else()
    add_compile_options(-Wall -Wextra -Werror -Wno-deprecated)
endif()

find_package(phosg REQUIRED)



# Library definitions

find_package(OpenAL REQUIRED)

add_library(
  phosg-audio
  src/Capture.cc
  src/Constants.cc
  src/Convert.cc
  src/File.cc
  src/FourierTransform.cc
  src/Sound.cc
  src/Stream.cc
)
target_include_directories(phosg-audio PUBLIC ${OPENAL_INCLUDE_DIR})
target_link_libraries(phosg-audio phosg::phosg ${OPENAL_LIBRARY})



# Executable definitions

add_executable(audiocat src/Audiocat.cc)
target_link_libraries(audiocat phosg-audio)



# Installation configuration


# Library package setup
install(
  TARGETS phosg-audio
  EXPORT phosg-audio
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Header files
file(GLOB Headers ${CMAKE_SOURCE_DIR}/src/*.hh)
install(FILES ${Headers} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/phosg-audio)

# Export definition
install(
  EXPORT phosg-audio
  FILE phosg-audio.cmake
  NAMESPACE phosg-audio::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/phosg-audio
)

# CMake config files
configure_package_config_file(${CMAKE_CURRENT_SOURCE_DIR}/phosg-audioConfig.cmake.in
  "${CMAKE_CURRENT_BINARY_DIR}/phosg-audioConfig.cmake"
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/phosg-audio
)
set(version 0.1)
set_property(TARGET phosg-audio PROPERTY VERSION ${version})
set_property(TARGET phosg-audio PROPERTY SOVERSION 0)
set_property(TARGET phosg-audio PROPERTY INTERFACE_phosg-audio_MAJOR_VERSION 0)
set_property(TARGET phosg-audio APPEND PROPERTY COMPATIBLE_INTERFACE_STRING phosg-audio_MAJOR_VERSION)
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/phosg-audioConfigVersion.cmake"
  VERSION "${version}"
  COMPATIBILITY AnyNewerVersion
)
install(
  FILES "${CMAKE_CURRENT_BINARY_DIR}/phosg-audioConfig.cmake" "${CMAKE_CURRENT_BINARY_DIR}/phosg-audioConfigVersion.cmake"
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/phosg-audio
)

# Executables
install(TARGETS audiocat DESTINATION bin)
