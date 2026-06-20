"""Leader -> follower replication scenarios.

These use the ``make_cluster`` fixture, which spins up a leader plus one or more
followers wired to stream its WAL. Replication is asynchronous, so reads on the
follower are polled with ``wait_for_value`` / ``wait_until``.
"""

from __future__ import annotations

import grpc
import pytest

pytestmark = [pytest.mark.replication, pytest.mark.slow]


def test_write_propagates_to_follower(make_cluster):
    cluster = make_cluster(followers=1)
    leader = cluster.leader_client
    follower = cluster.follower_client()

    leader.create_table("accounts")
    leader.upsert("accounts", "alice", "100")

    follower.wait_for_value("accounts", "alice", "100")


def test_follower_rejects_writes(make_cluster):
    cluster = make_cluster(followers=1)
    follower = cluster.follower_client()

    with pytest.raises(grpc.RpcError) as rejected:
        follower.upsert("accounts", "alice", "100")
    assert rejected.value.code() == grpc.StatusCode.FAILED_PRECONDITION


def test_follower_serves_reads(make_cluster):
    cluster = make_cluster(followers=1)
    leader = cluster.leader_client
    follower = cluster.follower_client()

    leader.create_table("accounts")
    balances = {f"user{number}": str(number * 100) for number in range(5)}
    for owner, balance in balances.items():
        leader.upsert("accounts", owner, balance)

    follower.wait_for_value("accounts", "user4", "400")
    records, _ = follower.scan("accounts")
    assert {record.key: record.value for record in records} == balances


def test_updates_and_deletes_replicate(make_cluster):
    cluster = make_cluster(followers=1)
    leader = cluster.leader_client
    follower = cluster.follower_client()

    leader.create_table("accounts")
    leader.upsert("accounts", "alice", "100")
    follower.wait_for_value("accounts", "alice", "100")

    leader.upsert("accounts", "alice", "250")
    follower.wait_for_value("accounts", "alice", "250")

    leader.delete("accounts", "alice")
    from sdb_testkit import wait_until

    wait_until(
        lambda: follower.get("accounts", "alice") is None,
        message="delete did not replicate to follower",
    )


def test_multiple_followers_converge(make_cluster):
    cluster = make_cluster(followers=2)
    leader = cluster.leader_client

    leader.create_table("accounts")
    leader.upsert("accounts", "alice", "100")

    for follower_index in range(2):
        cluster.follower_client(follower_index).wait_for_value(
            "accounts", "alice", "100"
        )
