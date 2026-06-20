"""A leader + N follower cluster for replication scenarios."""

from __future__ import annotations

from pathlib import Path
from typing import List, Optional

from .client import SdbClient
from .config import ServerConfig
from .ports import free_port
from .server import ServerProcess


class ReplicationCluster:
    """Spin up one leader and one or more followers wired to it.

    Example::

        with ReplicationCluster(workdir, followers=1) as cluster:
            cluster.leader_client.create_table("t")
            cluster.leader_client.upsert("t", "k", "v")
            cluster.follower_client().wait_for_value("t", "k", "v")
    """

    def __init__(
        self,
        workdir: str | Path,
        followers: int = 1,
        binary: Optional[str] = None,
        poll_interval_ms: int = 50,
    ) -> None:
        self.workdir = Path(workdir)
        self._binary = binary
        self._poll = poll_interval_ms
        self._num_followers = followers

        self.leader: Optional[ServerProcess] = None
        self.followers: List[ServerProcess] = []
        self._clients: List[SdbClient] = []

    # -- lifecycle ---------------------------------------------------------

    def start(self) -> "ReplicationCluster":
        leader_port = free_port()
        leader_cfg = ServerConfig(
            root=str(self.workdir / "leader" / "data"),
            port=leader_port,
            role="leader",
            poll_interval_ms=self._poll,
        )
        self.leader = ServerProcess(
            leader_cfg, self.workdir / "leader", binary=self._binary
        ).start()

        for i in range(self._num_followers):
            follower_cfg = ServerConfig(
                root=str(self.workdir / f"follower{i}" / "data"),
                port=free_port(),
                role="follower",
                leader_address=leader_cfg.target,
                poll_interval_ms=self._poll,
            )
            follower = ServerProcess(
                follower_cfg, self.workdir / f"follower{i}", binary=self._binary
            ).start()
            self.followers.append(follower)
        return self

    def stop(self) -> None:
        for client in self._clients:
            client.close()
        self._clients.clear()
        for follower in self.followers:
            follower.stop()
        self.followers.clear()
        if self.leader is not None:
            self.leader.stop()
            self.leader = None

    # -- clients -----------------------------------------------------------

    def _track(self, client: SdbClient) -> SdbClient:
        self._clients.append(client)
        return client

    @property
    def leader_client(self) -> SdbClient:
        assert self.leader is not None, "cluster not started"
        return self._track(SdbClient(self.leader.target))

    def follower_client(self, index: int = 0) -> SdbClient:
        return self._track(SdbClient(self.followers[index].target))

    # -- context manager ---------------------------------------------------

    def __enter__(self) -> "ReplicationCluster":
        return self.start()

    def __exit__(self, *exc) -> None:
        self.stop()
