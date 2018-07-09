# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#.rst:
# FindFreeImage
# -------------
#
# Finds the FreeImage library
#
# This will define the following variables
#
#   FreeImage_FOUND - True if FreeImages is found

find_package(PkgConfig)
pkg_check_modules(PC_FreeImage QUIET FreeImage)

find_path(FreeImage_INCLUDE_DIR
  NAMES FreeImage.h
  PATHS ${PC_FreeImage_INCLUDE_DIRS}
  PATH_SUFFIXES FreeImage
)
find_library(FreeImage_LIBRARY
    NAMES FreeImage freeimage
  PATHS ${PC_FreeImage_LIBRARY_DIRS}
)
set(Foo_VERSION ${PC_FreeImage_VERSION})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FreeImage
    FOUND_VAR FreeImage_FOUND
  REQUIRED_VARS
    FreeImage_LIBRARY
    FreeImage_INCLUDE_DIR
  VERSION_VAR FreeImage_VERSION
)

if(FreeImage_FOUND AND NOT TARGET FreeImage::FreeImage)
    add_library(FreeImage::FreeImage UNKNOWN IMPORTED)
    set_target_properties(FreeImage::FreeImage PROPERTIES
        IMPORTED_LOCATION "${FreeImage_LIBRARY}"
        INTERFACE_COMPILE_OPTIONS "${PC_FreeImage_CFLAGS_OTHER}"
        INTERFACE_INCLUDE_DIRECTORIES "${FreeImage_INCLUDE_DIR}"
  )
    # target_link_libraries(FreeImage::FreeImage INTERFACE m)
endif()

mark_as_advanced(
    FreeImage_INCLUDE_DIR
    FreeImage_LIBRARY
)
