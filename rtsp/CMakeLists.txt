cmake_minimum_required(VERSION 3.10)
project(rtsp)

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED  ON)


include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_SOURCE_DIR}/staging_dir/include)
include_directories(${CMAKE_SOURCE_DIR}/vendor)
#SET( SRCS protocol/rtp/rtcpPacket.cpp )

aux_source_directory(src SRCS)


set(CONFIG_PLAYBACK_SUPPORT N)
if(CONFIG_PLAYBACK_SUPPORT)
    add_definitions( -DAPPWEB_PLAYBACK)
endif()

# 支持公安一所GB28181 协议
if (CONFIG_GB28181_SUPPORT)
    add_definitions(-DSUPPORT_GB28181)
endif()

#是否支持ONVIF协议
if(CONFIG_ONVIF_SUPPORT)
    add_definitions( -DONVIF_ITC)
endif()

#rtcp功能支持
set(CONFIG_RTCP_SUPPORT yes)
if(CONFIG_RTCP_SUPPORT)
    add_definitions( -DRTP_RTCP)
endif()

add_library(${PROJECT_NAME} STATIC ${SRCS} ${GB28181_SRCS} ${PLAYBACK_SRCS} )

#add_custom_command( TARGET ${PROJECT_NAME} POST_BUILD
#        COMMAND ${CMAKE_COMMAND} -E copy
#        ${CMAKE_CURRENT_SOURCE_DIR}/include/server.h
#        ${CMAKE_CURRENT_SOURCE_DIR}/include/mediainfo.h
#        ${CMAKE_SOURCE_DIR}/standir_dir/include/
#)
