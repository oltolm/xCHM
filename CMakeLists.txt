cmake_minimum_required(VERSION 3.26)

project(
  xchm
  VERSION 1.36
  LANGUAGES CXX C
)

find_program(CCACHE ccache)
if(CCACHE)
  set(CMAKE_CXX_COMPILER_LAUNCHER ccache)
endif()

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(WIN32)
  add_executable(xchm WIN32)
else()
  add_executable(xchm)
endif()

# wxWidgets
if(USE_STATIC)
  set(wxWidgets_USE_STATIC ON)
endif()
find_package(wxWidgets REQUIRED COMPONENTS core base aui html net)
include(${wxWidgets_USE_FILE})

# gtk
if(UNIX)
  find_package(GTK2)
  if(GTK2_FOUND)
    include_directories(${GTK2_INCLUDE_DIRS})
  else(GTK2_FOUND)
    find_package(GTK)
    include_directories(${GTK_INCLUDE_DIRS})
  endif(GTK2_FOUND)
endif(UNIX)

target_sources(
  xchm
  PRIVATE
  src/chmapp.cpp
  src/chmfile.cpp
  src/chmfinddialog.cpp
  src/chmfontdialog.cpp
  src/chmframe.cpp
  src/chmfshandler.cpp
  src/chmhtmlnotebook.cpp
  src/chmhtmlwindow.cpp
  src/chmindexpanel.cpp
  src/chminputstream.cpp
  src/chmlistctrl.cpp
  src/chmsearchpanel.cpp
  src/hhcparser.cpp
  src/chm_lib.c
  src/lzx.c
)

if(WIN32)
  target_sources(xchm PRIVATE rc/xchm.rc)
endif()

target_link_libraries(xchm PRIVATE ${wxWidgets_LIBRARIES} ${GTK2_LIBRARIES} ${GTK_LIBRARIES})
target_compile_definitions(xchm PRIVATE UNICODE _UNICODE ENABLE_BUILTIN_CHMLIB)
target_include_directories(xchm PRIVATE src art)
target_compile_options(xchm PRIVATE "$<$<COMPILE_LANGUAGE:CXX>:-std=c++14;-W;-Wall;-Wno-ignored-qualifiers;-Wno-cast-function-type>")
