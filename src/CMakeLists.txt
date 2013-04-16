cmake_minimum_required(VERSION 2.6)
project(font-rsrc C)
enable_testing()

################################################################################
# Setup compiler
################################################################################
if(NOT CMAKE_COMPILER_IS_GNUCC)
  message(FATAL_ERROR "Unsupported compiler")
endif(NOT CMAKE_COMPILER_IS_GNUCC)

set(CMAKE_DEBUG_POSTFIX "-dbg")

set(CMAKE_C_FLAGS "-pedantic -std=c99 -Wall -Wextra -Wcast-align -Wmissing-declarations -Wmissing-prototypes -fvisibility=hidden -fstrict-aliasing -Wl,-z,defs -Wconversion")
set(CMAKE_C_FLAGS_DEBUG "-g -DDEBUG")
set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")

################################################################################
# Check dependencies
################################################################################
find_package(Freetype REQUIRED)
include_directories(${FREETYPE_INCLUDE_DIR_freetype2})

find_path(SNLSYS_INCLUDE_DIR "snlsys/snlsys.h")
find_library(SNLSYS_LIBRARY NAMES snlsys)
find_library(SNLSYS_DBG_LIBRARY NAMES snlsys-dbg)

if(NOT SNLSYS_INCLUDE_DIR OR NOT SNLSYS_LIBRARY)
  message(FATAL_ERROR "Can't find snlsys headers/library")
else()
  message(STATUS "snlsys found: ${SNLSYS_LIBRARY}")
endif()

include_directories(${SNLSYS_INCLUDE_DIR})

################################################################################
# Define targets
################################################################################
add_library(font-rsrc SHARED font_rsrc.c font_rsrc.h)
target_link_libraries(font-rsrc ${FREETYPE_LIBRARIES})
target_link_libraries(font-rsrc debug ${SNLSYS_DBG_LIBRARY})
target_link_libraries(font-rsrc optimized ${SNLSYS_LIBRARY})
set_target_properties(font-rsrc PROPERTIES DEFINE_SYMBOL FONT_SHARED_BUILD)

################################################################################
# Tests
################################################################################
add_executable(test_font_rsrc test_font_rsrc.c)
target_link_libraries(test_font_rsrc font-rsrc)

add_test(font_rsrc_6x12-iso8859-1 test_font_rsrc ../etc/6x12-iso8859-1.fon)
add_test(font_rsrc_8x13-iso8558-1 test_font_rsrc ../etc/8x13-iso8859-1.fon)
add_test(font_rsrc_TowerPrint test_font_rsrc ../etc/Tower_Print.ttf)

################################################################################
# Files to install
################################################################################
install(TARGETS font-rsrc LIBRARY DESTINATION lib)
install(FILES font_rsrc.h DESTINATION include)
install(FILES 
  ../etc/6x12-iso8859-1.fon 
  ../etc/8x13-iso8859-1.fon
  ../etc/Tower_Print.ttf
  DESTINATION etc/font/)
