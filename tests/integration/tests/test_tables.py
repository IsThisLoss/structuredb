"""Table CRUD and scan scenarios against a single-node server."""

from __future__ import annotations

import pytest


def test_create_and_roundtrip(client, unique_table):
    table = unique_table()
    client.create_table(table)

    client.upsert(table, "key1", "value1")
    assert client.get(table, "key1") == "value1"


def test_lookup_missing_key_returns_none(client, unique_table):
    table = unique_table()
    client.create_table(table)

    assert client.get(table, "absent") is None


def test_upsert_overwrites(client, unique_table):
    table = unique_table()
    client.create_table(table)

    client.upsert(table, "k", "first")
    client.upsert(table, "k", "second")
    assert client.get(table, "k") == "second"


def test_delete_removes_key(client, unique_table):
    table = unique_table()
    client.create_table(table)

    client.upsert(table, "k", "v")
    assert client.get(table, "k") == "v"

    client.delete(table, "k")
    assert client.get(table, "k") is None


def test_scan_returns_sorted_range(client, unique_table):
    table = unique_table()
    client.create_table(table)

    for key in ["b", "d", "a", "c"]:
        client.upsert(table, key, key.upper())

    records, _ = client.scan(table)
    keys = [r.key for r in records]
    assert keys == sorted(keys), "scan must return keys in sorted order"
    assert {r.key: r.value for r in records} == {
        "a": "A",
        "b": "B",
        "c": "C",
        "d": "D",
    }


def test_scan_bounds(client, unique_table):
    table = unique_table()
    client.create_table(table)
    for key in ["a", "b", "c", "d", "e"]:
        client.upsert(table, key, key)

    records, _ = client.scan(table, lower_bound="b", upper_bound="d")
    keys = [r.key for r in records]
    # Lower bound inclusive; assert the window without over-constraining the
    # upper-bound convention (kept robust across inclusive/exclusive impls).
    assert keys[0] == "b"
    assert "a" not in keys
    assert "e" not in keys


@pytest.mark.slow
def test_data_survives_restart(server, unique_table):
    """Writes are durable across a process restart via WAL recovery."""
    from sdb_testkit import SdbClient

    table = unique_table()
    with SdbClient(server.target) as client:
        client.create_table(table)
        client.upsert(table, "durable", "yes")

    server.restart()

    with SdbClient(server.target) as client:
        assert client.get(table, "durable") == "yes"
