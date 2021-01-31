######################################################################## 
# Build Soapy SDR support module for simple TCP remote RX pipe
######################################################################## 
cmake_minimum_required(VERSION 2.8.7)
project(SoapyFCDPP CXX C)

find_package(SoapySDR "0.6.0" NO_MODULE)
if (NOT SoapySDR_FOUND)
    message(FATAL_ERROR "Soapy SDR development files not found...") 
endif ()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})

if(CMAKE_COMPILER_IS_GNUCXX)

    #disable warnings for unused parameters
    add_definitions(-Wno-unused-parameter)

endif(CMAKE_COMPILER_IS_GNUCXX)

SOAPY_SDR_MODULE_UTIL(
    TARGET TCPRemoteSupport
    SOURCES
    SoapyTCPRemote.cpp
)

add_executable(SoapyTCPServer
    SoapyTCPServer.cpp
)
include(GNUInstallDirs)
install(TARGETS SoapyTCPServer DESTINATION ${CMAKE_INSTALL_BINDIR})
