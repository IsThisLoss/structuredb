# StructureDB integration tests

Black-box integration tests written in Python. They start real
`structuredb-server` processes and drive them over gRPC, exercising tables,
transaction isolation and replication end-to-end.

## Running

Via ctest (recommended — bootstraps its own virtualenv, builds nothing else):

```bash
make build                     # ensure structuredb-server is built
cd build/Debug && ctest -R integration --output-on-failure
```

Directly, against an already-built binary:

```bash
cd tests/integration
python3 run_tests.py --server-binary ../../build/Debug/src/server/structuredb-server
# forward args to pytest after `--`:
python3 run_tests.py --server-binary <bin> -- -k replication -v
```

`run_tests.py` creates `.venv/`, installs `requirements.txt`, generates the
Python gRPC stubs from `proto/*.proto` and runs pytest. Everything generated is
gitignored.

## Layout

```
tests/integration/
  sdb_testkit/        # the framework
    config.py         # ServerConfig -> config.yaml
    server.py         # ServerProcess: spawn / ready-wait / restart / teardown
    client.py         # SdbClient + Transaction (gRPC wrapper, tx threading)
    cluster.py        # ReplicationCluster: leader + N followers
    proto_gen.py      # compile & import the .proto stubs on demand
    ports.py, waiting.py
  conftest.py         # pytest fixtures
  tests/              # scenarios: test_tables / test_transactions / test_replication
  run_tests.py        # ctest entry point (venv + deps + pytest)
```

## Fixtures

| Fixture          | Gives you                                                    |
|------------------|--------------------------------------------------------------|
| `server`         | a running single-node `ServerProcess` (auto torn down)       |
| `client`         | an `SdbClient` connected to `server`                         |
| `server_config`  | the `ServerConfig` for `server` (override to tweak settings) |
| `make_cluster`   | factory: `make_cluster(followers=1)` -> started cluster      |
| `unique_table`   | callable returning collision-free table names               |
| `workdir`        | per-test temp dir (configs, data, `server.log` live here)    |

## Writing a test

```python
def test_roundtrip(client, unique_table):
    users = unique_table("users")
    client.create_table(users)
    client.upsert(users, "alice", "engineer")
    assert client.get(users, "alice") == "engineer"


def test_isolation(server):
    from sdb_testkit import SdbClient
    with SdbClient(server.target) as writer, SdbClient(server.target) as reader:
        writer.create_table("accounts")
        transaction = writer.begin()
        writer.upsert("accounts", "alice", "100", tx=transaction)
        assert reader.get("accounts", "alice") is None   # not visible before commit
        writer.commit(transaction)
        assert reader.get("accounts", "alice") == "100"


@pytest.mark.replication
def test_replicates(make_cluster):
    cluster = make_cluster(followers=1)
    cluster.leader_client.create_table("accounts")
    cluster.leader_client.upsert("accounts", "alice", "100")
    cluster.follower_client().wait_for_value("accounts", "alice", "100")  # async; polled
```

Replication is asynchronous, so reads on a follower are polled via
`client.wait_for_value(...)` or `sdb_testkit.wait_until(...)` rather than read
once.

## Requirements

A Python 3 interpreter on the host. All Python packages (`grpcio`,
`grpcio-tools`, `pytest`, `PyYAML`) are installed into the local venv by
`run_tests.py`; nothing is installed system-wide.
