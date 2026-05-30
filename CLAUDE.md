# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

StructureDB is a Log-Structured Merge (LSM) tree database implementation in C++20. It provides:
- **Server**: gRPC-based database server with transaction support and WAL recovery
- **Client library**: C++ client SDK for programmatic database access
- **CLI tool**: Interactive command-line interface for database operations
- **SDB Viewer**: Tool to inspect persisted database structure

## Build & Test Commands

### Setup & Building
```bash
make install-deps    # Install Conan dependencies
make cmake          # Generate CMake config
make build          # Build all targets
make clean          # Clean build artifacts
```

### Running
```bash
make run            # Run the gRPC server (listens on port from config.yaml)
make run-cli        # Run the interactive CLI client
```

### Testing
```bash
make tests          # Run all tests via ctest
cmake --build --preset conan-debug -- --target test  # Alternative
ctest --verbose    # Run tests with verbose output
```

To run a specific test:
```bash
cd build/Debug && ctest -R "TestName" -V
```

Tests use C++ coroutines with `co_await` patterns. Test fixtures are defined with `DATABASE_TEST()` macro and inherit async semantics from the test framework (see `tests/server/database/database_test.cpp`).

## Architecture

### Core Layers

**Storage Engine (LSM Tree)**
- `src/server/lsm/` — Log-Structured Merge tree implementation
  - `mem_table.hpp` — In-memory table for writes
  - `ss_table.hpp` — Sorted String tables on disk
  - `compaction/` — Compaction strategies and background compaction
  - `disk/` — File layout and page management
  - `iterators/` — Range and key scanning

**Persistence & Recovery**
- `src/server/wal/` — Write-Ahead Log for durability
  - `writer.hpp` — Log entries written before LSM updates
  - `recovery.hpp` — Replay WAL on startup
  - `cleaner.hpp` — Remove old WAL segments after compaction
- `src/server/sdb/` — SDB file format (LSM + index storage on disk)
- `src/server/io/` — Async file I/O with blocking executor (Boost ASIO)

**Database & Transactions**
- `src/server/database/` — Main database class and session management
  - `database.hpp` — Database lifecycle and session creation
  - `session.hpp` — Per-connection session with active transaction
  - `transaction/storage.hpp` — Transaction isolation and MVCC semantics
  - `catalog.hpp` — Table metadata registry
- `src/server/table/` — Table-level operations and storage
  - `table_client.hpp` — Table access from sessions
  - `storage/` — Per-table LSM instance

**RPC Interface**
- `src/server/services/` — gRPC service implementations
  - `table_service/` — Put, Get, Scan operations
  - `transaction_service/` — Begin, Commit, Rollback
- `proto/` — Protocol Buffer definitions for table, transaction, and replication services

**Client Library**
- `src/client/` — C++ SDK for database access
  - `database.hpp` — Async database client
  - `transaction.hpp` — Async transaction wrapper
  - `table/table_client.hpp` — Table read/write operations

### Async Model

The codebase uses **C++20 coroutines** throughout:
- `Awaitable<T>` is a coroutine wrapper type for async operations
- All I/O and database operations use `co_await` for non-blocking execution
- The `io/blocking_executor.hpp` bridges async operations with Boost ASIO
- Tests use `co_await` with the `DATABASE_TEST()` fixture macro

When adding new operations, return `Awaitable<T>` and use `co_await` internally for any blocking calls (file I/O, LSM operations, WAL writes).

## Key Design Decisions

- **LSM Tree** not used for transaction storage — transactions use a separate, simpler snapshot/isolation mechanism (see commit `333b020`)
- **Page-first reads/writes** — All disk access is page-aligned; sdb files use page-based layout (see commit `e18c54f`)
- **WAL-based recovery** — Database recovers from WAL on startup; old segments cleaned after compaction (see commits `5b2a2c2` and `333b020`)
- **Async everywhere** — No blocking calls on event loop; background jobs (compaction, WAL cleaning) scheduled as coroutines

## Testing Patterns

Tests use `DATABASE_TEST(TestName, { ... })` with `co_await` inside the block:
```cpp
DATABASE_TEST(MyTest, {
  auto& db = GetDatabase();
  auto session = co_await db.StartSession();
  auto table = co_await session.GetTable("my_table");
  // ... assertions with CO_ASSERT_*
  co_await session.Finish();
})
```

Key helper macros:
- `CO_ASSERT_EQ()`, `CO_ASSERT_NE()`, `CO_ASSERT_TRUE()`, etc. — Async-aware assertions
- `GetDatabase()` — Fixture-provided database instance
- `co_await session.Finish()` — Commit or rollback

## Configuration

`config.yaml` controls:
- `logger` — Console/file logging levels
- `database` — Data directory and server port
- Other runtime parameters (see `src/server/cfg/config.hpp`)

When running `make run`, the server reads `./config.yaml` by default (or pass `--config=/path/to/config.yaml`).

## Common Development Tasks

**Adding a new table operation**: Extend `table_client.hpp`, implement in `services/table_service/`, add proto method in `proto/table_service.proto`.

**Fixing transaction isolation issues**: Review transaction snapshot logic in `transaction/storage.hpp` and MVCC semantics.

**Optimizing LSM compaction**: Tune `lsm/compaction/compact_strategy.hpp` heuristics or adjust `kMaxRecordsInMemTable` thresholds in `lsm/lsm.hpp`.

**Investigating WAL recovery failures**: Check `wal/recovery.hpp` for replay logic and ensure page boundaries align with `sdb/` format specs.
