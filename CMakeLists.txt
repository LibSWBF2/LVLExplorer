cmake_minimum_required(VERSION 3.16)

project(LVLExplorer_Project)
add_executable(LVLExplorer WIN32)

# SETTINGS
set_property(TARGET LVLExplorer PROPERTY CXX_STANDARD 17)
set_property(TARGET LVLExplorer PROPERTY CXX_STANDARD_REQUIRED ON)

# Include src
target_include_directories(LVLExplorer PRIVATE ${PROJECT_SOURCE_DIR})

# Include and link LibSWBF2
add_subdirectory("${PROJECT_SOURCE_DIR}/ThirdParty/LibSWBF2/LibSWBF2" "${CMAKE_CURRENT_BINARY_DIR}/ThirdParty/LibSWBF2")
target_include_directories(LVLExplorer PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/LibSWBF2/LibSWBF2")
target_link_libraries(LVLExplorer LibSWBF2)

# Include and link wxWidgets
set(wxBUILD_MONOLITHIC OFF CACHE BOOL "Disable wxWidgets monolithic build" FORCE)
set(wxBUILD_SHARED OFF CACHE BOOL "Build wxWidgets as static lib" FORCE)
set(wxWidgets_CONFIGURATION mswu)
add_subdirectory("${PROJECT_SOURCE_DIR}/ThirdParty/wxWidgets" "${CMAKE_CURRENT_BINARY_DIR}/ThirdParty/wxWidgets")
target_include_directories(LVLExplorer PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/wxWidgets/include")

# TODO: add include dirs for generated header files for Linux and Mac!
if(MSVC)
  target_include_directories(LVLExplorer PRIVATE "${PROJECT_SOURCE_DIR}/ThirdParty/wxWidgets/lib/vc_x64_lib/mswu")
endif()
target_link_libraries(LVLExplorer core base)

target_sources(LVLExplorer PRIVATE 
  "${PROJECT_SOURCE_DIR}/src/LVLExplorerApp.cpp"
  "${PROJECT_SOURCE_DIR}/src/LVLExplorerFrame.cpp"
  "${PROJECT_SOURCE_DIR}/src/wxImagePanel.cpp"
)

# Copy LibSWBF2 after build
if (WIN32)
  SET(LibSWBF2_FileName "LibSWBF2.dll")
elseif(UNIX AND NOT APPLE)
  SET(LibSWBF2_FileName "libSWBF2.so")
else()
  SET(LibSWBF2_FileName "libSWBF2.dylib")
endif()

add_custom_command(
  TARGET LVLExplorer POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy
          "${CMAKE_CURRENT_BINARY_DIR}/ThirdParty/LibSWBF2/${LibSWBF2_FileName}"
          "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_BUILD_TYPE}/${LibSWBF2_FileName}"
)
