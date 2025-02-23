cmake_minimum_required(VERSION 3.11)
project(EasyDarwin )
 
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD 11)

MESSAGE("Identifying the OS...")
if(WIN32)
  MESSAGE(WARNING "This is Windows.")
elseif(APPLE)
  MESSAGE(WARNING "This is MacOS.")
elseif(UNIX)
  MESSAGE(WARNING "This is Linux.")
endif()


set(CMAKE_VERBOSE_MAKEFILE ON )
set(CMAKE_AUTOGEN_VERBOSE  ON)

#include(CMakeConfig.txt)


#set(TOOP_DIR ${CMAKE_SOURCE_DIR} CACHE PATH "top cmake path" FORCE)

# ============================= linux静态.so库输出路径【默认路径在-B build目录下】 =============================
#linux .so输出路径
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/staging_dir/lib")
 
# ============================= 执行程序输出路径 =============================
#执行程序、dll动态库、pdb调试文件
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/staging_dir/bin")
 
# ============================= 归档输出路径 =============================
# 静态库 .lib 动态库.lib地址文件 linux静态库.a 静态库pdb调试文件

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/staging_dir/lib")
 
set(BUILD_SHARED_LIBS ON) #动态库

#add_subdirectory(vendor)
#include(vendor/CMakeLists.txt)

# 定义第三方库的信息


# 包含ExternalProject模块
include(ExternalProject)
# 添加第三方项目
ExternalProject_Add(
    libnet
#    GIT_REPOSITORY "https://github.com/zntx/libnet.git"  # 替换为实际仓库或URL
#    GIT_TAG "result"         # 指定分支或标签
	SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/libnet          # 源码路径
    CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_LIST_DIR}/staging_dir/         # 指定安装路径
        -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}                   # 传递构建类型
    BUILD_ALWAYS OFF                                             # 仅在需要时重新构建
)

ExternalProject_Add(
    fmt
    #    GIT_REPOSITORY https://github.com/fmtlib/fmt.git           # 替换为实际仓库或URL
    #    GIT_TAG main                                               # 指定分支或标签
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/fmt                       # 源码路径
    CMAKE_ARGS
     -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_LIST_DIR}/staging_dir/  # 指定安装路径
     -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}                         # 传递构建类型
    BUILD_ALWAYS OFF                                                # 仅在需要时重新构建
)


ExternalProject_Add(
    spdlog
    #GIT_REPOSITORY https://github.com/gabime/spdlog.git           # 替换为实际仓库或URL
    #GIT_TAG main                                               # 指定分支或标签
    SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor/spdlog                       # 源码路径
    CMAKE_ARGS
    -DCMAKE_INSTALL_PREFIX=${CMAKE_CURRENT_LIST_DIR}/staging_dir/  # 指定安装路径
    -DCMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE}                         # 传递构建类型
    BUILD_ALWAYS OFF                                                # 仅在需要时重新构建
)

add_subdirectory(rtsp )
