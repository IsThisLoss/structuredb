"""Transaction scenarios: read-your-writes, isolation, atomic commit.

The server models a transaction as an opaque ``tx`` token threaded through
requests. These tests exercise that surface via the high-level helpers.
"""

from __future__ import annotations

import pytest


def test_read_your_own_writes(client, unique_table):
    accounts = unique_table("accounts")
    client.create_table(accounts)

    with client.transaction() as transaction:
        transaction.upsert(accounts, "alice", "100")
        # The writing transaction sees its own uncommitted write.
        assert transaction.lookup(accounts, "alice") == "100"


def test_uncommitted_write_isolated_from_other_reader(server, unique_table):
    """A separate connection must not observe an in-flight transaction's write."""
    from sdb_testkit import SdbClient

    accounts = unique_table("accounts")
    with SdbClient(server.target) as writer, SdbClient(server.target) as reader:
        writer.create_table(accounts)

        transaction = writer.begin()
        writer.upsert(accounts, "alice", "100", tx=transaction)

        # Outside the transaction the key is not yet visible.
        assert reader.get(accounts, "alice") is None

        writer.commit(transaction)

        # After commit the value becomes visible to the other reader.
        assert reader.get(accounts, "alice") == "100"


def test_committed_transaction_is_visible(client, unique_table):
    accounts = unique_table("accounts")
    client.create_table(accounts)

    with client.transaction() as transaction:
        transaction.upsert(accounts, "alice", "100")
        transaction.upsert(accounts, "bob", "250")

    assert client.get(accounts, "alice") == "100"
    assert client.get(accounts, "bob") == "250"


def test_abandoned_transaction_not_applied(server, unique_table):
    """A transaction whose token is dropped without commit leaves no trace."""
    from sdb_testkit import SdbClient

    accounts = unique_table("accounts")
    with SdbClient(server.target) as setup:
        setup.create_table(accounts)

    with SdbClient(server.target) as writer:
        transaction = writer.begin()
        writer.upsert(accounts, "carol", "999", tx=transaction)
        # never commit

    with SdbClient(server.target) as reader:
        assert reader.get(accounts, "carol") is None


@pytest.mark.slow
def test_committed_transaction_survives_restart(server, unique_table):
    from sdb_testkit import SdbClient

    accounts = unique_table("accounts")
    with SdbClient(server.target) as client:
        client.create_table(accounts)
        with client.transaction() as transaction:
            transaction.upsert(accounts, "alice", "100")

    server.restart()

    with SdbClient(server.target) as client:
        assert client.get(accounts, "alice") == "100"
