cmake_minimum_required(3.11)
project(leveldb)

aux_source_directory(db DB)
aux_source_directory(include INCLUDE)
aux_source_directory(port PORT)
aux_source_directory(table TABLE)
aux_source_directory(util UTIL)

add_library(leveldb "")
target_sources(leveldb PUBLIC
        DB
        INCLUDE
        PORT
        TABLE
        UTIL
        )
target_link_libraries(leveldb include .)