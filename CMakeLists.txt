cmake_minimum_required(VERSION 3.14)
project(LogStoreDB)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(KVS_core
    src/KVStore.cpp
    src/WAL.cpp
    src/MemTable.cpp
    src/SkipList.cpp
    src/SSTableWriter.cpp
    src/SSTableReader.cpp
)
target_include_directories(KVS_core PUBLIC include)

add_executable(kvstore cli/main.cpp)
target_link_libraries(kvstore PRIVATE KVS_core)

# Testing
enable_testing()

include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(test_skiplist tests/test_skiplist.cpp)
target_link_libraries(test_skiplist PRIVATE KVS_core gtest_main)

add_executable(test_memtable tests/test_memtable.cpp)
target_link_libraries(test_memtable PRIVATE KVS_core gtest_main)

add_executable(test_wal tests/test_wal.cpp)
target_link_libraries(test_wal PRIVATE KVS_core gtest_main)

add_executable(test_kvstore tests/test_kvstore.cpp)
target_link_libraries(test_kvstore PRIVATE KVS_core gtest_main)

add_executable(test_sstable tests/test_sstable.cpp)
target_link_libraries(test_sstable PRIVATE KVS_core gtest_main)

include(GoogleTest)
gtest_discover_tests(test_skiplist)
gtest_discover_tests(test_memtable)
gtest_discover_tests(test_wal)
gtest_discover_tests(test_kvstore)
gtest_discover_tests(test_sstable)
