# Copyright (C) 
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

set(ACE_EXPECTED_VERSION 5.8.3)
find_package(ACE REQUIRED)

if(UNIX)
  # Look for an installed ace on unix
  add_library(ace SHARED IMPORTED GLOBAL)

  set_target_properties(ace
    PROPERTIES
      IMPORTED_LOCATION
        "${ACE_LIBRARY}"
      INTERFACE_INCLUDE_DIRECTORIES
        "${ACE_INCLUDE_DIR}")
else()
    add_subdirectory(ace)
endif()

