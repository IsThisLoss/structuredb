# Repository Guidelines

## Project Structure & Module Organization

StructureDB is a C++20 database with an LSM storage engine, WAL recovery, transactions, and gRPC replication. `src/server/` contains the engine (`lsm/`, `wal/`, `sdb/`), database logic, I/O, and services. The C++ SDK is in `src/client/`, the interactive client in `src/cli/`, and the file inspector in `src/sdb_viewer/`. Define RPC interfaces in `proto/`. C++ tests live in `tests/server/`; Python integration tests and their helpers live in `tests/integration/`. `third_party/` contains vendored dependencies, and `docs/replication.md` describes replication operations.

## Build, Test, and Development Commands

Install a C++20 compiler, CMake 3.30 or newer, Conan 2.x, and Python 3 for integration tests.

- `make install-deps` installs Debug dependencies through Conan.
- `make build` configures and builds all targets using the `conan-debug` preset.
- `make run` builds and starts the server using `config.yaml`; `make run-cli` starts the built CLI.
- `make tests` builds and runs the CTest suites.
- `ctest --test-dir build/Debug -R integration --output-on-failure` runs the integration suite after building. Its runner creates a virtual environment and installs Python dependencies.
- `make clean` removes the build directory and `CMakeUserPresets.json`; rerun `make install-deps` before building again.

## Coding Style & Server Concurrency

Follow nearby code: two-space C++ indentation, `.hpp` headers, `snake_case` filenames and variables, PascalCase types and methods, and trailing underscores on data members. `.clang-tidy` configures analyzer, Core Guidelines, and modernization checks with warnings treated as errors and Google formatting.

`src/server/` uses C++20 coroutines on Boost.Asio. Standard blocking synchronization and waiting primitives, including `std::mutex`, `std::condition_variable`, and `std::this_thread::sleep_for`, are forbidden in server code. Use coroutine-friendly primitives from `src/server/io/`; introduce a new coroutine-friendly primitive there if needed. Synchronization and waits must suspend the coroutine without blocking the event-loop thread. Route blocking file operations through the blocking executor.

## Testing Guidelines

Use GoogleTest and `*_test.cpp` files for C++ tests. Database tests use `DATABASE_TEST(Name, { ... })` and `CO_ASSERT_*` helpers. Add descriptive pytest `test_*` scenarios for end-to-end behavior and reuse the integration fixtures for server lifecycle and temporary data. Cover regressions and changed behavior; no numeric coverage threshold is configured.

## Commit & Pull Request Guidelines

Recent commits use `type(scope): description`, for example `fix(replication): retain WAL needed by connected followers`. Keep commits focused. In pull requests, describe the behavior change, link relevant issues, and report the commands and results used for validation. Call out protocol, persistence-format, and configuration changes.
