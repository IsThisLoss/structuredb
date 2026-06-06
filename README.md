# StructureDB

StructureDB is a Log-Structured Merge (LSM) tree database written from scratch in modern
**C++20**. It is a learning-oriented project that implements a real storage engine end to
end: an LSM-based storage layer, a Write-Ahead Log for durability, transactions, a gRPC
server, a client SDK, and an interactive CLI.

The whole codebase is built around **C++20 coroutines** — all I/O, LSM, and WAL operations
are asynchronous (`co_await`) and run on a Boost.Asio event loop with no blocking calls.

## Features

- **LSM tree storage engine** — in-memory memtables, immutable SSTables on disk, and
  background compaction.
- **Write-Ahead Log (WAL)** — every mutation is logged before it is applied; the database
  replays the WAL on startup to recover, and old segments are cleaned after compaction.
- **Transactions** — `BEGIN` / `COMMIT` / `ROLLBACK` with snapshot isolation, kept separate
  from the LSM storage path.
- **Page-aligned on-disk format** — the `.sdb` file format stores LSM data and indexes in
  fixed-size pages, with an LRU page cache in front of disk reads.
- **gRPC interface** — table, transaction, and replication services defined in Protocol
  Buffers.
- **System views** — introspect runtime state through `sys.tables` and `sys.transactions`.
- **Tooling** — an interactive REPL CLI with autocompletion/hints, and an `sdb-viewer` tool
  for inspecting persisted database files.

## Architecture

```
                ┌──────────────┐        ┌──────────────┐
   CLI / SDK ─► │  gRPC server │ ─────► │  Database    │
                └──────────────┘        │  + Sessions  │
                                        │  + Txns      │
                                        └──────┬───────┘
                                               │
                          ┌────────────────────┼────────────────────┐
                          ▼                     ▼                    ▼
                   ┌────────────┐        ┌────────────┐       ┌────────────┐
                   │    WAL     │        │  LSM Tree  │       │  Catalog / │
                   │  (writer / │        │ memtable + │       │  system    │
                   │  recovery) │        │  SSTables  │       │  views     │
                   └────────────┘        └─────┬──────┘       └────────────┘
                                               ▼
                                     ┌───────────────────┐
                                     │  .sdb page format │
                                     │  + LRU page cache │
                                     └───────────────────┘
```

Source layout:

| Path | Responsibility |
| --- | --- |
| `src/server/lsm/` | LSM tree: memtables, SSTables, compaction, disk layout, iterators |
| `src/server/wal/` | Write-Ahead Log: writer, recovery, cleaner |
| `src/server/sdb/` | `.sdb` page-based file format (LSM + index storage) |
| `src/server/io/` | Async file I/O on a blocking executor (Boost.Asio) |
| `src/server/cache/` | LRU page cache |
| `src/server/database/` | Database lifecycle, sessions, background jobs, system views |
| `src/server/table/` | Table-level operations and per-table storage |
| `src/server/transaction/` | Transaction isolation / snapshot semantics |
| `src/server/services/` | gRPC service implementations (tables, transactions) |
| `src/client/` | Async C++ client SDK |
| `src/cli/` | Interactive REPL command-line client |
| `src/sdb_viewer/` | Tool to inspect persisted `.sdb` files |
| `proto/` | Protocol Buffer definitions |

A deeper architectural overview lives in [`CLAUDE.md`](CLAUDE.md).

## Requirements

- A C++20 compiler
- [CMake](https://cmake.org/) ≥ 3.30
- [Conan](https://conan.io/) 2.x (package manager)

Dependencies are pulled in via Conan and include Boost, gRPC, Protobuf, spdlog,
yaml-cpp, and GoogleTest.

## Build

```bash
make install-deps    # Install dependencies via Conan
make cmake           # Generate the CMake configuration
make build           # Build all targets
```

`make build` already runs `make cmake`, so a plain `make build` after `install-deps` is
enough. Use `make clean` to remove build artifacts.

## Run

Start the server (reads `./config.yaml` by default, listens on the configured port):

```bash
make run
```

In another terminal, start the interactive CLI:

```bash
make run-cli
```

The CLI connects to `localhost:50051` by default; override it with `--target`, or run a
single command non-interactively with `-c`:

```bash
./build/Debug/src/cli/structuredb-cli --target=localhost:50051
./build/Debug/src/cli/structuredb-cli -c "SCAN my_table"
```

## CLI commands

| Command | Description |
| --- | --- |
| `CREATE TABLE <table>` | Create a table |
| `DROP TABLE <table>` | Drop a table |
| `UPSERT <table> <key> <value>` | Insert or update a key |
| `LOOKUP <table> <key>` | Read a value by key |
| `DELETE <table> <key>` | Delete a key |
| `SCAN <table> [<lower_bound> <upper_bound>]` | Range scan over keys |
| `BEGIN` | Start a transaction |
| `COMMIT` | Commit the active transaction |
| `ROLLBACK` | Roll back the active transaction |
| `exit` / `quit` | Leave the REPL |

The REPL offers tab-completion and inline hints. When a transaction is open the prompt
switches to `structuredb (tx)>`.

Example session:

```
structuredb> CREATE TABLE users
structuredb> UPSERT users alice "Alice"
structuredb> UPSERT users bob "Bob"
structuredb> LOOKUP users alice
Alice
structuredb> BEGIN
structuredb (tx)> UPSERT users carol "Carol"
structuredb (tx)> COMMIT
structuredb> SCAN users a c
alice -> Alice
bob   -> Bob
```

## Configuration

Runtime behaviour is controlled by `config.yaml`:

```yaml
root: /tmp/db          # data directory
port: 50051            # gRPC listen port
logger:
  level: debug
  console: true
compaction:
  interval: 60000      # background compaction interval (ms)
flush:
  interval: 1000       # memtable flush interval (ms)
wal:
  clean:
    interval: 60000    # WAL cleanup interval (ms)
lsm:
  max_records_in_mem_table: 10000
  max_ro_mem_tables: 1
  page_size: 4096
  page_cache_capacity: 1024
```

Pass a custom file with `--config=/path/to/config.yaml`.

## Tests

```bash
make tests           # Build and run the full test suite via ctest
```

Run a specific test with verbose output:

```bash
cd build/Debug && ctest -R "TestName" -V
```

Tests are written with GoogleTest and C++ coroutines. Database tests use the
`DATABASE_TEST(Name, { ... })` fixture macro with `co_await` and `CO_ASSERT_*` async
assertions (see `tests/server/database/`).

## gRPC API

The server exposes three Protocol Buffer services (see `proto/`):

- **Tables** — `Upsert`, `Lookup`, `Scan`, `Delete`, `CreateTable`, `DropTable`,
  `CompactTable`
- **Transactions** — `Begin`, `Commit`
- **Replication** — `GetEvents` (streams WAL events)

## License

See [`LICENSE`](LICENSE).
