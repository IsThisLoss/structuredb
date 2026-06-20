"""Table CRUD and scan scenarios against a single-node server."""

from __future__ import annotations

import pytest


def test_create_and_roundtrip(client, unique_table):
    users = unique_table("users")
    client.create_table(users)

    client.upsert(users, "alice", "engineer")
    assert client.get(users, "alice") == "engineer"


def test_lookup_missing_key_returns_none(client, unique_table):
    users = unique_table("users")
    client.create_table(users)

    assert client.get(users, "absent") is None


def test_upsert_overwrites(client, unique_table):
    users = unique_table("users")
    client.create_table(users)

    client.upsert(users, "alice", "engineer")
    client.upsert(users, "alice", "manager")
    assert client.get(users, "alice") == "manager"


def test_delete_removes_key(client, unique_table):
    users = unique_table("users")
    client.create_table(users)

    client.upsert(users, "alice", "engineer")
    assert client.get(users, "alice") == "engineer"

    client.delete(users, "alice")
    assert client.get(users, "alice") is None


def test_scan_returns_sorted_range(client, unique_table):
    countries = unique_table("countries")
    client.create_table(countries)

    capitals = {
        "france": "paris",
        "germany": "berlin",
        "italy": "rome",
        "spain": "madrid",
    }
    for country, capital in capitals.items():
        client.upsert(countries, country, capital)

    records, _ = client.scan(countries)
    scanned_keys = [record.key for record in records]
    assert scanned_keys == sorted(scanned_keys), "scan must return keys in sorted order"
    assert {record.key: record.value for record in records} == capitals


def test_scan_bounds(client, unique_table):
    letters = unique_table("letters")
    client.create_table(letters)
    for letter in ["alpha", "bravo", "charlie", "delta", "echo"]:
        client.upsert(letters, letter, letter.upper())

    records, _ = client.scan(letters, lower_bound="bravo", upper_bound="delta")
    scanned_keys = [record.key for record in records]
    # Lower bound inclusive; assert the window without over-constraining the
    # upper-bound convention (kept robust across inclusive/exclusive impls).
    assert scanned_keys[0] == "bravo"
    assert "alpha" not in scanned_keys
    assert "echo" not in scanned_keys


@pytest.mark.slow
def test_data_survives_restart(server, unique_table):
    """Writes are durable across a process restart via WAL recovery."""
    from sdb_testkit import SdbClient

    accounts = unique_table("accounts")
    with SdbClient(server.target) as client:
        client.create_table(accounts)
        client.upsert(accounts, "alice", "100")

    server.restart()

    with SdbClient(server.target) as client:
        assert client.get(accounts, "alice") == "100"
