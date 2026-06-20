"""Pytest fixtures for StructureDB integration tests.

Make the ``sdb_testkit`` package importable regardless of where pytest is
invoked from, then expose fixtures that hand tests a ready server + client, and
a factory for replication clusters.
"""

from __future__ import annotations

import sys
import uuid
from pathlib import Path
from typing import Callable, Iterator

import pytest

_HERE = Path(__file__).resolve().parent
if str(_HERE) not in sys.path:
    sys.path.insert(0, str(_HERE))

from sdb_testkit import ReplicationCluster, SdbClient, ServerConfig, ServerProcess  # noqa: E402
from sdb_testkit.ports import free_port  # noqa: E402


@pytest.fixture(scope="session", autouse=True)
def _generate_proto_stubs() -> None:
    """Compile the .proto files once per session before any client is built."""
    from sdb_testkit import proto_gen

    proto_gen.load()


@pytest.fixture
def workdir(tmp_path: Path) -> Path:
    """A clean per-test working directory (configs, data, logs live here)."""
    return tmp_path


@pytest.fixture
def server_config(workdir: Path) -> ServerConfig:
    """Default single-node leader config on a free port."""
    return ServerConfig(root=str(workdir / "data"), port=free_port())


@pytest.fixture
def server(server_config: ServerConfig, workdir: Path) -> Iterator[ServerProcess]:
    """A running single-node server, torn down at test end."""
    proc = ServerProcess(server_config, workdir)
    proc.start()
    try:
        yield proc
    finally:
        proc.stop()


@pytest.fixture
def client(server: ServerProcess) -> Iterator[SdbClient]:
    """A connected client to the single-node ``server`` fixture."""
    cli = SdbClient(server.target)
    try:
        yield cli
    finally:
        cli.close()


@pytest.fixture
def unique_table() -> Callable[[], str]:
    """Return a generator of collision-free table names."""

    def _make(prefix: str = "t") -> str:
        return f"{prefix}_{uuid.uuid4().hex[:8]}"

    return _make


@pytest.fixture
def make_cluster(workdir: Path) -> Iterator[Callable[..., ReplicationCluster]]:
    """Factory fixture: ``make_cluster(followers=1)`` -> started cluster.

    All clusters created during a test are stopped at teardown.
    """
    created: list[ReplicationCluster] = []

    def _make(followers: int = 1) -> ReplicationCluster:
        cluster = ReplicationCluster(
            workdir / f"cluster{len(created)}", followers=followers
        )
        cluster.start()
        created.append(cluster)
        return cluster

    try:
        yield _make
    finally:
        for cluster in created:
            cluster.stop()
