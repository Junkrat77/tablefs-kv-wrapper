cmake_minimum_required(VERSION 3.14)
project(tablefs)

set(CMAKE_CXX_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(FUSE REQUIRED fuse)
pkg_check_modules(LIBPMEM REQUIRED libpmem)
pkg_check_modules(LIBPMEMOBJ REQUIRED libpmemobj)

add_subdirectory(lib/leveldb)

add_definitions(-D_FILE_OFFSET_BITS=64 )
add_definitions(-DLEVELDB_PLATFORM_POSIX)

option(BUILD_BENCH "build tablefs bench" OFF)
option(BUILD_TEST "build fs_test" ON)

option(WITH_UTREE "build tablefs with utree" OFF)
option(WITH_ROCKSDB "build tablefs with rocksdb" OFF)
option(WITH_METAKV "build tablefs with metakv" ON)
option(WITH_HIKV "build tablefs with hikv" OFF)
option(WITH_ROART "build tablefs with roart" OFF)
option(WITH_TLHASH "build tablefs with two level hash" OFF)

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
target_link_libraries(tablefs_lib leveldb ${FUSE_LIBRARIES})

if (WITH_UTREE)
    message("Define uTree")
    add_definitions(-DUTREE)
    add_definitions(-DUSE_PMDK)
    target_sources(tablefs_lib PUBLIC
            "adaptor/utree_wrapper.cc"
            "adaptor/utree_wrapper.h"
            )
    add_subdirectory(kv/utreedb)
    target_include_directories(tablefs_lib PUBLIC kv/utreedb/multiThread $LIBPMEMOBJ_INCLUDE_DIRS})
    target_link_libraries(tablefs_lib treedb ${LIBPMEMOBJ_LIBRARIES})
endif ()

if (WITH_ROCKSDB)
    message("Define RocksDB")
    add_definitions(-DROCKSDB)
    target_sources(tablefs_lib PUBLIC
            "adaptor/rocksdb_wrapper.cc"
            "adaptor/rocksdb_wrapper.h"
            )
    add_definitions(-DWITH_DCPMM)
    add_definitions(-DON_DCPMM)
    add_subdirectory(kv/pmem-rocksdb)
    target_include_directories(tablefs_lib PUBLIC kv/pmem-rocksdb/include)
    target_link_libraries(tablefs_lib rocksdb)
endif ()

if (WITH_HIKV)
    message("Define HiKV")
    add_definitions(-DHIKV)
    target_sources(tablefs_lib PUBLIC
            "adaptor/hikv_adaptor.cc"
            "adaptor/hikv_adaptor.h"
            )
    add_subdirectory(kv/hikv)
    target_include_directories(tablefs_lib PUBLIC kv/hikv)
    target_link_libraries(tablefs_lib open_hikv)
endif ()

if (WITH_METAKV)
    message("Define MetaKV")
    add_definitions(-DMETAKV)
    target_sources(tablefs_lib PUBLIC
            "adaptor/metakv_wrapper.cc"
            "adaptor/metakv_wrapper.h"
            )
#     find_library(CLHT_LIBRARIES NAMES clht PATHS kv/metakv/src/CLHT REQUIRED)
#     find_library(SSMEM_LIBRARIES NAMES ssmem PATHS kv/metakv/src/CLHT REQUIRED)
    add_subdirectory(kv/metakv)
    target_include_directories(tablefs_lib PUBLIC kv/metakv)
    target_link_libraries(tablefs_lib metakv)
endif ()

if (WITH_ROART)
    message("Define ROART")
    add_definitions(-DROART)
    add_definitions(-DCLWB)
    add_definitions(-DVARIABLE_LENGTH)
    add_definitions(-DUSE_PMDK)
    add_definitions(-DACMA) # for fastfair and skiplist with dcmm
    add_definitions(-DFF_GC) # ff_gc
    add_definitions(-DRECLAIM_MEMORY)
    add_definitions(-DKEY_INLINE)
    add_definitions(-DINSTANT_RESTART)

    add_definitions(-DLEAF_ARRAY)
    add_definitions(-DFIND_FIRST)
    add_definitions(-DTRANSACTIONAL)
    target_sources(tablefs_lib PUBLIC
            "adaptor/roart_wrapper.cc"
            "adaptor/roart_wrapper.h"
            )
    add_subdirectory(kv/roart)
    target_include_directories(tablefs_lib PUBLIC kv/roart/ART kv/roart/nvm_mgr kv/roart/benchmark)
    target_link_libraries(tablefs_lib Indexes)
endif ()

if (WITH_TLHASH)
    message("Define Two Level Hash")
    add_definitions(-DTLHASH)
    target_sources(tablefs_lib PUBLIC
            "adaptor/tlhash_wrapper.cc"
            "adaptor/tlhash_wrapper.h"
            "kv/tlhash/src/CCEH.cpp"
            "kv/tlhash/src/global_log.cpp"
            "kv/tlhash/src/hash.cpp"
            "kv/tlhash/src/HybridHash.cpp"
            )
    link_libraries(${LIBPMEMOBJ_LIBRARIES})
    include_directories(${LIBPMEMOBJ_INCLUDE_DIRS})
    #find_library(TLHASH_LIBRARIES NAMES metadb PATHS kv/tlhash/ REQUIRED)
    find_library(CLHT_LIBRARIES NAMES clht PATHS kv/tlhash/lib REQUIRED)
    find_library(SSMEM_LIBRARIES NAMES ssmem PATHS kv/tlhash/lib REQUIRED)
    target_include_directories(tablefs_lib PUBLIC kv/tlhash/include kv/tlhash/src)
    message("Find two level hash lib ${TLHASH_LIBRARIES}")
    target_link_libraries(tablefs_lib
            #${TLHASH_LIBRARIES}
            ${CLHT_LIBRARIES} ${SSMEM_LIBRARIES})
endif ()


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
    add_definitions(-DUSING_VALID)
    add_executable(metakv_wrapper_test "")
    target_sources(metakv_wrapper_test PUBLIC
            "adaptor/metakv_wrapper_test.cc"
            "adaptor/metakv_wrapper.cc"
            "adaptor/metakv_wrapper.h"

            "adaptor/kv_wrapper.cc"
            "adaptor/kv_wrapper.h"
            "adaptor/statistics.cc"
            "adaptor/statistics.h"
            "util/properties.cpp"
            "util/properties.h"
            )
    target_include_directories(metakv_wrapper_test PUBLIC kv/metakv lib/leveldb lib/leveldb/include include .)
    target_link_libraries(metakv_wrapper_test metakv leveldb ${LIBPMEM_LIBRARIES})

    add_executable(tablefs_test
            "fswrapper_test.cpp"
            )
    target_link_libraries(tablefs_test tablefs_lib)
endif ()

# if (BUILD_TEST)
#     add_executable(utree_wrapper_test "")
#     target_sources(utree_wrapper_test PUBLIC
#             "adaptor/utree_wrapper_test.cc"
#             "adaptor/utree_wrapper.cc"
#             "adaptor/utree_wrapper.h"
#             "adaptor/kv_wrapper.cc"
#             "adaptor/kv_wrapper.h"
#             "adaptor/statistics.cc"
#             "adaptor/statistics.h"
#             "util/properties.cpp"
#             "util/properties.h"
#             )
#     target_include_directories(utree_wrapper_test PUBLIC kv/utree/singleThread/uTree lib/leveldb lib/leveldb/include include .)
#     target_link_libraries(utree_wrapper_test utree leveldb ${LIBPMEM_LIBRARIES})

#     add_executable(tablefs_test
#             "fswrapper_test.cpp"
#             )
#     target_link_libraries(tablefs_test tablefs_lib)
# endif ()




