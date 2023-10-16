cmake_minimum_required (VERSION 2.8)

add_compile_options(-std=c++11)

# 定义当前工程名字
project (RedisCacheTest)

# 配置编译选项
set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

# 配置头文件的搜索路径
include_directories(${PROJECT_SOURCE_DIR}/include)

add_subdirectory(cache-penetration)
add_subdirectory(cache-breakdown)
add_subdirectory(cache-avalanche)
add_subdirectory(data-consistency)