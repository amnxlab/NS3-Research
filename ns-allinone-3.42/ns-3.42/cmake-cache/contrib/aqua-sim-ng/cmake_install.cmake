# Install script for directory: /home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "default")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Install shared libraries without execute permission?
if(NOT DEFINED CMAKE_INSTALL_SO_NO_EXE)
  set(CMAKE_INSTALL_SO_NO_EXE "1")
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-aqua-sim-ng-default.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-aqua-sim-ng-default.so")
    file(RPATH_CHECK
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-aqua-sim-ng-default.so"
         RPATH "/usr/local/lib:$ORIGIN/:$ORIGIN/../lib:/usr/local/lib64:$ORIGIN/:$ORIGIN/../lib64")
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE SHARED_LIBRARY FILES "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/build/lib/libns3.42-aqua-sim-ng-default.so")
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-aqua-sim-ng-default.so" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-aqua-sim-ng-default.so")
    file(RPATH_CHANGE
         FILE "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-aqua-sim-ng-default.so"
         OLD_RPATH "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/build/lib::::::::::::::::::::::"
         NEW_RPATH "/usr/local/lib:$ORIGIN/:$ORIGIN/../lib:/usr/local/lib64:$ORIGIN/:$ORIGIN/../lib64")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/usr/bin/strip" "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/libns3.42-aqua-sim-ng-default.so")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/ns3" TYPE FILE FILES
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-application.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-address.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-pt-tag.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-channel.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-energy-model.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-hash-table.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-header.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-header-goal.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-header-mac.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mobility-pattern.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-modulation.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-net-device.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-node.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-noise-generator.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-phy.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-phy-cmn.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-propagation.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-range-propagation.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-simple-propagation.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-signal-cache.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-sinr-checker.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/helper/aqua-sim-helper.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac-broadcast.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac-fama.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac-aloha.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac-copemac.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac-goal.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac-sfama.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac-libra.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac-trumac.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac-jmac.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac-tdma.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mac-uwan.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-rmac.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-rmac-buffer.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-tmac.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing-static.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-header-routing.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing-dynamic.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing-flooding.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-datastructure.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing-buffer.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing-vbf.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing-dbr.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing-vbva.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mobility-kinematic.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-mobility-rwp.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-synchronization.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-localization.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing-ddos.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-attack-model.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-trace-reader.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-time-tag.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/ndn/named-data.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/ndn/named-data-header.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/ndn/name-discovery.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/ndn/pit.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/ndn/fib.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/ndn/content-storage.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/ndn/cs-fifo.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/ndn/cs-lru.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/ndn/cs-random.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/ndn/onoff-nd-application.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/helper/named-data-helper.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/helper/on-off-nd-helper.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/helper/aqua-sim-application-helper.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/helper/aqua-sim-traffic-gen-helper.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-traffic-gen.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing-dummy.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/aqua-sim-routing-ddbr.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/contrib/aqua-sim-ng/model/lib/svm.h"
    "/home/ahmed/NS3-Research/ns-allinone-3.42/ns-3.42/build/include/ns3/aqua-sim-ng-module.h"
    )
endif()

