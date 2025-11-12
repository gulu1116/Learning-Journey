cmake_minimum_required(VERSION 2.6)

project (jsoncpp)

ADD_DEFINITIONS(-O1 -g -W -Wall -D_REENTRANT -D_FILE_OFFSET_BITS=64 -DAC_HAS_INFO
-DAC_HAS_WARNING -DAC_HAS_ERROR -DAC_HAS_CRITICAL -DTIXML_USE_STL
-DAC_HAS_DEBUG -DLINUX_DAEMON -std=c++11)

# 添加头文件目录
#INCLUDE_DIRECTORIES("/usr/local/include/jsoncpp")
INCLUDE_DIRECTORIES("include")
# 添加库文件目录
#LINK_DIRECTORIES("/usr/local/lib/")

set(jsoncpp_sources
                lib_json/json_tool.h
                lib_json/json_reader.cpp
                lib_json/json_valueiterator.inl
                lib_json/json_value.cpp
                lib_json/json_writer.cpp)

# 执行文件
ADD_EXECUTABLE(jsoncpp_speed  jsoncpp_speed.cpp ${jsoncpp_sources})

# 链接库
#TARGET_LINK_LIBRARIES(jsoncpp_speed jsoncpp)
#TARGET_LINK_LIBRARIES(jsoncpp_speed)






