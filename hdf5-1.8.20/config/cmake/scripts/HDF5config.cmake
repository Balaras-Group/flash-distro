#
# Copyright by The HDF Group.
# All rights reserved.
#
# This file is part of HDF5.  The full HDF5 copyright notice, including
# terms governing use, modification, and redistribution, is contained in
# the COPYING file, which can be found at the root of the source code
# distribution tree, or in https://support.hdfgroup.org/ftp/HDF5/releases.
# If you do not have access to either file, you may request a copy from
# help@hdfgroup.org.

#############################################################################################
### ${CTEST_SCRIPT_ARG} is of the form OPTION=VALUE                                       ###
### BUILD_GENERATOR required [Unix, VS2017, VS201764, VS2015, VS201564, VS2013, VS201364] ###
### ctest -S HDF518config.cmake,BUILD_GENERATOR=VS201264 -C Release -VV -O hdf518.log     ###
#############################################################################################

cmake_minimum_required (VERSION 3.2.2 FATAL_ERROR)
############################################################################
# Usage:
#     ctest -S HDF518config.cmake,OPTION=VALUE -C Release -VV -O test.log
# where valid options for OPTION are:
#     BUILD_GENERATOR - The cmake build generator:
#            Unix      * Unix Makefiles
#            VS2017    * Visual Studio 15 2017
#            VS201764  * Visual Studio 15 2017 Win64
#            VS2015    * Visual Studio 14 2015
#            VS201564  * Visual Studio 14 2015 Win64
#            VS2013    * Visual Studio 12 2013
#            VS201364  * Visual Studio 12 2013 Win64
#
#     INSTALLDIR  -  root folder where hdf5 is installed
#     CTEST_CONFIGURATION_TYPE  - Release, Debug, etc
#     CTEST_SOURCE_NAME  -  source folder
#     STATIC_ONLY  -  Build/use static libraries
#     FORTRAN_LIBRARIES -  Build/use fortran libraries
#     NO_MAC_FORTRAN  - Yes to be SHARED on a Mac
##############################################################################

set (CTEST_SOURCE_VERSION "1.8.20")
set (CTEST_SOURCE_VERSEXT "")

##############################################################################
# handle input parameters to script.
#BUILD_GENERATOR - which CMake generator to use, required
#INSTALLDIR - HDF5-1.8 root folder
#CTEST_CONFIGURATION_TYPE - Release, Debug, RelWithDebInfo
#CTEST_SOURCE_NAME - name of source folder; HDF5-1.8.20
#STATIC_ONLY - Default is YES
#FORTRAN_LIBRARIES - Default is NO
#NO_MAC_FORTRAN - set to TRUE to allow shared libs on a Mac
if (DEFINED CTEST_SCRIPT_ARG)
    # transform ctest script arguments of the form
    # script.ctest,var1=value1,var2=value2
    # to variables with the respective names set to the respective values
    string (REPLACE "," ";" script_args "${CTEST_SCRIPT_ARG}")
    foreach (current_var ${script_args})
        if ("${current_var}" MATCHES "^([^=]+)=(.+)$")
            set ("${CMAKE_MATCH_1}" "${CMAKE_MATCH_2}")
        endif ()
    endforeach ()
endif ()

# build generator must be defined
if (NOT DEFINED BUILD_GENERATOR)
  message (FATAL_ERROR "BUILD_GENERATOR must be defined - Unix, VS2017, or VS201764, VS2015, VS201564, VS2013, VS201364")
endif ()

###################################################################
### Following Line is one of [Release, RelWithDebInfo, Debug] #####
set (CTEST_CONFIGURATION_TYPE "$ENV{CMAKE_CONFIG_TYPE}")
###################################################################

if (NOT DEFINED INSTALLDIR)
  if (WIN32)
    set (INSTALLDIR "C:/Program Files/HDF_Group/HDF5/${CTEST_SOURCE_VERSION}")
  else ()
    set (INSTALLDIR "${CTEST_SCRIPT_DIRECTORY}/HDF_Group/HDF5/${CTEST_SOURCE_VERSION}")
  endif ()
endif ()
if (NOT DEFINED CTEST_CONFIGURATION_TYPE)
  set (CTEST_CONFIGURATION_TYPE "Release")
endif ()
if (NOT DEFINED CTEST_SOURCE_NAME)
  set (CTEST_SOURCE_NAME "hdf5-${CTEST_SOURCE_VERSION}${CTEST_SOURCE_VERSEXT}")
endif ()
if (NOT DEFINED STATIC_ONLY)
  set (STATICONLYLIBRARIES "YES")
else ()
  set (STATICONLYLIBRARIES "NO")
endif ()
if (NOT DEFINED FORTRAN_LIBRARIES)
  set (FORTRANLIBRARIES "NO")
else ()
  set(FORTRANLIBRARIES "YES")
endif ()

set (CTEST_BINARY_NAME "build")
set (CTEST_DASHBOARD_ROOT "${CTEST_SCRIPT_DIRECTORY}")
if (WIN32)
  set (CTEST_SOURCE_DIRECTORY "${CTEST_DASHBOARD_ROOT}\\${CTEST_SOURCE_NAME}")
  set (CTEST_BINARY_DIRECTORY "${CTEST_DASHBOARD_ROOT}\\${CTEST_BINARY_NAME}")
else ()
  set (CTEST_SOURCE_DIRECTORY "${CTEST_DASHBOARD_ROOT}/${CTEST_SOURCE_NAME}")
  set (CTEST_BINARY_DIRECTORY "${CTEST_DASHBOARD_ROOT}/${CTEST_BINARY_NAME}")
endif ()

###################################################################
#########       Following describes compiler           ############
if (WIN32)
  set (SITE_OS_NAME "Windows")
  set (SITE_OS_VERSION "WIN7")
  if (${BUILD_GENERATOR} STREQUAL "VS201764")
    set (CTEST_CMAKE_GENERATOR "Visual Studio 15 2017 Win64")
    set (SITE_OS_BITS "64")
    set (SITE_COMPILER_NAME "vs2017")
    set (SITE_COMPILER_VERSION "15")
  elseif (${BUILD_GENERATOR} STREQUAL "VS2017")
    set (CTEST_CMAKE_GENERATOR "Visual Studio 15 2017")
    set (SITE_OS_BITS "32")
    set (SITE_COMPILER_NAME "vs2017")
    set (SITE_COMPILER_VERSION "15")
  elseif (${BUILD_GENERATOR} STREQUAL "VS201564")
    set (CTEST_CMAKE_GENERATOR "Visual Studio 14 2015 Win64")
    set (SITE_OS_BITS "64")
    set (SITE_COMPILER_NAME "vs2015")
    set (SITE_COMPILER_VERSION "14")
  elseif (${BUILD_GENERATOR} STREQUAL "VS2015")
    set (CTEST_CMAKE_GENERATOR "Visual Studio 14 2015")
    set (SITE_OS_BITS "32")
    set (SITE_COMPILER_NAME "vs2015")
    set (SITE_COMPILER_VERSION "14")
  elseif (${BUILD_GENERATOR} STREQUAL "VS201364")
    set (CTEST_CMAKE_GENERATOR "Visual Studio 12 2013 Win64")
    set (SITE_OS_BITS "64")
    set (SITE_COMPILER_NAME "vs2013")
    set (SITE_COMPILER_VERSION "12")
  elseif (${BUILD_GENERATOR} STREQUAL "VS2013")
    set (CTEST_CMAKE_GENERATOR "Visual Studio 12 2013")
    set (SITE_OS_BITS "32")
    set (SITE_COMPILER_NAME "vs2013")
    set (SITE_COMPILER_VERSION "12")
  elseif (${BUILD_GENERATOR} STREQUAL "VS201264")
    set (CTEST_CMAKE_GENERATOR "Visual Studio 11 2012 Win64")
    set (SITE_OS_BITS "64")
    set (SITE_COMPILER_NAME "vs2012")
    set (SITE_COMPILER_VERSION "11")
  elseif (${BUILD_GENERATOR} STREQUAL "VS2012")
    set (CTEST_CMAKE_GENERATOR "Visual Studio 11 2012")
    set (SITE_OS_BITS "32")
    set (SITE_COMPILER_NAME "vs2012")
    set (SITE_COMPILER_VERSION "11")
  else ()
    message (FATAL_ERROR "Invalid BUILD_GENERATOR must be - Unix, VS2017, or VS201764, VS2015, VS201564, VS2013, VS201364")
  endif ()
##  Set the following to unique id your computer  ##
  set (CTEST_SITE "WIN7${BUILD_GENERATOR}.XXXX")
else ()
  set (CTEST_CMAKE_GENERATOR "Unix Makefiles")
##  Set the following to unique id your computer  ##
  if (APPLE)
    set (CTEST_SITE "MAC.XXXX")
  else ()
    set (CTEST_SITE "LINUX.XXXX")
  endif ()
  if (APPLE)
    execute_process (COMMAND xcrun --find cc OUTPUT_VARIABLE XCODE_CC OUTPUT_STRIP_TRAILING_WHITESPACE)
    execute_process (COMMAND xcrun --find c++ OUTPUT_VARIABLE XCODE_CXX OUTPUT_STRIP_TRAILING_WHITESPACE)
    set (ENV{CC} "${XCODE_CC}")
    set (ENV{CXX} "${XCODE_CXX}")
    set (CTEST_USE_LAUNCHERS        1)
    set (RR_WARNINGS_COMMON "-Wno-format-nonliteral -Wno-cast-align -Wno-unused -Wno-unused-variable -Wno-unused-function -Wno-self-assign -Wno-unused-parameter -Wno-sign-compare")
    set (RR_WARNINGS_C "${RR_WARNINGS_COMMON} -Wno-deprecated-declarations -Wno-uninitialized")
    set (RR_WARNINGS_CXX "${RR_WARNINGS_COMMON} -Woverloaded-virtual -Wshadow -Wwrite-strings -Wc++11-compat")
    set (RR_FLAGS_COMMON "-g -O0 -fstack-protector-all -D_FORTIFY_SOURCE=2")
    set (RR_FLAGS_C "${RR_FLAGS_COMMON}")
    set (RR_FLAGS_CXX "${RR_FLAGS_COMMON}")
    set (ENV{CFLAGS} "${RR_WARNINGS_C} ${RR_FLAGS_C}")
    set (ENV{CXXFLAGS} "${RR_WARNINGS_CXX} ${RR_FLAGS_CXX}")
  endif ()
endif ()
###################################################################

###################################################################
#########       Following is for submission to CDash   ############
###################################################################
set (MODEL "Experimental")
###################################################################

###################################################################
#####       Following controls CDash submission               #####
#set (LOCAL_SUBMIT "TRUE")
#####       Following controls test process                   #####
#set (LOCAL_SKIP_TEST "TRUE")
#set (LOCAL_MEMCHECK_TEST "TRUE")
#set (LOCAL_COVERAGE_TEST "TRUE")
#####       Following controls cpack command                  #####
#set (LOCAL_NO_PACKAGE "TRUE")
#####       Following controls source update                  #####
#set (LOCAL_UPDATE "TRUE")
set (REPOSITORY_URL "https://git@bitbucket.hdfgroup.org/scm/hdffv/hdf5.git")
set (REPOSITORY_BRANCH "hdf5_1_8_20")

#uncomment to use a compressed source file: *.tar on linux or mac *.zip on windows
#set(CTEST_USE_TAR_SOURCE "${CTEST_SOURCE_VERSION}")
###################################################################

###################################################################
if (${STATICONLYLIBRARIES})
  set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DBUILD_SHARED_LIBS:BOOL=OFF")
  #########       Following describes computer           ############
  ## following is optional to describe build                       ##
  set (SITE_BUILDNAME_SUFFIX "STATIC")
endif ()
###################################################################
####      fortran       ####
if (${FORTRANLIBRARIES})
  set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DHDF5_BUILD_FORTRAN:BOOL=ON")
  ### enable Fortran 2003 depends on HDF5_BUILD_FORTRAN
  set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DHDF5_ENABLE_F2003:BOOL=ON")
else ()
  set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DHDF5_BUILD_FORTRAN:BOOL=OFF")
  ### enable Fortran 2003 depends on HDF5_BUILD_FORTRAN
  set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DHDF5_ENABLE_F2003:BOOL=OFF")
endif ()

### change install prefix
set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DCMAKE_INSTALL_PREFIX:PATH=${INSTALLDIR}")
set (ADD_BUILD_OPTIONS "${ADD_BUILD_OPTIONS} -DCTEST_CONFIGURATION_TYPE:STRING=$ENV{CMAKE_CONFIG_TYPE}")

###################################################################

if (WIN32)
  set (BINFILEBASE "HDF5-${CTEST_SOURCE_VERSION}${CTEST_SOURCE_VERSEXT}-win${SITE_OS_BITS}")
  include (${CTEST_SCRIPT_DIRECTORY}\\HDF5options.cmake)
  include (${CTEST_SCRIPT_DIRECTORY}\\CTestScript.cmake)
  if (EXISTS "${CTEST_BINARY_DIRECTORY}\\${BINFILEBASE}.exe")
    file (COPY "${CTEST_BINARY_DIRECTORY}\\${BINFILEBASE}.exe" DESTINATION ${CTEST_SCRIPT_DIRECTORY})
  endif ()
  if (EXISTS "${CTEST_BINARY_DIRECTORY}\\${BINFILEBASE}.msi")
    file (COPY "${CTEST_BINARY_DIRECTORY}\\${BINFILEBASE}.msi" DESTINATION  ${CTEST_SCRIPT_DIRECTORY})
  endif ()
  if (EXISTS "${CTEST_BINARY_DIRECTORY}\\${BINFILEBASE}.zip")
    file (COPY "${CTEST_BINARY_DIRECTORY}\\${BINFILEBASE}.zip" DESTINATION  ${CTEST_SCRIPT_DIRECTORY})
  endif ()
else ()
  set (BINFILEBASE "HDF5-${CTEST_SOURCE_VERSION}${CTEST_SOURCE_VERSEXT}")
  include (${CTEST_SCRIPT_DIRECTORY}/HDF5options.cmake)
  include (${CTEST_SCRIPT_DIRECTORY}/CTestScript.cmake)
  if (APPLE)
    if (EXISTS "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-Darwin.dmg")
      file (COPY "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-Darwin.dmg" DESTINATION ${CTEST_SCRIPT_DIRECTORY})
    endif ()
    if (EXISTS "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-Darwin.tar.gz")
      file (COPY "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-Darwin.tar.gz" DESTINATION  ${CTEST_SCRIPT_DIRECTORY})
    endif ()
    if (EXISTS "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-Darwin.sh")
      file (COPY "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-Darwin.sh" DESTINATION ${CTEST_SCRIPT_DIRECTORY})
    endif ()
  else ()
    if (CYGWIN)
      if (EXISTS "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-CYGWIN.sh")
        file (COPY "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-CYGWIN.sh" DESTINATION ${CTEST_SCRIPT_DIRECTORY})
      endif ()
      if (EXISTS "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-CYGWIN.tar.gz")
        file (COPY "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-CYGWIN.tar.gz" DESTINATION  ${CTEST_SCRIPT_DIRECTORY})
      endif ()
    else ()
      if (EXISTS "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-Linux.sh")
        file (COPY "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-Linux.sh" DESTINATION ${CTEST_SCRIPT_DIRECTORY})
      endif ()
      if (EXISTS "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-Linux.tar.gz")
        file (COPY "${CTEST_BINARY_DIRECTORY}/${BINFILEBASE}-Linux.tar.gz" DESTINATION  ${CTEST_SCRIPT_DIRECTORY})
      endif ()
    endif ()
  endif ()
endif ()
