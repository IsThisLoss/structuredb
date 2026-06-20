"""Manage the lifecycle of a single ``structuredb-server`` process."""

from __future__ import annotations

import os
import signal
import subprocess
import time
from pathlib import Path
from typing import Optional

from .config import ServerConfig
from .ports import port_is_open


def _server_binary() -> str:
    binary = os.environ.get("STRUCTUREDB_SERVER_BIN")
    if not binary:
        raise RuntimeError(
            "STRUCTUREDB_SERVER_BIN is not set. Run the suite via ctest, or "
            "export it to the built server path "
            "(e.g. build/Debug/src/server/structuredb-server)."
        )
    if not Path(binary).is_file():
        raise FileNotFoundError(f"server binary not found: {binary}")
    return binary


class ServerProcess:
    """A running server instance: writes its config, spawns it, waits for ready.

    Use as a context manager or call ``start()`` / ``stop()`` explicitly.
    Stdout/stderr are captured to ``<workdir>/server.log`` and the tail is
    surfaced on a failed startup to make debugging easy.
    """

    def __init__(
        self,
        config: ServerConfig,
        workdir: str | Path,
        binary: Optional[str] = None,
        startup_timeout: float = 20.0,
    ) -> None:
        self.config = config
        self.workdir = Path(workdir)
        self.binary = binary or _server_binary()
        self.startup_timeout = startup_timeout
        self._proc: Optional[subprocess.Popen] = None
        self._log_path = self.workdir / "server.log"
        self._log_file = None

    # -- lifecycle ---------------------------------------------------------

    def start(self) -> "ServerProcess":
        self.workdir.mkdir(parents=True, exist_ok=True)
        # create_directory() in the server is non-recursive, so make sure the
        # data root's parent exists; creating root itself is harmless.
        Path(self.config.root).mkdir(parents=True, exist_ok=True)

        config_path = self.config.write(self.workdir / "config.yaml")
        self._log_file = open(self._log_path, "wb")
        self._proc = subprocess.Popen(
            [self.binary, f"--config={config_path}"],
            stdout=self._log_file,
            stderr=subprocess.STDOUT,
            cwd=str(self.workdir),
        )
        self._wait_until_ready()
        return self

    def _wait_until_ready(self) -> None:
        deadline = time.monotonic() + self.startup_timeout
        host, port = "127.0.0.1", self.config.port
        while time.monotonic() < deadline:
            if self._proc.poll() is not None:
                raise RuntimeError(
                    f"server exited early (code {self._proc.returncode}) "
                    f"during startup:\n{self.log_tail()}"
                )
            if port_is_open(host, port):
                return
            time.sleep(0.05)
        self.stop()
        raise TimeoutError(
            f"server did not open {host}:{port} within {self.startup_timeout}s:\n"
            f"{self.log_tail()}"
        )

    def stop(self, timeout: float = 10.0) -> None:
        proc = self._proc
        if proc is not None and proc.poll() is None:
            proc.send_signal(signal.SIGTERM)
            try:
                proc.wait(timeout=timeout)
            except subprocess.TimeoutExpired:
                proc.kill()
                proc.wait(timeout=timeout)
        self._proc = None
        if self._log_file is not None:
            self._log_file.close()
            self._log_file = None

    def restart(self) -> "ServerProcess":
        """Stop and start again with the same config/data dir (recovery path)."""
        self.stop()
        return self.start()

    # -- introspection -----------------------------------------------------

    @property
    def target(self) -> str:
        return self.config.target

    @property
    def pid(self) -> Optional[int]:
        return self._proc.pid if self._proc else None

    def is_running(self) -> bool:
        return self._proc is not None and self._proc.poll() is None

    def log_tail(self, lines: int = 60) -> str:
        try:
            content = self._log_path.read_text(errors="replace").splitlines()
        except OSError:
            return "<no server log>"
        return "\n".join(content[-lines:])

    # -- context manager ---------------------------------------------------

    def __enter__(self) -> "ServerProcess":
        return self.start()

    def __exit__(self, *exc) -> None:
        self.stop()
