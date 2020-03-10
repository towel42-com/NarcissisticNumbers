# The MIT License (MIT)
#
# Copyright (c) 2020 Scott Aron Bloom
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sub-license, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
# SOFTWARE.

cmake_minimum_required(VERSION 3.1)
if(CMAKE_VERSION VERSION_LESS "3.7.0")
    set(CMAKE_INCLUDE_CURRENT_DIR ON)
endif()
project(NarcissisticNumbers)

SET(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/Modules ${CMAKE_MODULE_PATH})

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED true)
find_package(Threads REQUIRED)
find_package(Qt5 COMPONENTS Core Widgets REQUIRED)

# Get all propreties that cmake supports
execute_process(COMMAND cmake --help-property-list OUTPUT_VARIABLE CMAKE_PROPERTY_LIST)

# Convert command output into a CMake list
STRING(REGEX REPLACE ";" "\\\\;" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")
STRING(REGEX REPLACE "\n" ";" CMAKE_PROPERTY_LIST "${CMAKE_PROPERTY_LIST}")

#function(print_properties)
#    message ("CMAKE_PROPERTY_LIST = ${CMAKE_PROPERTY_LIST}")
#endfunction(print_properties)
#
#function(print_target_properties tgt)
#    if(NOT TARGET ${tgt})
#      message("There is no target named '${tgt}'")
#      return()
#    endif()
#
#    foreach (prop ${CMAKE_PROPERTY_LIST})
#        string(REPLACE "<CONFIG>" "${CMAKE_BUILD_TYPE}" prop ${prop})
#    # Fix https://stackoverflow.com/questions/32197663/how-can-i-remove-the-the-location-property-may-not-be-read-from-target-error-i
#    if(prop STREQUAL "LOCATION" OR prop MATCHES "^LOCATION_" OR prop MATCHES "_LOCATION$")
#        continue()
#    endif()
#        # message ("Checking ${prop}")
#        get_property(propval TARGET ${tgt} PROPERTY ${prop} SET)
#        if (propval)
#            get_target_property(propval ${tgt} ${prop})
#            message ("${tgt} ${prop} = ${propval}")
#        endif()
#    endforeach(prop)
#endfunction(print_target_properties)

#print_target_properties (Qt5::Widgets )
#get_target_property(_widgets_dirs Qt5::Widgets INTERFACE_INCLUDE_DIRECTORIES)
#$list(GET _widgets_dirs 0 _qt_inc_dir)
##MESSAGE( STATUS "IncDir=${_qt_inc_dir}" )
#LIST(APPEND Qt5Widgets_INCLUDE_DIRS ${_qt_inc_dir}/QtWidgets/5.12.7)
#include_directories( AFTER SYSTEM ${_qt_inc_dir}/QtWidgets/5.12.7 ${_qt_inc_dir}/QtWidgets/5.12.7/QtWidgets ${_qt_inc_dir}/QtGui/5.12.7 ${_qt_inc_dir}/QtGui/5.12.7/QtGui ${_qt_inc_dir}/QtCore/5.12.7 ${_qt_inc_dir}/QtCore/5.12.7/QtCore )


find_package(Deploy REQUIRED)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

SET(CMAKE_INSTALL_PREFIX ${CMAKE_BINARY_DIR}/Install)
#IF ( "${CPACK_RUNNING}" STREQUAL "" )
#    IF(${CMAKE_INSTALL_CONFIG_NAME} MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
#        SET(CMAKE_INSTALL_PREFIX ${CMAKE_INSTALL_PREFIX}.debug)
#    ENDIF()
#ENDIF()

INSTALL( CODE "
        MESSAGE( STATUS \"Making a \${CMAKE_INSTALL_CONFIG_NAME} install area\" )
        #IF ( \"\${CPACK_RUNNING_CPACK}\" STREQUAL \"\" )
        #    IF(\${CMAKE_INSTALL_CONFIG_NAME} MATCHES \"^([Dd][Ee][Bb][Uu][Gg])$\")
        #        SET(CMAKE_INSTALL_PREFIX \${CMAKE_INSTALL_PREFIX}.debug)
        #    ENDIF()
        #ENDIF()
        MESSAGE( STATUS \"Installing to \${CMAKE_INSTALL_PREFIX}\" )
    ")

SET(CMAKE_AUTOMOC OFF)
SET(CMAKE_AUTORCC OFF)
SET(CMAKE_AUTOUIC OFF)

include( include.cmake )
QT5_WRAP_UI(qtproject_UIS_H ${qtproject_UIS})
QT5_WRAP_CPP(qtproject_MOC_SRCS ${qtproject_H})
QT5_ADD_RESOURCES( qtproject_QRC_SRCS ${qtproject_QRC} )

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_BINARY_DIR})

source_group("Generated Files" FILES ${qtproject_UIS_H} ${qtproject_MOC_SRCS} ${qtproject_QRC_SRCS} )
source_group("Resource Files" FILES ${qtproject_QRC} ${qtproject_QRC_SOURCES} )
source_group("Designer Files" FILES ${qtproject_UIS} )
source_group("Header Files" FILES ${qtproject_H} ${project_H} )
SET( _CMAKE_FILES "CMakeLists.txt;include.cmake" )
source_group("CMake Files" FILES ${_CMAKE_FILES} )
FILE(GLOB _CMAKE_MODULE_FILES "${CMAKE_SOURCE_DIR}/Modules/*")
source_group("CMake Files\\Modues" FILES ${_CMAKE_MODULE_FILES} )

add_executable(NarcissisticNumbers WIN32
    ${project_SRCS} 
    ${project_H}  
    ${qtproject_SRCS} 
    ${qtproject_QRC} 
    ${qtproject_QRC_SRCS} 
    ${qtproject_UIS_H} 
    ${qtproject_MOC_SRCS} 
    ${qtproject_H} 
    ${qtproject_UIS}
    ${qtproject_QRC_SOURCES}
    ${_CMAKE_FILES}
    ${_CMAKE_MODULE_FILES}
)
target_link_libraries( NarcissisticNumbers 
    Qt5::Widgets
    Qt5::Core
    ${CMAKE_THREAD_LIBS_INIT}
)
SET(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION .)

DeployQt(NarcissisticNumbers .)
DeploySystem(NarcissisticNumbers)

INSTALL(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION . )
INSTALL(FILES ${CMAKE_CURRENT_BINARY_DIR}/Debug/${PROJECT_NAME}.pdb DESTINATION . CONFIGURATIONS Debug )


#SET(CPACK_PROPERTIES_FILE ${CMAKE_SOURCE_DIR}/CPackProperties.cmake)
#SET(CPACK_PROJECT_CONFIG_FILE ${PROJECT_BINARY_DIR}/CPackOptions.cmake)

SET(CPACK_PACKAGE_VENDOR "Narcissistic Number Finder")
SET(CPACK_PACKAGE_VERSION_MAJOR "1")
SET(CPACK_PACKAGE_VERSION_MINOR "0")
SET(CPACK_PACKAGE_VERSION_PATCH "0")
SET(CPACK_GENERATOR ZIP NSIS )
set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY 0)
include(CPack)
STRING(APPEND _CPACK_OTHER_VARIABLES_ "\nset(CPACK_RUNNING 1)" )
#get_cmake_property(_variableNames VARIABLES)
#list (SORT _variableNames)
#foreach (_variableName ${_variableNames})
#    message(STATUS "${_variableName}=${${_variableName}}")
#endforeach()
