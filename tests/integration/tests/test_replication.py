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

    leader.create_table("t")
    leader.upsert("t", "k", "v")

    follower.wait_for_value("t", "k", "v")


def test_follower_rejects_writes(make_cluster):
    cluster = make_cluster(followers=1)
    follower = cluster.follower_client()

    with pytest.raises(grpc.RpcError) as exc_info:
        follower.upsert("any_table", "k", "v")
    assert exc_info.value.code() == grpc.StatusCode.FAILED_PRECONDITION


def test_follower_serves_reads(make_cluster):
    cluster = make_cluster(followers=1)
    leader = cluster.leader_client
    follower = cluster.follower_client()

    leader.create_table("reads")
    for i in range(5):
        leader.upsert("reads", f"k{i}", f"v{i}")

    follower.wait_for_value("reads", "k4", "v4")
    records, _ = follower.scan("reads")
    assert {r.key: r.value for r in records} == {f"k{i}": f"v{i}" for i in range(5)}


def test_updates_and_deletes_replicate(make_cluster):
    cluster = make_cluster(followers=1)
    leader = cluster.leader_client
    follower = cluster.follower_client()

    leader.create_table("t")
    leader.upsert("t", "k", "first")
    follower.wait_for_value("t", "k", "first")

    leader.upsert("t", "k", "second")
    follower.wait_for_value("t", "k", "second")

    leader.delete("t", "k")
    from sdb_testkit import wait_until

    wait_until(
        lambda: follower.get("t", "k") is None,
        message="delete did not replicate to follower",
    )


def test_multiple_followers_converge(make_cluster):
    cluster = make_cluster(followers=2)
    leader = cluster.leader_client

    leader.create_table("t")
    leader.upsert("t", "k", "shared")

    for i in range(2):
        cluster.follower_client(i).wait_for_value("t", "k", "shared")
