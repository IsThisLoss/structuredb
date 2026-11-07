"""A thin, test-friendly gRPC client for StructureDB.

Wraps the generated ``Tables`` / ``Transactions`` stubs and exposes ergonomic
methods. The server models a transaction as an opaque ``tx`` token threaded
through every request; ``SdbClient`` lets you pass ``tx=...`` explicitly, or use
the :class:`Transaction` helper / :meth:`SdbClient.transaction` context manager
which thread it for you.
"""

from __future__ import annotations

from dataclasses import dataclass
from typing import List, Optional, Tuple

import grpc

from . import proto_gen
from .waiting import wait_until


@dataclass(frozen=True)
class Record:
    key: str
    value: str


class SdbClient:
    """Connect to a server target (``host:port``) and run table/tx operations."""

    def __init__(self, target: str, connect_timeout: float = 10.0) -> None:
        self._pb = proto_gen.load()
        self.target = target
        self._channel = grpc.insecure_channel(target)
        try:
            grpc.channel_ready_future(self._channel).result(timeout=connect_timeout)
        except grpc.FutureTimeoutError as exc:  # pragma: no cover - startup race
            raise TimeoutError(f"could not connect to {target}") from exc
        self._tables = self._pb.table_grpc.TablesStub(self._channel)
        self._tx = self._pb.tx_grpc.TransactionsStub(self._channel)

    def close(self) -> None:
        self._channel.close()

    def __enter__(self) -> "SdbClient":
        return self

    def __exit__(self, *exc) -> None:
        self.close()

    # -- DDL ---------------------------------------------------------------

    def create_table(self, name: str, tx: Optional[str] = None) -> str:
        req = self._pb.table_pb2.CreateTableRequest(name=name)
        if tx is not None:
            req.tx = tx
        return self._tables.CreateTable(req).tx

    def drop_table(self, name: str, tx: Optional[str] = None) -> str:
        req = self._pb.table_pb2.DropTableRequest(name=name)
        if tx is not None:
            req.tx = tx
        return self._tables.DropTable(req).tx

    def compact_table(self, table: str, tx: Optional[str] = None) -> None:
        req = self._pb.table_pb2.CompactTableRequest(table=table)
        if tx is not None:
            req.tx = tx
        self._tables.CompactTable(req)

    # -- DML ---------------------------------------------------------------

    def upsert(self, table: str, key: str, value: str, tx: Optional[str] = None) -> str:
        req = self._pb.table_pb2.UpsertTableRequest(table=table, key=key, value=value)
        if tx is not None:
            req.tx = tx
        return self._tables.Upsert(req).tx

    def lookup(
        self, table: str, key: str, tx: Optional[str] = None
    ) -> Tuple[Optional[str], str]:
        """Return ``(value_or_None, tx)``. Missing keys yield ``None``."""
        req = self._pb.table_pb2.LookupTableRequest(table=table, key=key)
        if tx is not None:
            req.tx = tx
        resp = self._tables.Lookup(req)
        value = resp.value if resp.HasField("value") else None
        return value, resp.tx

    def get(self, table: str, key: str, tx: Optional[str] = None) -> Optional[str]:
        """Convenience: just the value (or None), dropping the tx token."""
        return self.lookup(table, key, tx)[0]

    def delete(self, table: str, key: str, tx: Optional[str] = None) -> str:
        req = self._pb.table_pb2.DeleteTableRequest(table=table, key=key)
        if tx is not None:
            req.tx = tx
        return self._tables.Delete(req).tx

    def scan(
        self,
        table: str,
        lower_bound: Optional[str] = None,
        upper_bound: Optional[str] = None,
        tx: Optional[str] = None,
    ) -> Tuple[List[Record], str]:
        req = self._pb.table_pb2.ScanTableRequest(table=table)
        if lower_bound is not None:
            req.lower_bound = lower_bound
        if upper_bound is not None:
            req.upper_bound = upper_bound
        if tx is not None:
            req.tx = tx
        resp = self._tables.Scan(req)
        records = [Record(r.key, r.value) for r in resp.records]
        return records, resp.tx

    # -- transactions ------------------------------------------------------

    def begin(self) -> str:
        return self._tx.Begin(self._pb.tx_pb2.BeginRequest()).tx

    def commit(self, tx: str) -> None:
        self._tx.Commit(self._pb.tx_pb2.CommitRequest(tx=tx))

    def transaction(self) -> "Transaction":
        """Open a transaction; commits on clean exit, abandons on exception."""
        return Transaction(self, self.begin())

    # -- replication / async helpers --------------------------------------

    def wait_for_value(
        self, table: str, key: str, expected: str, *, timeout: float = 10.0
    ) -> None:
        """Poll ``lookup`` until ``key`` reads back ``expected`` (e.g. on a follower)."""
        wait_until(
            lambda: self.get(table, key) == expected,
            timeout=timeout,
            message=f"{table}/{key} did not become {expected!r} on {self.target}",
        )


class Transaction:
    """Operations bound to a single ``tx`` token.

    Used as a context manager::

        with client.transaction() as tx:
            tx.upsert("t", "k", "v")
        # committed here
    """

    def __init__(self, client: SdbClient, tx: str) -> None:
        self._client = client
        self.id = tx
        self._done = False

    def create_table(self, name: str) -> None:
        self._client.create_table(name, tx=self.id)

    def drop_table(self, name: str) -> None:
        self._client.drop_table(name, tx=self.id)

    def upsert(self, table: str, key: str, value: str) -> None:
        self._client.upsert(table, key, value, tx=self.id)

    def lookup(self, table: str, key: str) -> Optional[str]:
        return self._client.lookup(table, key, tx=self.id)[0]

    def delete(self, table: str, key: str) -> None:
        self._client.delete(table, key, tx=self.id)

    def scan(
        self, table: str, lower_bound: Optional[str] = None, upper_bound: Optional[str] = None
    ) -> List[Record]:
        return self._client.scan(table, lower_bound, upper_bound, tx=self.id)[0]

    def commit(self) -> None:
        if not self._done:
            self._client.commit(self.id)
            self._done = True

    def __enter__(self) -> "Transaction":
        return self

    def __exit__(self, exc_type, *_) -> None:
        if exc_type is None:
            self.commit()
        # On error we simply drop the token; the server discards uncommitted work.
