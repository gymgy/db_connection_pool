cmake_minimum_required(VERSION 3.0)

set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} -g)

aux_source_directory(. SRC_LIST)

link_directories(/usr/lib64/mysql)

set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

add_executable(ConnectionPool ${SRC_LIST})
target_link_libraries(ConnectionPool mysqlclient pthread)