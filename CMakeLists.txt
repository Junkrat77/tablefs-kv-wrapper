cmake_minimum_required(3.11)
project(tablefs_petakv)

set(CMAKE_CXX_STANDARD 17)

find_library(FUSE fuse)

add_library(tablefs_lib "")

aux_source_directory(adaptor ADAPTOR)
aux_source_directory(fs FS)
aux_source_directory(util UTIL)

message(${ADAPTOR})
message(${FS})
message(${UTIL})

add_executable(lib/leveldb)

target_include_directories(tablefs include .)

target_sources(tablefs_lib PRIVATE
        ADAPTOR
        FS
        UTIL
        )

target_link_libraries(tablefs_lib FUSE)

add_executable(tablefs "tablefs_main.cpp")
target_link_libraries(tablefs tablefs_lib)
