"""Leader -> follower replication scenarios.

These use the ``make_cluster`` fixture, which spins up a leader plus one or more
followers wired to stream its WAL. Replication is asynchronous, so reads on the
follower are polled with ``wait_for_value`` / ``wait_until``.
"""

from __future__ import annotations

from pathlib import Path

import grpc
import pytest

pytestmark = [pytest.mark.replication, pytest.mark.slow]


@pytest.mark.parametrize("segment,page", [(-1, 0), (0, -1), (0, 1)])
def test_invalid_replication_position_is_rejected(server, segment, page):
    from sdb_testkit import proto_gen

    pb = proto_gen.load()
    with grpc.insecure_channel(server.target) as channel:
        stub = pb.repl_grpc.ReplicationStub(channel)
        request = pb.repl_pb2.GetEventsRequest()
        position = getattr(request, "from")
        position.segment_no = segment
        position.page_no = page
        with pytest.raises(grpc.RpcError) as rejected:
            next(stub.GetEvents(request, timeout=5))
        assert rejected.value.code() == grpc.StatusCode.INVALID_ARGUMENT


@pytest.mark.parametrize("start,missing", [(0, 0), (0, 1), (1, 1)])
def test_missing_wal_segment_requires_snapshot(server, start, missing):
    from sdb_testkit import proto_gen

    # Segment zero is the live writer. Simulate a retained later segment with
    # a hole in front of it; even an empty later segment proves the gap exists.
    wal_dir = Path(server.config.root) / "wal"
    (wal_dir / "0002.wal.sdb").touch()
    if missing == 0:
        (wal_dir / "0000.wal.sdb").unlink()
    pb = proto_gen.load()
    with grpc.insecure_channel(server.target) as channel:
        stub = pb.repl_grpc.ReplicationStub(channel)
        request = pb.repl_pb2.GetEventsRequest()
        getattr(request, "from").segment_no = start
        with pytest.raises(grpc.RpcError) as rejected:
            next(stub.GetEvents(request, timeout=5))
        assert rejected.value.code() == grpc.StatusCode.FAILED_PRECONDITION
        assert f"snapshot required: WAL segment {missing} is unavailable" in rejected.value.details()


def test_stream_reports_gap_discovered_after_connecting(server, client):
    from sdb_testkit import proto_gen

    client.create_table("accounts")
    client.upsert("accounts", "alice", "100")
    pb = proto_gen.load()
    with grpc.insecure_channel(server.target) as channel:
        stub = pb.repl_grpc.ReplicationStub(channel)
        stream = stub.GetEvents(pb.repl_pb2.GetEventsRequest(), timeout=5)
        assert next(stream).position.segment_no == 0
        (Path(server.config.root) / "wal" / "0002.wal.sdb").touch()
        with pytest.raises(grpc.RpcError) as rejected:
            next(stream)
        assert rejected.value.code() == grpc.StatusCode.FAILED_PRECONDITION
        assert "snapshot required: WAL segment 1 is unavailable" in rejected.value.details()


def test_stream_advances_to_next_segment_after_complete_page(server):
    from sdb_testkit import proto_gen

    wal_dir = Path(server.config.root) / "wal"
    (wal_dir / "0000.wal.sdb").write_bytes(bytes(4096))
    pb = proto_gen.load()
    with grpc.insecure_channel(server.target) as channel:
        stub = pb.repl_grpc.ReplicationStub(channel)
        stream = stub.GetEvents(pb.repl_pb2.GetEventsRequest(), timeout=5)
        first = next(stream)
        assert (first.position.segment_no, first.position.page_no) == (0, 0)
        assert len(first.data) == 4096
        (wal_dir / "0001.wal.sdb").write_bytes(b"next page")
        second = next(stream)
        assert (second.position.segment_no, second.position.page_no) == (1, 0)
        assert second.data == b"next page"
        stream.cancel()


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


def test_follower_reconnects_after_leader_restart(make_cluster):
    from sdb_testkit import wait_until

    cluster = make_cluster(followers=1)
    leader = cluster.leader_client
    follower = cluster.follower_client()
    leader.create_table("accounts")
    leader.upsert("accounts", "alice", "100")
    follower.wait_for_value("accounts", "alice", "100")

    cluster.leader.stop()
    wait_until(
        lambda: "Replication: stream ended" in cluster.followers[0].log_tail(),
        message="follower did not notice leader disconnect",
    )
    # This RPC needs the follower's event loop while its retry timer is pending.
    assert follower.get("accounts", "alice") == "100"
    cluster.leader.start()
    leader.upsert("accounts", "alice", "250")
    follower.wait_for_value("accounts", "alice", "250", timeout=20)
