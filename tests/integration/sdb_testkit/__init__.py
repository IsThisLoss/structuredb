"""sdb_testkit — a small framework for StructureDB integration tests.

It spins up real ``structuredb-server`` processes, talks to them over gRPC and
gives tests high-level helpers for tables, transactions and replication.

The public surface is intentionally tiny:

    from sdb_testkit import SdbClient, ServerConfig, ServerProcess, ReplicationCluster

Most tests never touch these directly — they use the pytest fixtures in
``conftest.py`` (``client``, ``server``, ``make_cluster`` …).
"""

from .config import ServerConfig
from .server import ServerProcess
from .client import SdbClient, Record, Transaction
from .cluster import ReplicationCluster
from .waiting import wait_until

__all__ = [
    "ServerConfig",
    "ServerProcess",
    "SdbClient",
    "Record",
    "Transaction",
    "ReplicationCluster",
    "wait_until",
]
