# Repository Guidelines

## Project Structure & Module Organization

StructureDB is a C++20 LSM-tree database using Boost.Asio coroutines and gRPC. Server code lives in `src/server/`, grouped into storage (`lsm/`, `sdb/`, `wal/`), database, transactions, I/O, and RPC services. `src/client/` contains the SDK, `src/cli/` the interactive client, and `src/sdb_viewer/` the file inspector. Protocol definitions live in `proto/`; external code lives in `third_party/`. C++ tests are in `tests/server/`, and Python end-to-end scenarios are in `tests/integration/`. See `docs/replication.md` for replication operations.

## Build, Test, and Development Commands

Use a C++20 compiler, CMake 3.30+, and Conan 2.x.

- `make install-deps`: install Debug dependencies through Conan.
- `make build`: configure and build using the `conan-debug` preset.
- `make run`: build and launch the server with `./config.yaml`.
- `make run-cli`: launch the already-built CLI.
- `make tests`: build and run all registered CTest suites.
- `ctest --test-dir build/Debug -R integration --output-on-failure`: run integration tests against built binaries. Python 3 is required; the runner creates a virtualenv and installs dependencies.
- `make clean`: remove `build/` and `CMakeUserPresets.json`; reinstall dependencies before rebuilding.

## Coding Style & Naming Conventions

Match surrounding C++ code: two-space indentation, `.hpp` headers, `snake_case` filenames and variables, PascalCase types and methods, and trailing underscores for member fields. `.clang-tidy` enables analyzer, C++ Core Guidelines, and modernization checks, treats warnings as errors, and selects Google formatting. Preserve the coroutine model: use `Awaitable<T>` and `co_await`, and route blocking file work through the blocking executor.

## Server Coroutine Requirements

`src/server/` uses C++20 coroutines on top of Boost.Asio. Standard C++ blocking synchronization and waiting primitives, including `std::mutex`, `std::condition_variable`, and `std::this_thread::sleep_for`, are forbidden in server code. Use the coroutine-friendly primitives in `src/server/io/` instead. If those primitives are insufficient, introduce a new coroutine-friendly primitive there; synchronization and waiting must suspend the coroutine without blocking the event-loop thread.

## Testing Guidelines

Use GoogleTest for C++ tests in `*_test.cpp`. Database scenarios use `DATABASE_TEST(Name, { ... })` and `CO_ASSERT_*` helpers. Add descriptive pytest `test_*` scenarios for observable table, transaction, recovery, or replication behavior. Reuse integration fixtures for server lifecycle and temporary data. No numeric coverage threshold is configured; cover changed behavior and regressions.

## Commit & Pull Request Guidelines

Recent commits follow `type(scope): description`, such as `fix(replication): retain WAL needed by connected followers`. Keep commits focused. Pull requests should describe the behavior change, link relevant issues, and report validation commands and results. Explain protocol, persistence-format, or configuration changes explicitly.
