"""Render a ``config.yaml`` for a server instance under test.

Mirrors ``src/server/cfg/config.hpp``. Defaults are tuned for fast tests:
short flush / compaction / wal-clean intervals so background jobs and
replication converge quickly.
"""

from __future__ import annotations

from dataclasses import dataclass, field
from pathlib import Path
from typing import Optional

import yaml


@dataclass
class ServerConfig:
    root: str
    port: int

    # replication
    role: str = "leader"  # "leader" | "follower"
    leader_address: Optional[str] = None
    poll_interval_ms: int = 50

    # background-job intervals (ms) — short so tests don't wait minutes
    flush_interval_ms: int = 200
    compaction_interval_ms: int = 1000
    wal_clean_interval_ms: int = 1000

    # lsm — small mem table so flushes / SSTables happen with little data
    max_records_in_mem_table: int = 128
    max_ro_mem_tables: int = 1
    page_size: int = 4096
    page_cache_capacity: int = 1024

    # logging
    log_level: str = "info"
    log_console: bool = True
    log_file: Optional[str] = None

    extra: dict = field(default_factory=dict)

    def to_dict(self) -> dict:
        logger: dict = {"level": self.log_level, "console": self.log_console}
        if self.log_file is not None:
            logger["file"] = self.log_file

        replication: dict = {"role": self.role, "poll_interval": self.poll_interval_ms}
        if self.role == "follower":
            if not self.leader_address:
                raise ValueError("follower config requires leader_address")
            replication["leader_address"] = self.leader_address

        cfg = {
            "root": self.root,
            "port": self.port,
            "logger": logger,
            "compaction": {"interval": self.compaction_interval_ms},
            "flush": {"interval": self.flush_interval_ms},
            "wal": {"clean": {"interval": self.wal_clean_interval_ms}},
            "lsm": {
                "max_records_in_mem_table": self.max_records_in_mem_table,
                "max_ro_mem_tables": self.max_ro_mem_tables,
                "page_size": self.page_size,
                "page_cache_capacity": self.page_cache_capacity,
            },
            "replication": replication,
        }
        cfg.update(self.extra)
        return cfg

    def write(self, path: str | Path) -> Path:
        path = Path(path)
        path.write_text(yaml.safe_dump(self.to_dict(), sort_keys=False))
        return path

    @property
    def target(self) -> str:
        """gRPC target (host:port) clients connect to."""
        return f"127.0.0.1:{self.port}"
