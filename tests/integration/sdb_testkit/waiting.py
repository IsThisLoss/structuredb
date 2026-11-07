"""Polling helpers for asynchronous behaviour (flush, compaction, replication)."""

from __future__ import annotations

import time
from typing import Callable, Optional, TypeVar

T = TypeVar("T")


class TimeoutError_(AssertionError):
    """Raised when a polled condition does not become true in time."""


def wait_until(
    predicate: Callable[[], T],
    *,
    timeout: float = 10.0,
    interval: float = 0.1,
    message: Optional[str] = None,
) -> T:
    """Poll ``predicate`` until it returns a truthy value or ``timeout`` elapses.

    Returns the truthy value. Raises ``TimeoutError_`` on timeout. Exceptions
    raised by ``predicate`` are swallowed and retried until the deadline, so it
    is safe to poll an endpoint that is briefly unavailable.
    """
    deadline = time.monotonic() + timeout
    last_exc: Optional[BaseException] = None
    while True:
        try:
            value = predicate()
            if value:
                return value
        except Exception as exc:  # noqa: BLE001 - retry transient failures
            last_exc = exc
        if time.monotonic() >= deadline:
            detail = message or "condition not met"
            if last_exc is not None:
                detail += f" (last error: {last_exc!r})"
            raise TimeoutError_(f"wait_until timed out after {timeout}s: {detail}")
        time.sleep(interval)
