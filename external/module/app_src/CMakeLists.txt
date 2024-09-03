cmake_minimum_required(VERSION 3.20)

project(WS2812_app C)
set(C_STANDARD 11)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(FetchContent)

# TFB library does not support 24 bpp
# FetchContent_Declare(tfb
#   GIT_REPOSITORY https://github.com/vvaltchev/tfblib.git
#   GIT_TAG 99c8195761d6591254b8b847c3fd6adcf649e636
#   #SOURCE_DIR ./libs/tfblib/src
#   #BINARY_DIR ./libs/tfblib/bin
# )

set(FBG_LIB_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/external/fbg/src/fbgraphics.c
  ${CMAKE_CURRENT_SOURCE_DIR}/external/fbg/custom_backend/fbdev/fbg_fbdev.c
)

set(FBG_LIB_HEADERS_DIRS
  ${CMAKE_CURRENT_SOURCE_DIR}/external/fbg/src
  ${CMAKE_CURRENT_SOURCE_DIR}/external/fbg/custom_backend/fbdev
)

set(FBG_ADDITIONAL_FLAGS
  -D_GNU_SOURCE -D_POSIX_SOURCE -DWITHOUT_STB_IMAGE -DWITHOUT_JPEG -DWITHOUT_PNG -lm
)

message(STATUS "FBG_LIB_SOURCES: ${FBG_LIB_SOURCES}")
message(STATUS "FBG_LIB_HEADERS_DIRS: ${FBG_LIB_HEADERS_DIRS}")
message(STATUS "FBG_ADDITIONAL_FLAGS: ${FBG_ADDITIONAL_FLAGS}")

add_library(fbg STATIC ${FBG_LIB_SOURCES})
target_include_directories(fbg PUBLIC "${FBG_LIB_HEADERS_DIRS}")
target_compile_options(fbg PRIVATE "${FBG_ADDITIONAL_FLAGS}")
target_link_libraries(fbg PRIVATE m)

set(CMAKE_BUILD_TYPE Release)
# FetchContent_MakeAvailable(tfb)


add_executable(WS2812_app app_main.c)
target_include_directories(WS2812_app PUBLIC "/usr/include")
#target_include_directories(WS2812_app PRIVATE "${tfb_SOURCE_DIR}/include")
target_link_libraries(WS2812_app PRIVATE fbg)