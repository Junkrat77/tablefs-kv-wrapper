cmake_minimum_required(VERSION 3.11)
project(tablefs)

set(CMAKE_CXX_STANDARD 17)

find_library(FUSE fuse)

add_subdirectory(lib/leveldb)

add_definitions(-D_FILE_OFFSET_BITS=64 )
add_definitions(-DLEVELDB_PLATFORM_POSIX)

option(BUILD_BENCH "build tablefs bench" OFF)
option(BUILD_TEST "build fs_test" ON)

add_library(tablefs_lib "")
target_sources(tablefs_lib PRIVATE
        "util/allocator.cpp"
        "util/allocator.h"
        "util/command.cpp"
        "util/command.h"
        "util/tfs_logging.cpp"
        "util/tfs_logging.h"
        "util/myhash.cpp"
        "util/myhash.h"
        "util/properties.cpp"
        "util/properties.h"

        "adaptor/kv_wrapper.cc"
        "adaptor/kv_wrapper.h"
        "adaptor/statistics.cc"
        "adaptor/statistics.h"
        "adaptor/leveldb_wrapper.cc"
        "adaptor/leveldb_wrapper.h"

        "fs/dcache.cpp"
        "fs/dcache.h"
        "fs/fswrapper.cpp"
        "fs/fswrapper.h"
        "fs/icache.cpp"
        "fs/icache.h"
        "fs/inodemutex.cpp"
        "fs/inodemutex.h"
        "fs/tablefs.cpp"
        "fs/tablefs.h"
        "fs/tfs_inode.h"
        "fs/tfs_state.cpp"
        "fs/tfs_state.h"
        )

target_include_directories(tablefs_lib PUBLIC include lib/leveldb lib/leveldb/include lib/leveldb/port .)

target_link_libraries(tablefs_lib fuse leveldb)

add_executable(tablefs "tablefs_main.cpp")
target_link_libraries(tablefs tablefs_lib)

if (BUILD_BENCH)
    add_executable(tablefs_bench
            "util/monitor.cpp"
            "util/monitor.h"
            "util/socket.cpp"
            "util/socket.h"
            "util/traceloader.cpp"
            "util/traceloader.h"
            "fsbench.cpp"
            )
    target_link_libraries(tablefs_bench tablefs_lib)
endif ()

if (BUILD_TEST)
    add_executable(tablefs_test
            "fswrapper_test.cpp"
            )
    target_link_libraries(tablefs_test tablefs_lib)
endif ()




