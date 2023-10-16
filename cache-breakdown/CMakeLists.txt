# 配置源文件的搜索路径
aux_source_directory(${PROJECT_SOURCE_DIR}/src SRC_LIST)
aux_source_directory(${PROJECT_SOURCE_DIR}/cache-breakdown CACHE_BREAKDOWN)

# 配置最终的可执行文件输出的路径
SET (EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# 设置可执行文件
add_executable(cache-breakdown ${SRC_LIST} ${CACHE_BREAKDOWN})

# 执行可执行文件需要依赖的库
target_link_libraries(cache-breakdown mysqlclient hiredis pthread)

