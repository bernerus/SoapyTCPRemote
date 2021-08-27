######################################################################## 
# Build Soapy SDR support module for simple TCP remote RX pipe
######################################################################## 
cmake_minimum_required(VERSION 2.8.7)
project(SoapyTCPRemote CXX C)

find_package(SoapySDR "0.6.0" NO_MODULE)
if (NOT SoapySDR_FOUND)
    message(FATAL_ERROR "Soapy SDR development files not found...") 
endif ()
find_package(Threads REQUIRED)

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
target_link_libraries(TCPRemoteSupport
    Threads::Threads
)

add_executable(SoapyTCPServer
    SoapyTCPServer.cpp
)
target_link_libraries(SoapyTCPServer
    SoapySDR
    Threads::Threads
)

add_executable(SoapyTCPTest
    SoapyTCPTest.cpp
)
target_link_libraries(SoapyTCPTest
    SoapySDR
)

include(GNUInstallDirs)
install(TARGETS SoapyTCPServer DESTINATION ${CMAKE_INSTALL_BINDIR})
