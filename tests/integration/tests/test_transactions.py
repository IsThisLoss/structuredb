"""Transaction scenarios: read-your-writes, isolation, atomic commit.

The server models a transaction as an opaque ``tx`` token threaded through
requests. These tests exercise that surface via the high-level helpers.
"""

from __future__ import annotations

import pytest


def test_read_your_own_writes(client, unique_table):
    table = unique_table()
    client.create_table(table)

    with client.transaction() as tx:
        tx.upsert(table, "k", "inside")
        # The writing transaction sees its own uncommitted write.
        assert tx.lookup(table, "k") == "inside"


def test_uncommitted_write_isolated_from_other_reader(server, unique_table):
    """A separate connection must not observe an in-flight transaction's write."""
    from sdb_testkit import SdbClient

    table = unique_table()
    with SdbClient(server.target) as writer, SdbClient(server.target) as reader:
        writer.create_table(table)

        tx = writer.begin()
        writer.upsert(table, "k", "pending", tx=tx)

        # Outside the transaction the key is not yet visible.
        assert reader.get(table, "k") is None

        writer.commit(tx)

        # After commit the value becomes visible to the other reader.
        assert reader.get(table, "k") == "pending"


def test_committed_transaction_is_visible(client, unique_table):
    table = unique_table()
    client.create_table(table)

    with client.transaction() as tx:
        tx.upsert(table, "a", "1")
        tx.upsert(table, "b", "2")

    assert client.get(table, "a") == "1"
    assert client.get(table, "b") == "2"


def test_abandoned_transaction_not_applied(server, unique_table):
    """A transaction whose token is dropped without commit leaves no trace."""
    from sdb_testkit import SdbClient

    table = unique_table()
    with SdbClient(server.target) as setup:
        setup.create_table(table)

    with SdbClient(server.target) as writer:
        tx = writer.begin()
        writer.upsert(table, "ghost", "value", tx=tx)
        # never commit

    with SdbClient(server.target) as reader:
        assert reader.get(table, "ghost") is None


@pytest.mark.slow
def test_committed_transaction_survives_restart(server, unique_table):
    from sdb_testkit import SdbClient

    table = unique_table()
    with SdbClient(server.target) as client:
        client.create_table(table)
        with client.transaction() as tx:
            tx.upsert(table, "k", "committed")

    server.restart()

    with SdbClient(server.target) as client:
        assert client.get(table, "k") == "committed"
