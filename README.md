# ⚡ LogStoreDB — LSM-Style Key-Value Storage Engine

![C++](https://img.shields.io/badge/C%2B%2B-17-blue)
![Platform](https://img.shields.io/badge/platform-Windows%2FLinux-lightgrey)
![Storage Engine](https://img.shields.io/badge/type-storage__engine-orange)
![LSM Tree](https://img.shields.io/badge/design-LSM--style-green)
![Tests](https://img.shields.io/badge/tests-24%20passing-success)
![License](https://img.shields.io/badge/license-MIT-yellow)

A lightweight, crash-recoverable **Log-Structured Merge (LSM)-style key-value storage engine** built from scratch in modern C++.

LogStoreDB implements core storage-engine internals including:

- Write-Ahead Logging (WAL) with CRC32 integrity checking
- In-memory MemTable backed by a custom SkipList
- Explicit sequence numbers for deterministic version ordering
- Immutable SSTables with block-based storage
- SSTable index and footer metadata
- Tombstone-based logical deletes
- Streaming K-way merge compaction
- Atomic SSTable installation using temporary files and rename
- Defensive corruption and bounds validation
- O(1) LRU block caching for repeated reads
- Crash recovery through WAL replay and persistent sequence recovery
- Automated correctness, corruption, restart, and stress testing

Inspired by storage-engine concepts used in systems such as **LevelDB, RocksDB, Pebble, and Badger**.

---

# 📑 Table of Contents

- [Features](#-features)
- [Architecture Overview](#-architecture-overview)
- [Write Path](#-write-path-put)
- [Read Path](#-read-path-get)
- [WAL & Crash Recovery](#-wal--crash-recovery)
- [Sequence Numbers](#-sequence-numbers)
- [MemTable & SkipList](#-memtable--skiplist)
- [SSTable Storage](#-sstable-storage)
- [Tombstones](#-tombstones)
- [Compaction](#-compaction)
- [LRU Block Cache](#-lru-block-cache)
- [Corruption Handling](#-corruption-handling)
- [Benchmarks](#-benchmarks)
- [Testing](#-testing)
- [Project Structure](#-project-structure)
- [Build & Test](#-build--test)
- [Design Decisions](#-design-decisions)
- [Learning Outcomes](#-learning-outcomes)
- [Limitations](#-limitations)
- [License](#-license)

---

# 🚀 Features

## 💾 Persistent LSM Storage

- **MemTable**: Ordered in-memory storage backed by a custom SkipList.
- **SSTables**: Immutable on-disk tables containing sorted records organized into approximately 4 KB data blocks.
- **Index Blocks**: Map SSTable key ranges to data-block locations so reads can avoid scanning the entire file.
- **Footer Metadata**: Stores persistent SSTable metadata including the maximum sequence number.
- **Write-Ahead Log**: Records mutations before they are reflected in memory, enabling recovery after restart.
- **CRC32**: Detects corrupted WAL records during recovery.

## 🔢 Explicit Versioning

Each mutation receives a monotonically increasing `uint64_t` sequence number.

```text
PUT A = "old"    seq=10
PUT A = "new"    seq=20

latest version:
A = "new"
```

Version selection is based on the sequence number rather than filesystem ordering or filename ordering.

## 🗑️ Tombstone-Based Deletes

Deletes are represented explicitly using:

```cpp
std::optional<std::string>
```

A normal record contains a value:

```text
A → "hello"
```

A deletion is represented as:

```text
A → nullopt
```

This prevents collisions with user-provided values such as:

```text
"[[TOMBSTONE]]"
```

## 🔄 Streaming Compaction

Multiple sorted SSTables are merged using a streaming K-way merge.

- Avoids loading all SSTable records into a large in-memory map.
- Resolves overlapping keys using sequence numbers.
- Keeps compaction memory proportional to the number of input streams.
- Produces a new compacted SSTable through atomic file installation.

## ⚡ LRU Block Cache

Frequently accessed SSTable blocks can be served from an in-memory LRU cache.

- `std::unordered_map` + `std::list`
- Average `O(1)` lookup
- Bounded memory usage
- Caches parsed block contents
- Cache hits avoid repeated disk reads and block parsing

---

# 🧠 Architecture Overview

LogStoreDB follows an LSM-style architecture:

```text
                    Client / CLI
                         │
                         ▼
                 ┌───────────────┐
                 │    KVStore    │
                 └───────┬───────┘
                         │
              ┌──────────┴──────────┐
              │                     │
              ▼                     ▼
       ┌─────────────┐       ┌─────────────┐
       │     WAL     │       │  MemTable   │
       │    CRC32    │       │  SkipList   │
       └─────────────┘       └──────┬──────┘
                                    │
                              Flush threshold
                                    │
                                    ▼
                           ┌─────────────────┐
                           │    SSTable      │
                           │                 │
                           │  Data Blocks    │
                           │  Index Block    │
                           │  Footer         │
                           └────────┬────────┘
                                    │
                           ┌────────▼────────┐
                           │  LRU Block      │
                           │     Cache       │
                           └────────┬────────┘
                                    │
                                    ▼
                           ┌─────────────────┐
                           │   Compaction    │
                           │   K-Way Merge   │
                           └─────────────────┘
```

The main responsibilities are divided into four layers:

```text
Durability       → WAL + CRC32
In-memory state  → MemTable + SkipList
Persistent state → SSTables
Maintenance      → Compaction + Block Cache
```

---

# ✍️ Write Path (PUT)

```text
PUT key value
      │
      ▼
1. Allocate sequence number
      │
      ▼
2. Append binary record to WAL
      │
      ▼
3. Update MemTable
      │
      ▼
4. Continue serving requests
      │
      ▼
5. MemTable reaches flush threshold
      │
      ├── Build sorted SSTable
      ├── Write to temporary .tmp file
      ├── Complete and close file
      └── Atomically rename to .sst
```

The WAL provides a recovery record while the MemTable provides fast in-memory access.

---

# 🔍 Read Path (GET)

```text
GET key
   │
   ▼
1. Check MemTable
   │
   ├── Found newest visible version
   │
   └── Otherwise continue
            │
            ▼
2. Search SSTables
            │
            ▼
3. Locate candidate block using SSTable index
            │
            ▼
4. Check LRU Block Cache
        ┌───┴───┐
        │       │
       HIT     MISS
        │       │
        │       ▼
        │   Read block
        │       │
        │       ▼
        │   Validate + parse
        │       │
        │       ▼
        │   Insert into cache
        │
        └───┬───┘
            ▼
5. Resolve matching KVPair by sequence number
            │
            ▼
6. Newest sequence wins
            │
            ▼
7. Tombstone → NOT FOUND
```

The logical result does not depend on whether a block was served from the cache or read directly from disk.

---

# 📝 DELETE Path

Deletes are represented using explicit tombstones.

```text
REMOVE key
     │
     ▼
Allocate sequence number
     │
     ▼
Append DELETE record to WAL
     │
     ▼
Insert tombstone into MemTable
     │
     ▼
Flush / Compact
```

Example:

```text
Older SSTable:
seq=10 → user = Alice

Newer record:
seq=20 → user = DELETE
```

The delete masks the older version.

---

# 💿 WAL & Crash Recovery

The WAL is an append-only binary log containing mutation records and checksums.

Conceptually:

```text
WAL Record
┌────────────────────────┐
│ Sequence Number        │
│ Operation              │
│ Key Size               │
│ Value Size             │
│ Key                    │
│ Value                  │
│ CRC32                  │
└────────────────────────┘
```

During recovery:

```text
Database Restart
       │
       ├───────────────┐
       │               │
       ▼               ▼
Load SSTables       Replay WAL
       │               │
       └───────┬───────┘
               ▼
       Reconstruct state
               │
               ▼
       Restore sequence state
```

Each WAL record is validated before replay:

```text
Read record
    │
    ▼
Calculate CRC32
    │
    ▼
Compare checksum
    │
    ├── Valid ───────► Replay
    │
    └── Invalid ─────► Reject corrupted record
```

This protects recovery from malformed or corrupted WAL records.

---

# 🔢 Sequence Numbers

Every mutation receives an explicit monotonically increasing sequence number.

Example:

```text
seq=1  PUT     user = Alice
seq=2  PUT     age  = 20
seq=3  PUT     user = Bob
seq=4  DELETE  age
seq=5  PUT     user = Charlie
```

For a key with multiple physical versions:

```text
seq=1 → Alice
seq=3 → Bob
seq=5 → Charlie
```

the newest version is:

```text
seq=5 → Charlie
```

Sequence numbers are preserved through:

```text
WAL
 ↓
MemTable
 ↓
SSTable
 ↓
Compaction
 ↓
Read conflict resolution
```

SSTable footer metadata also preserves the maximum sequence number represented by the table, allowing the engine to recover the correct sequence counter even after the WAL has been flushed.

Example:

```text
SSTable A → maxSeq = 100
SSTable B → maxSeq = 147
SSTable C → maxSeq = 132

Recovered maxSeq = 147
Next sequence   = 148
```

---

# 🧠 MemTable & SkipList

The MemTable stores recent mutations in memory.

It is backed by a custom SkipList that maintains keys in sorted order.

```text
Level 3:        ────────────────►
Level 2:    ──────────► ────────►
Level 1:    ───► ───► ───► ────►
Level 0:    A  →  B  →  C  →  D  →  E
```

Level 0 contains all records in sorted order.

Higher levels act as express lanes, giving the SkipList expected logarithmic search and insertion behavior.

Sorted MemTable contents can then be written directly into sorted SSTable blocks.

---

# 📦 SSTable Storage

When the MemTable reaches its configured threshold, its sorted contents are persisted as an immutable SSTable.

```text
MemTable
   │
   ▼
Sorted KVPair records
   │
   ▼
BlockBuilder
   │
   ├── Data Block 1
   ├── Data Block 2
   ├── Data Block 3
   └── ...
   │
   ▼
Index Block
   │
   ▼
Footer
   │
   ▼
SSTable
```

A logical record contains:

```text
Sequence Number
Key
Value / Tombstone
```

The approximately 4 KB block structure keeps individual reads bounded while allowing the SSTable reader to avoid processing unrelated data.

---

# 📚 SSTable Index

Each SSTable maintains index information describing where data blocks are stored.

Conceptually:

```text
firstKey       offset       size
----------------------------------
apple          0            4096
banana         4096         4020
carrot         8116         3975
```

A lookup can therefore narrow the search to the relevant block:

```text
Key
 │
 ▼
Index
 │
 ▼
Candidate Block
 │
 ▼
Block Reader
 │
 ▼
KVPair
```

This avoids scanning the entire SSTable for every lookup.

---

# 🧱 Atomic SSTable Installation

Flush and compaction outputs are first written to temporary files:

```text
Build SSTable
     │
     ▼
sst_x.sst.tmp
     │
     ▼
Complete write
     │
     ▼
Close file
     │
     ▼
Atomic rename
     │
     ▼
sst_x.sst
```

This prevents an incomplete write from being installed as a normal SSTable.

Temporary files represent unfinished work, while `.sst` files represent completed table outputs.

---

# 🗑️ Tombstones

A deletion is not represented using a special user-visible string.

Instead:

```cpp
std::optional<std::string>
```

is used to distinguish between:

```text
Actual value:
"A" → "hello"

Tombstone:
"A" → nullopt
```

This means a user can safely store:

```text
[[TOMBSTONE]]
```

as an ordinary value.

For example:

```text
PUT message "[[TOMBSTONE]]"

GET message
→ [[TOMBSTONE]]
```

while:

```text
REMOVE message

GET message
→ NOT FOUND
```

Tombstones also prevent older SSTable versions from becoming visible again.

---

# 🔄 Compaction

As SSTables accumulate, the engine performs compaction.

```text
SSTable 1 ── Iterator ──┐
SSTable 2 ── Iterator ──┤
SSTable 3 ── Iterator ──┼──► K-Way Merge
SSTable 4 ── Iterator ──┤
SSTable N ── Iterator ──┘
                             │
                             ▼
                     Sequence Resolution
                             │
                             ▼
                     New SSTable
```

Each input SSTable is traversed incrementally using an iterator rather than being fully loaded into memory.

For the same key:

```text
SSTable A:
seq=10 → A = old

SSTable B:
seq=25 → A = new

SSTable C:
seq=18 → A = older
```

the compactor retains:

```text
seq=25 → A = new
```

because the highest sequence number represents the newest version.

---

# ⚡ LRU Block Cache

The LRU Block Cache stores parsed SSTable blocks in memory.

```text
                  ┌──────────────────┐
                  │   Block Cache    │
                  │                  │
                  │ B1  B7  B9  B12  │
                  └──────────────────┘
                           ▲
                           │
                        GET(key)
                           │
                    SSTable Index
```

The cache uses:

```text
std::unordered_map
        +
std::list
```

to provide average constant-time operations.

```text
Lookup   → O(1) average
Insert   → O(1)
Touch    → O(1)
Eviction → O(1)
```

The cache is bounded, so its memory consumption depends on its configured capacity rather than total database size.

Only successfully validated and parsed blocks are inserted into the cache.

---

# 🛡️ Corruption Handling

Persistent storage is treated as untrusted input.

The engine validates:

- WAL checksums
- WAL record structure
- SSTable footer metadata
- index offsets
- block offsets
- block sizes
- variable-length fields
- physical file boundaries
- truncated files

For example, before accessing a block:

```text
offset
  │
  ├── must be within file
  │
size
  │
  └── must fit completely within file
```

Invalid metadata is rejected instead of blindly allocating memory or reading outside the file's valid boundaries.

---

# 📊 Benchmarks

A cache benchmark was run using **10,000 generated records across multiple SSTables**.

### Random GET

```text
Without Cache : 1028 ms
With Cache    : 479 ms

Improvement   : ~2.15× faster
```

### Hot / Repeated GET

```text
Without Cache : 991 ms
With Cache    : 326 ms

Improvement   : ~3.04× faster
```

### Cache Statistics

```text
Hits          : 18,569
Misses        : 2,792
Hit Rate      : ~86.9%
Evictions     : 2,728
```

These results demonstrate the benefit of caching repeatedly accessed SSTable blocks for the tested workload.

Performance is workload- and environment-dependent and should not be interpreted as a universal database benchmark.

---

# 🧪 Testing

The project includes automated tests covering the core storage-engine pipeline.

Test coverage includes:

- WAL writes and replay
- CRC32 validation
- SkipList operations
- MemTable behavior
- SSTable creation and reading
- SSTable corruption handling
- sequence-number ordering
- sequence recovery after restart
- tombstone handling
- literal tombstone-like user values
- overlapping SSTables
- compaction
- LRU cache behavior
- lifecycle stress testing

Current test result:

```text
24 tests
24 passed
0 failed
```

A larger lifecycle stress test also exercises:

```text
20,000 keys
    │
    ▼
Bulk PUT
    │
    ▼
Flush
    │
    ▼
State validation
    │
    ▼
Delete subset
    │
    ▼
Additional flushes
    │
    ▼
Engine shutdown
    │
    ▼
Restart
    │
    ▼
State validation
```

---

# 📂 Project Structure

```text
LogStoreDB/
│
├── include/
│   ├── Block.h
│   ├── BlockCache.h
│   ├── KVPair.h
│   ├── KVStore.h
│   ├── MemTable.h
│   ├── SSTableReader.h
│   ├── SSTableWriter.h
│   ├── SkipList.h
│   ├── WAL.h
│   └── WALRecord.h
│
├── src/
│   ├── Block.cpp
│   ├── BlockCache.cpp
│   ├── KVStore.cpp
│   ├── MemTable.cpp
│   ├── SSTableReader.cpp
│   ├── SSTableWriter.cpp
│   ├── SkipList.cpp
│   └── WAL.cpp
│
├── cli/
│   └── main.cpp
│
├── tests/
│   ├── test_benchmark.cpp
│   ├── test_cache.cpp
│   ├── test_corruption.cpp
│   ├── test_kvstore.cpp
│   ├── test_memtable.cpp
│   ├── test_sequence.cpp
│   ├── test_skiplist.cpp
│   ├── test_sstable.cpp
│   ├── test_stress.cpp
│   └── test_wal.cpp
│
├── screenshots/
│   ├── WAL-binDump.png
│   ├── cli-demo.png
│   └── crash-recovery.png
│
├── CMakeLists.txt
├── README.md
└── .gitignore
```

---

# 🛠️ Build & Test

LogStoreDB uses **CMake** and **CTest**.

## Configure

```bash
cmake -S . -B build
```

## Build

```bash
cmake --build build
```

## Run Tests

```bash
ctest --test-dir build --output-on-failure
```

---

# 💻 CLI Usage

The command-line interface supports basic key-value operations:

```text
PUT <key> <value>
GET <key>
REMOVE <key>
EXIT
```

Example:

```text
PUT name Sintu
GET name
REMOVE name
GET name
EXIT
```

Example output flow:

```text
PUT name Sintu
→ OK

GET name
→ Sintu

REMOVE name
→ OK

GET name
→ NOT FOUND
```

---

# 🏗️ Design Decisions

## Why WAL?

The WAL provides a persistent record of recent mutations that can be replayed after a process restart.

```text
Mutation
   │
   ▼
WAL
   │
   ▼
MemTable
```

This separates recovery durability from the later SSTable flush process.

---

## Why SkipList?

The SkipList provides ordered in-memory storage with expected logarithmic lookup and insertion behavior while remaining relatively simple to implement from scratch.

Its sorted ordering also makes SSTable generation straightforward.

---

## Why SSTables?

SSTables provide immutable sorted storage on disk.

Their block-oriented structure supports:

- targeted reads
- sequential writes
- indexing
- compaction
- caching

---

## Why Sequence Numbers?

Filesystem order is not a reliable representation of logical update order.

Explicit sequence numbers provide deterministic version selection:

```text
higher seq
    ↓
newer version
```

This becomes particularly important when the same key exists across multiple SSTables.

---

## Why Tombstones?

Deleting only the in-memory entry is insufficient because older copies may still exist in SSTables.

A tombstone ensures that the deletion remains visible during reads and compaction.

---

## Why K-Way Merge?

SSTables are sorted, making them suitable for multi-way merge operations.

Streaming merge avoids materializing all input records into a giant in-memory structure.

---

## Why LRU Cache?

Hot workloads may repeatedly access the same SSTable blocks.

Caching parsed blocks reduces repeated disk reads and parsing work while keeping memory usage bounded.

---

# 📐 Complexity

Approximate expected complexity:

| Operation          | Complexity          |
| ------------------ | ------------------- |
| SkipList lookup    | `O(log N)` expected |
| SkipList insertion | `O(log N)` expected |
| SkipList deletion  | `O(log N)` expected |
| WAL append         | `O(record size)`    |
| MemTable flush     | `O(N)` + disk I/O   |
| LRU lookup         | `O(1)` average      |
| LRU insertion      | `O(1)`              |
| LRU eviction       | `O(1)`              |
| K-way compaction   | `O(N log K)`        |
| Compaction memory  | `O(K)`              |

Where:

- `N` = number of records processed
- `K` = number of input SSTables

---

# 🎯 Learning Outcomes

This project demonstrates practical understanding of systems and storage-engine concepts:

### **LSM-Tree Internals**

Understanding how mutable in-memory state is transformed into immutable sorted disk structures.

### **Custom Data Structures**

Designed and implemented a SkipList rather than relying solely on a standard ordered container.

### **Binary File Formats**

Worked with explicit serialization of:

- sequence numbers
- keys
- values
- sizes
- offsets
- index metadata
- footer metadata

### **Durability & Recovery**

Implemented WAL-based replay and persistent sequence recovery across restarts.

### **Versioned Storage**

Used explicit sequence numbers to resolve multiple physical versions of the same logical key.

### **Deletion Semantics**

Implemented tombstones to prevent deleted data from resurfacing from older SSTables.

### **Compaction**

Implemented a streaming K-way merge rather than materializing all records in memory.

### **Corruption Resistance**

Validated persistent metadata and boundaries before using disk-derived sizes and offsets.

### **Performance Engineering**

Implemented and benchmarked an LRU block cache to measure the impact of locality-aware caching.

### **Testing**

Built tests around correctness, corruption, recovery, compaction, sequence ordering, and larger lifecycle workloads.

---

# ⚠️ Limitations

LogStoreDB is an educational and portfolio-scale storage engine rather than a production replacement for mature databases.

The current implementation does not attempt to provide:

- Distributed replication
- Raft / consensus
- Sharding
- Multi-node coordination
- Multi-key transactions
- Network database protocol
- Full production-grade observability
- Production-scale durability guarantees across every filesystem/hardware configuration
- The complete multi-level compaction sophistication of systems such as RocksDB

These limitations are intentional. The project focuses on implementing and understanding the fundamental mechanisms of a persistent LSM-style storage engine.

---

# 🔮 Future Work

Possible future extensions include:

- Bloom filters
- More sophisticated multi-level compaction
- Immutable/frozen MemTables
- Background flushing
- Background compaction
- Fuzz testing
- Additional crash/failure injection
- Extended benchmarking
- More detailed metrics and observability
- Optional network access

The current core implementation is considered **feature-complete for the project's intended scope**.

---

# 📌 Project Status

```text
┌─────────────────────────────────────────────┐
│            LogStoreDB STATUS                │
├─────────────────────────────────────────────┤
│ WAL                         ✓               │
│ CRC32                       ✓               │
│ Crash Recovery              ✓               │
│ MemTable                    ✓               │
│ SkipList                    ✓               │
│ Sequence Numbers            ✓               │
│ Tombstones                  ✓               │
│ SSTables                    ✓               │
│ Block Storage               ✓               │
│ SSTable Index               ✓               │
│ SSTable Footer              ✓               │
│ Streaming Compaction        ✓               │
│ Corruption Validation       ✓               │
│ Atomic SSTable Installation ✓               │
│ LRU Block Cache             ✓               │
│ Stress Testing              ✓               │
│ Benchmarks                  ✓               │
│ Automated Tests             ✓               │
└─────────────────────────────────────────────┘
```

---

# 📄 License

MIT License
