# Copyright (C) 
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

# Enforce compileparameters for corebuilds under GCC
# This to stop a few silly crashes that could have been avoided IF people
# weren't doing some -O3 psychooptimizations etc.

if(CMAKE_COMPILER_IS_GNUCXX AND NOT MINGW)
  add_definitions(-fno-delete-null-pointer-checks)
endif()
 
  set(sources_windows_Debugging
    ${CMAKE_SOURCE_DIR}/src/server/shared/Debugging/WheatyExceptionReport.cpp
    ${CMAKE_SOURCE_DIR}/src/server/shared/Debugging/WheatyExceptionReport.h
  )
  
  #libs
  add_subdirectory(shared)
  add_subdirectory(game)
  add_subdirectory(collision)
  add_subdirectory(shade)
  add_subdirectory(scripts)
  
  #master nodes
  add_subdirectory(authserver)
  add_subdirectory(worldserver)  
  add_subdirectory(proxyserver)
  
  #sub-nodes
  #add_subdirectory(eastern_kingdoms_node)
  #add_subdirectory(kalimdor_node)
  #add_subdirectory(outland_node)
  #add_subdirectory(northrend_node)
  
  #misc nodes
  add_subdirectory(pvp_node)
  #add_subdirectory(pve_node)
  
  #misc
  #add_subdirectory(bgnode)
  #add_subdirectory(raidnode)
   