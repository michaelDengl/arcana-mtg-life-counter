cmake_minimum_required(VERSION 3.24)
set(SDKCONFIG_DEFAULTS "sdkconfig.defaults")
if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/sdkconfig.local")
 list(APPEND SDKCONFIG_DEFAULTS "sdkconfig.local")
endif()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
set(PROJECT_VER "0.17.3")
project(arcana)

