# Log-Structured Key-Value Store (C++)

A crash-safe, append-only, log-structured key-value store built from scratch in C++.

This project implements a simplified storage engine inspired by real-world database systems using write-ahead logging, deterministic replay, and tombstone-based deletes.

---

## 🚀 Features

* In-memory storage using `std::unordered_map`
* Append-only binary Write-Ahead Log (WAL)
* Crash recovery via deterministic replay
* Tombstone-based logical deletes
* Last-write-wins semantics
* Thread-safe operations using `std::mutex`
* Interactive CLI (PUT / GET / REMOVE / EXIT)

---

## 🧠 Architecture Overview

### Write Path (PUT)

```
PUT key value →
  1. Append binary record to data.bin
  2. Update in-memory map
```

Durability is guaranteed because disk is written before memory is updated.

---

### Delete Path (REMOVE)

```
REMOVE key →
  1. Append tombstone record (value_size = -1)
  2. Erase key from memory
```

Deletes are logical. Old records remain in the log but are ignored during replay.

---

### Recovery on Startup

On initialization:

```
Replay entire binary log →
Reconstruct latest state →
Last write wins
```

Partial or corrupted tail records are safely ignored.

---

## 📦 Binary Log Format

Each record is stored as:

```
[int key_size][int value_size][key bytes][value bytes]
```

### Tombstone Record

```
value_size == -1
```

This indicates a DELETE operation.

Storing sizes before data ensures precise parsing in a raw binary stream.

---

## 🔒 Thread Safety

All operations (`PUT`, `GET`, `REMOVE`, and `replay`) are protected by a single `std::mutex`.

This guarantees:

* No race conditions on `unordered_map`
* No interleaving of log writes
* Atomic disk + memory updates

The design prioritizes correctness over parallel performance.

---

## 🛠 Build & Run

Compile:

```bash
g++ main.cpp KVStore.cpp -std=c++17 -o kvstore
```

Run:

```bash
./kvstore
```

Supported commands:

```
PUT key value
GET key
REMOVE key
EXIT
```

---

## 🧪 Crash Safety

The system is resilient to:

* Forced termination during writes
* Partial log records
* Restart after unexpected shutdown

The log is the single source of truth.

---

## ⚠ Limitations

* No log compaction (file grows indefinitely)
* Single global mutex (no fine-grained locking)
* No networking interface
* No disk-based indexing
* No unit test suite

This project focuses on core storage engine concepts.

---

## 🎯 Learning Outcomes

This project demonstrates understanding of:

* Binary serialization
* Write-Ahead Logging (WAL)
* Log-structured storage design
* Tombstone-based deletes
* Crash recovery via replay
* Concurrency control using mutex
* Separation of storage logic and interface

---

## 📌 Future Improvements (Optional)

* Log compaction
* Offset-based index for disk reads
* Read-write locks for better concurrency
* Networking layer
* Benchmarks and stress tests

---

## 📄 License

Educational / personal project.

---