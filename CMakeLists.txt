cmake_minimum_required (VERSION 3.5.1) # Just the current tested version
project (imgkap)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

find_package(FreeImage)
if(NOT FreeImage_FOUND)
  set(FREEIMAGE_BUILD_STATIC 1)
  add_subdirectory(freeimage)
endif()


# clang++ -I../freeimage/Source/ -o imgkap imgkap.c -lFreeImage_static -L../freeimage/build_a/lib/ -lpng -lz

add_executable(imgkap imgkap.c imgkap.h imgkap_main.c kml.h kml.c)
set_property(TARGET imgkap PROPERTY CXX_STANDARD 14)
target_link_libraries(imgkap FreeImage::FreeImage m)
