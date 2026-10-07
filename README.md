# ⚡ LogStoreDB (LSM Storage Engine)

![C++](https://img.shields.io/badge/C++-17-blue)
![Platform](https://img.shields.io/badge/platform-linux%2Fwindows-lightgrey)
![Storage Engine](https://img.shields.io/badge/type-storage_engine-orange)
![LSM Tree](https://img.shields.io/badge/design-lsm_tree-green)
![License](https://img.shields.io/badge/license-MIT-yellow)

A crash-safe, Log-Structured Merge (LSM) Tree key-value storage engine built in modern C++. 

LogStoreDB implements core database internals including:
- Write-Ahead Logging (WAL)
- In-memory MemTable (powered by a custom SkipList)
- Immutable on-disk SSTables (Sorted String Tables) with Block Storage
- Block Indexing for efficient O(1) block lookups
- Background/Synchronous Compaction (L0 to L1)
- Deterministic crash recovery
- Tombstone-based deletes

Inspired by production storage systems like **LevelDB, RocksDB, Pebble, and Badger**.

---

# 📑 Table of Contents

- [Features](#-features)
- [Architecture Overview](#-architecture-overview)
- [Write Path](#-write-path-put)
- [Read Path](#-read-path-get)
- [Compaction](#-compaction)
- [Project Structure](#-project-structure)
- [Build & Test](#-build--test)
- [Learning Outcomes](#-learning-outcomes)
- [License](#-license)

---

# 🚀 Features

## 💾 Persistent LSM Storage
- **MemTable**: In-memory sorted structure (SkipList) for fast writes and reads.
- **SSTables**: Immutable disk files containing sorted data chunked into 4KB Data Blocks.
- **Index Blocks**: Each SSTable has an embedded index block (mapping `firstKey` to block offsets) to avoid full-file scans during `GET`.
- **Write-Ahead Log (WAL)**: Ensures crash-safe recovery for data in the MemTable.

## ⚡ Key-Value Operations
- PUT / GET / REMOVE support.
- Last-write-wins semantics.
- Tombstone-based logical deletes that correctly mask older values in disk levels.

## 🔄 Compaction
- Automatic merging of smaller SSTables into larger compacted tables.
- Purges tombstones and reclaims space.

---

# 🧠 Architecture Overview

LogStoreDB follows a classic LSM-Tree architecture:

```text
Client Request
      │
      ▼
+----------------+
|    KVStore     |
+----------------+
    │        │
    ▼        ▼
  WAL    MemTable (SkipList)
    │        │
    │        ▼
    │   +---------+
    │   | SSTable | (Level 0)
    │   +---------+
    │        │
    │        ▼
    │   Compaction
    │        │
    │        ▼
    │   +---------+
    │   | SSTable | (Level 1)
    │   +---------+
```

---

# ✍️ Write Path (PUT)

```text
PUT key value
      │
      ▼
1. Append binary record to WAL (disk)
2. Update MemTable (memory)
3. If MemTable > Limit:
   a. Flush to immutable SSTable (disk)
   b. Clear WAL and MemTable
4. If SSTable count >= 4:
   a. Trigger L0 -> L1 Compaction
```

# 🔍 Read Path (GET)

```text
GET key
      │
      ▼
1. Check MemTable. If found, return.
2. Check SSTables (newest to oldest):
   a. Load Index Block
   b. Find target Data Block using firstKey index
   c. Read Data Block and search for key
3. If Tombstone found, return NOT FOUND.
```

---

# 🛠️ Build & Test

LogStoreDB uses CMake and GoogleTest.

## Build

```bash
mkdir build
cd build
cmake ..
cmake --build .
```

## Run Tests

The test suite covers SkipList levels, MemTable iteration, WAL recovery, SSTable Blocks/Indexing, and full KVStore Compaction.

```bash
ctest --output-on-failure
```

---

# 📂 Project Structure

```text
LogStoreDB/
│
├── include/
│   ├── Block.h         # SSTable block builder/reader
│   ├── KVStore.h       # Main LSM engine orchestrator
│   ├── MemTable.h      # SkipList-backed MemTable
│   ├── SkipList.h      # Custom probabilistic SkipList
│   ├── SSTableReader.h # On-disk Index & Data block reader
│   ├── SSTableWriter.h # On-disk block flusher
│   └── WAL.h           # Write-Ahead Log
│
├── src/                # Implementation files
├── tests/              # GoogleTest test suites
│   ├── test_kvstore.cpp
│   ├── test_memtable.cpp
│   ├── test_skiplist.cpp
│   ├── test_sstable.cpp
│   └── test_wal.cpp
│
└── CMakeLists.txt
```

---

# 📚 Learning Outcomes

This project demonstrates deep understanding of systems engineering:
- **LSM-Tree Internals**: Bridging memory and disk for high-throughput writes.
- **Custom Data Structures**: Probabilistic SkipLists for sorted in-memory bounds.
- **Disk Layouts**: Chunking files into data blocks and utilizing index blocks.
- **Serialization**: Raw binary encoding of metadata, sizes, offsets, and strings.
- **System Testing**: GoogleTest integration for simulated crash recovery and compaction pipelines.

---

# 📄 License

MIT License