cmake_minimum_required(VERSION 3.20)
project(Interceptor LANGUAGES CXX OBJCXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/build")

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew")
list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/opencv")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)
find_package(OpenCV REQUIRED COMPONENTS core imgproc highgui videoio imgcodecs)
find_package(PkgConfig REQUIRED)

find_path(EIGEN3_INCLUDE_DIR
  NAMES Eigen/Dense
  HINTS /opt/homebrew/include/eigen3 /usr/local/include/eigen3
)

if(NOT EIGEN3_INCLUDE_DIR)
  message(FATAL_ERROR "Eigen headers were not found. Install Eigen with Homebrew: brew install eigen")
endif()

set(EIGEN3_INCLUDE_DIRS "${EIGEN3_INCLUDE_DIR}")

if(PkgConfig_FOUND)
  pkg_check_modules(LIBNOVA QUIET libnova)
endif()

if(NOT LIBNOVA_FOUND)
  find_path(LIBNOVA_INCLUDE_DIR NAMES libnova/ln_types.h)
  find_library(LIBNOVA_LIBRARY NAMES nova libnova)
  if(LIBNOVA_INCLUDE_DIR AND LIBNOVA_LIBRARY)
    set(LIBNOVA_INCLUDE_DIRS "${LIBNOVA_INCLUDE_DIR}")
    set(LIBNOVA_LIBRARIES "${LIBNOVA_LIBRARY}")
  else()
    message(FATAL_ERROR "libnova was not found. Install it with Homebrew: brew install libnova")
  endif()
endif()

find_library(FOUNDATION_FRAMEWORK Foundation REQUIRED)
find_library(CORE_LOCATION_FRAMEWORK CoreLocation REQUIRED)

file(GLOB_RECURSE PROJECT_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cc"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cxx"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.m"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/*.mm"
)

list(FILTER PROJECT_SOURCES EXCLUDE REGEX "(/|^)(moc_.*\\.cpp|moc_.*\\.cxx)$")

add_executable(Interceptor ${PROJECT_SOURCES})

target_include_directories(Interceptor PRIVATE
  "${CMAKE_CURRENT_SOURCE_DIR}/src"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/sgp4"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/skyfield"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/skywatcher"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/zwo"
  ${OpenCV_INCLUDE_DIRS}
  ${LIBNOVA_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIRS}
)

target_link_libraries(Interceptor PRIVATE
  Qt6::Core
  Qt6::Gui
  Qt6::Widgets
  ${OpenCV_LIBS}
  ${LIBNOVA_LIBRARIES}
  "${CMAKE_CURRENT_SOURCE_DIR}/src/zwo/libASICamera2.dylib"
  "${FOUNDATION_FRAMEWORK}"
  "${CORE_LOCATION_FRAMEWORK}"
)

if(APPLE)
  target_compile_options(Interceptor PRIVATE -Wno-deprecated-declarations)
endif()

# Ensure the ASI camera runtime library is available next to the executable so
# dyld can load it via @loader_path/libASICamera2.dylib
add_custom_command(TARGET Interceptor POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${CMAKE_CURRENT_SOURCE_DIR}/src/zwo/libASICamera2.dylib"
    "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/libASICamera2.dylib"
)
