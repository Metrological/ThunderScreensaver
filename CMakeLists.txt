# If not stated otherwise in this file or this component's LICENSE file the
# following copyright and licenses apply:
#
# Copyright 2022 Metrological
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(Screensaver)

cmake_minimum_required(VERSION 3.3)

find_package(WPEFramework)

project_version(1.0.0)

set(MODULE_NAME ${NAMESPACE}${PROJECT_NAME})

message("Setup ${MODULE_NAME} v${PROJECT_VERSION}")

find_package(${NAMESPACE}Plugins CONFIG REQUIRED)
find_package(${NAMESPACE}Definitions CONFIG REQUIRED)
find_package(CompileSettingsDebug CONFIG REQUIRED)
find_package(compositorclient CONFIG REQUIRED)
find_package(${NAMESPACE}VirtualInput CONFIG REQUIRED)

add_subdirectory(esTransorm)

add_subdirectory(simpleworker)

set(PLUGIN_SCREENSAVER_AUTOSTART true CACHE STRING "Automatically start this plugin")

set(PLUGIN_SCREENSAVER_TIMEOUT 30 CACHE STRING "Timeout in seconds of inactivaty to start the show the screensaver")
set(PLUGIN_SCREENSAVER_FADEIN 1000 CACHE STRING "Fade in time in milliseconds")
set(PLUGIN_SCREENSAVER_INSTANT true CACHE STRING "Instant start the screensaver after plugin start")

set(PLUGIN_SCREENSAVER_INTERVAL 10 CACHE STRING "Interval between checks to trigger the screensaver in seconds")
set(PLUGIN_SCREENSAVER_REPORTFPS true CACHE STRING "Report FPS")

add_library(${MODULE_NAME} SHARED
    Module.cpp
    EGLRender.cpp
    EGLShader.cpp
    Screensaver.cpp)

#target_sources(${MODULE_NAME} PRIVATE
#    EGLCube.cpp)

#target_sources(${MODULE_NAME} PRIVATE
#    EGLTriangle.cpp)

set_target_properties(${MODULE_NAME} PROPERTIES
        CXX_STANDARD 11
        CXX_STANDARD_REQUIRED YES)

target_link_libraries(${MODULE_NAME}
    PRIVATE
        esTransform::esTransform
        SimpleWorker::SimpleWorker
        compositorclient::compositorclient
        ${NAMESPACE}VirtualInput::${NAMESPACE}VirtualInput
        CompileSettingsDebug::CompileSettingsDebug
        ${NAMESPACE}Plugins::${NAMESPACE}Plugins
        ${NAMESPACE}Definitions::${NAMESPACE}Definitions)

install(TARGETS ${MODULE_NAME}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/${STORAGE_DIRECTORY}/plugins)

install(DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/shaders
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/${NAMESPACE}/Screensaver)

write_config()
