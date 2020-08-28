# Copyright (C) 2008-2010 Trinity <http://www.trinitycore.org/>
#
# This file is free software; as a special exception the author gives
# unlimited permission to copy and/or distribute it, with or without
# modifications, as long as this notice is preserved.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY, to the extent permitted by law; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.

file(GLOB_RECURSE sources_CommandLine CommandLine/*.cpp CommandLine/*.h)
file(GLOB_RECURSE sources_RemoteAccess RemoteAccess/*.cpp RemoteAccess/*.h)
file(GLOB_RECURSE sources_TCSoap TCSoap/*.cpp TCSoap/*.h)
file(GLOB_RECURSE sources_ProxyThread ProxyThread/*.cpp ProxyThread/*.h)
file(GLOB sources_localdir *.cpp *.h)

if( USE_COREPCH )
  include_directories(${CMAKE_CURRENT_BINARY_DIR})
endif()

if ( USE_COREPCH AND MSVC )
  set(proxyserver_SRCS
    PrecompiledHeaders/proxyPCH.cpp
    PrecompiledHeaders/proxyPCH.h
  )
endif()

set(proxyserver_SRCS
  ${proxyserver_SRCS}
  ${sources_CommandLine}
##  ${sources_RemoteAccess}
##  ${sources_TCSoap}
  ${sources_ProxyThread}
  ${sources_localdir}
)

if( WIN32 )
  set(proxyserver_SRCS
    ${proxyserver_SRCS}
    proxyserver.rc
    ${sources_windows_Debugging}
  )
  if ( MSVC )
    set(proxyserver_SRCS
      ${proxyserver_SRCS}
      proxyserver.rc
    )
  endif()
endif()

include_directories(
  ${CMAKE_BINARY_DIR}
  ${CMAKE_SOURCE_DIR}/dep/g3dlite/include
  ${CMAKE_SOURCE_DIR}/dep/gsoap
  ${CMAKE_SOURCE_DIR}/dep/SFMT
  ${CMAKE_SOURCE_DIR}/dep/mersennetwister
  ${CMAKE_SOURCE_DIR}/src/server/collision
  ${CMAKE_SOURCE_DIR}/src/server/collision/Management
  ${CMAKE_SOURCE_DIR}/src/server/shared
  ${CMAKE_SOURCE_DIR}/src/server/shared/Configuration
  ${CMAKE_SOURCE_DIR}/src/server/shared/Cryptography
  ${CMAKE_SOURCE_DIR}/src/server/shared/Cryptography/Authentication
  ${CMAKE_SOURCE_DIR}/src/server/shared/Database
  ${CMAKE_SOURCE_DIR}/src/server/shared/DataStores
  ${CMAKE_SOURCE_DIR}/src/server/shared/Debugging
  ${CMAKE_SOURCE_DIR}/src/server/shared/Dynamic/CountedReference
  ${CMAKE_SOURCE_DIR}/src/server/shared/Dynamic/LinkedReference
  ${CMAKE_SOURCE_DIR}/src/server/shared/Dynamic
  ${CMAKE_SOURCE_DIR}/src/server/shared/Logging
  ${CMAKE_SOURCE_DIR}/src/server/shared/Networking
  ${CMAKE_SOURCE_DIR}/src/server/shared/Networking/Headers
  ${CMAKE_SOURCE_DIR}/src/server/shared/Networking/Sockets
  ${CMAKE_SOURCE_DIR}/src/server/shared/Networking/Strategies
  ${CMAKE_SOURCE_DIR}/src/server/shared/Packets
  ${CMAKE_SOURCE_DIR}/src/server/shared/PlayerHolder
  ${CMAKE_SOURCE_DIR}/src/server/shared/PlayerHolder/PlayerHolderDB
  ${CMAKE_SOURCE_DIR}/src/server/shared/PlayerHolder/PlayerHolderPacket
  ${CMAKE_SOURCE_DIR}/src/server/shared/Policies
  ${CMAKE_SOURCE_DIR}/src/server/shared/Threading
  ${CMAKE_SOURCE_DIR}/src/server/shared/Utilities
  ${CMAKE_SOURCE_DIR}/src/server/shared/Utilities/ProgressBar
  ${CMAKE_SOURCE_DIR}/src/server/shade
  ${CMAKE_SOURCE_DIR}/src/server/shade/Accounts
  ${CMAKE_SOURCE_DIR}/src/server/shade/Achievements
  ${CMAKE_SOURCE_DIR}/src/server/shade/Addons
  ${CMAKE_SOURCE_DIR}/src/server/shade/Battleground
  ${CMAKE_SOURCE_DIR}/src/server/shade/Cache
  ${CMAKE_SOURCE_DIR}/src/server/shade/Character
  ${CMAKE_SOURCE_DIR}/src/server/shade/Chat
  ${CMAKE_SOURCE_DIR}/src/server/shade/Chat/Commands
  ${CMAKE_SOURCE_DIR}/src/server/shade/Database
  ${CMAKE_SOURCE_DIR}/src/server/shade/DataStores
  ${CMAKE_SOURCE_DIR}/src/server/shade/Groups
  ${CMAKE_SOURCE_DIR}/src/server/shade/Handler
  ${CMAKE_SOURCE_DIR}/src/server/shade/Proxy
  ${CMAKE_SOURCE_DIR}/src/server/shade/Loot
  ${CMAKE_SOURCE_DIR}/src/server/shade/Maps
  ${CMAKE_SOURCE_DIR}/src/server/shade/Misc
  ${CMAKE_SOURCE_DIR}/src/server/shade/Objects
  ${CMAKE_SOURCE_DIR}/src/server/shade/Server
  ${CMAKE_SOURCE_DIR}/src/server/shade/Server/Client
  ${CMAKE_SOURCE_DIR}/src/server/shade/Server/Node
  ${CMAKE_SOURCE_DIR}/src/server/shade/Server/Control
  ${CMAKE_SOURCE_DIR}/src/server/shade/Server/Generics
  ${CMAKE_SOURCE_DIR}/src/server/shade/Warden
  ${CMAKE_SOURCE_DIR}/src/server/authserver/Server
  ${CMAKE_SOURCE_DIR}/src/server/authserver/Realms
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/CommandLine
##  ${CMAKE_CURRENT_SOURCE_DIR}/RemoteAccess
##  ${CMAKE_CURRENT_SOURCE_DIR}/TCSoap
  ${CMAKE_CURRENT_SOURCE_DIR}/ProxyThread
  ${ACE_INCLUDE_DIR}
  ${MYSQL_INCLUDE_DIR}
  ${OPENSSL_INCLUDE_DIR}
)

add_executable(proxyserver
  ${proxyserver_SRCS}
)

if( NOT WIN32 )
  set_target_properties(proxyserver PROPERTIES
    COMPILE_DEFINITIONS _TRINITY_CORE_CONFIG="${CONF_DIR}/proxyserver.conf"
  )
endif()

add_dependencies(proxyserver revision.h)

if( UNIX )
  if ( USE_GOLD )
    set(proxyserver_LINK_FLAGS "-B'/usr/local/bin' -pthread -lncurses -ldl ${proxyserver_LINK_FLAGS}")
  else()
    set(proxyserver_LINK_FLAGS "-pthread -lncurses ${proxyserver_LINK_FLAGS}")
  endif()
endif()

set_target_properties(proxyserver PROPERTIES LINK_FLAGS "${proxyserver_LINK_FLAGS}")

target_link_libraries(proxyserver
  shared
  collision
  g3dlib
  shade
  gsoap
  Detour
  ${JEMALLOC_LIBRARY}
  ${READLINE_LIBRARY}
  ${TERMCAP_LIBRARY}
  ${ACE_LIBRARY}
  ${MYSQL_LIBRARY}
  ${OPENSSL_LIBRARIES}
  ${OPENSSL_EXTRA_LIBRARIES}
  ${ZLIB_LIBRARIES}
  ${OSX_LIBS}
)

if( WIN32 )
  add_custom_command(TARGET proxyserver
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_CURRENT_SOURCE_DIR}/proxyserver.conf.dist ${CMAKE_BINARY_DIR}/bin/$(ConfigurationName)/
  )
endif()

if( UNIX )
  install(TARGETS proxyserver DESTINATION bin)
  install(FILES proxyserver.conf.dist DESTINATION etc)
elseif( WIN32 )
  install(TARGETS proxyserver DESTINATION "${CMAKE_INSTALL_PREFIX}")
  install(FILES proxyserver.conf.dist DESTINATION "${CMAKE_INSTALL_PREFIX}")
endif()

# Generate precompiled header
if( USE_COREPCH )
  if(CMAKE_COMPILER_IS_GNUCXX)
    add_precompiled_header(proxyserver ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders/proxyPCH.h)
  elseif(MSVC)
    add_native_precompiled_header(proxyserver ${CMAKE_CURRENT_SOURCE_DIR}/PrecompiledHeaders/proxyPCH)
  endif()
endif()

