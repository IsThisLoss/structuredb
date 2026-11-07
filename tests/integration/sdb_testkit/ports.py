"""Free-port allocation for ephemeral server instances."""

from __future__ import annotations

import socket


def free_port() -> int:
    """Return a currently-free TCP port on localhost.

    There is an inherent race between releasing the port here and the server
    binding it, but for local test processes started immediately afterwards it
    is reliable in practice.
    """
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        sock.bind(("127.0.0.1", 0))
        return sock.getsockname()[1]


def port_is_open(host: str, port: int, timeout: float = 0.25) -> bool:
    """Return True if a TCP connection to ``host:port`` succeeds."""
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as sock:
        sock.settimeout(timeout)
        try:
            sock.connect((host, port))
            return True
        except OSError:
            return False
