cmake_minimum_required(VERSION 3.14)
project(tablefs)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED fuse)
pkg_check_modules(LIBPMEM REQUIRED libpmem)

add_subdirectory(lib/leveldb)

add_definitions(-D_FILE_OFFSET_BITS=64 )
add_definitions(-DLEVELDB_PLATFORM_POSIX)

option(BUILD_BENCH "build tablefs bench" OFF)
option(BUILD_TEST "build fs_test" ON)

option(WITH_UTREE "build tablefs with utree" ON)
option(WITH_ROCKSDB "build tablefs with rocksdb" ON)
option(WITH_METAKV "build tablefs with metakv" ON)

link_libraries(${LIBPMEM_LIBRARIES})
include_directories(${LIBPMEM_INCLUDE_DIRS})

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
        "adaptor/utree_wrapper.cc"
        "adaptor/utree_wrapper.h"
        "adaptor/rocksdb_wrapper.cc"
        "adaptor/rocksdb_wrapper.h"
        "adaptor/metakv_wrapper.cc"
        "adaptor/metakv_wrapper.h"

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

target_include_directories(tablefs_lib PUBLIC ${FUSE_INCLUDE_DIRS} include lib/leveldb lib/leveldb/include lib/leveldb/port .)
if (WITH_UTREE)
    add_subdirectory(kv/utree/singleThread/uTree)
    target_include_directories(tablefs_lib PUBLIC kv/utree/singleThread/uTree)
    target_link_libraries(tablefs_lib utree)
endif ()

if (WITH_ROCKSDB)
    add_definitions(-DWITH_DCPMM)
    add_definitions(-DON_DCPMM)
    add_subdirectory(kv/pmem-rocksdb)
    target_include_directories(tablefs_lib PUBLIC kv/pmem-rocksdb/include)
    target_link_libraries(tablefs_lib rocksdb)
endif ()

if (WITH_METAKV)
    find_library(CLHT_LIBRARIES NAMES clht PATHS kv/metakv/src/CLHT REQUIRED)
    find_library(SSMEM_LIBRARIES NAMES ssmem PATHS kv/metakv/src/CLHT REQUIRED)
    add_subdirectory(kv/metakv)
    target_include_directories(tablefs_lib PUBLIC kv/metakv)
    target_link_libraries(tablefs_lib metakv)
endif ()

target_link_libraries(tablefs_lib leveldb ${FUSE_LIBRARIES})

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
    add_executable(utree_wrapper_test "")
    target_sources(utree_wrapper_test PUBLIC
            "adaptor/utree_wrapper_test.cc"
            "adaptor/utree_wrapper.cc"
            "adaptor/utree_wrapper.h"
            "adaptor/rocksdb_wrapper.cc"
            "adaptor/rocksdb_wrapper.h"
            "adaptor/kv_wrapper.cc"
            "adaptor/kv_wrapper.h"
            "adaptor/statistics.cc"
            "adaptor/statistics.h"
            "util/properties.cpp"
            "util/properties.h"
            )
    target_include_directories(utree_wrapper_test PUBLIC kv/utree/singleThread/uTree lib/leveldb lib/leveldb/include include .)
    target_link_libraries(utree_wrapper_test utree leveldb ${LIBPMEM_LIBRARIES})

    add_executable(tablefs_test
            "fswrapper_test.cpp"
            )
    target_link_libraries(tablefs_test tablefs_lib)
endif ()




