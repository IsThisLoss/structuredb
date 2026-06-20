# Replication (operations guide)

StructureDB supports **asynchronous, leader → follower** replication by shipping
WAL pages. A leader accepts writes; followers are read-only and continuously
stream and apply the leader's write-ahead log.

## Configuration

Add a `replication` section to `config.yaml`.

Leader:

```yaml
replication:
  role: leader
  poll_interval: 200   # ms between WAL re-scans when followers are caught up
```

Follower:

```yaml
replication:
  role: follower
  leader_address: "127.0.0.1:50051"   # leader's gRPC host:port
```

A follower:

- streams WAL pages from the leader, mirrors them into its own `wal/` directory
  and applies them through the normal recovery path;
- rejects `Upsert`, `Delete`, `CreateTable` and `DropTable` with
  `FAILED_PRECONDITION` ("replica is read-only");
- still serves `Lookup` / `Scan` locally;
- resumes from the first locally-incomplete WAL segment after a restart, so it
  catches up on anything written while it was offline.

DDL replicates over the same stream: a table created on the leader is
materialized on the follower the first time one of its rows is applied.

## Guarantees

Replication is **asynchronous**: a write is acknowledged to the client before it
is shipped. On leader loss the unshipped WAL tail can be lost, and followers are
eventually consistent. There is no automatic failover or quorum.

## WAL retention

The leader retains any persisted WAL segment still needed by a **connected**
follower (tracked per stream). When no follower is connected, WAL cleaning is
unconstrained — a follower that has been disconnected long enough for its
segments to be cleaned must be re-bootstrapped from a snapshot (below).

## Bootstrapping a follower

A fresh follower with an empty data directory simply streams from segment 0,
provided the leader still retains that far back. If the leader has already
cleaned the early WAL (data compacted into SSTables), seed the follower from a
copy of the leader's data directory:

1. Stop the follower (if running).
2. Copy the leader's `root` directory to the follower's `root`
   (SSTable dirs + `wal/`). Prefer a quiet moment; applies are idempotent, so a
   slightly inconsistent copy is reconciled by the subsequent WAL stream as long
   as the copy is newer than the leader's retained WAL floor.
3. Start the follower. `DetectResumeSegment` picks the first incomplete segment
   in the copied `wal/` and resumes streaming from there.

## Manual promotion (follower → leader)

A follower already holds a complete node state (mirrored WAL + SSTables), so
promotion is a config change:

1. (Optional) Confirm the follower has caught up.
2. Stop the follower.
3. Edit its `config.yaml`: set `replication.role: leader` and drop
   `leader_address`.
4. Restart it. Its WAL writer continues numbering after the mirrored segments,
   and it begins serving writes and the `Replication` stream.
5. Repoint clients and any remaining followers at the new leader.

## Quick local two-node demo

```bash
# leader on :50051, follower on :50052 (see config examples above)
./build/Debug/src/server/structuredb-server --config=leader.yaml &
./build/Debug/src/server/structuredb-server --config=follower.yaml &

CLI=./build/Debug/src/cli/structuredb-cli
$CLI --target=127.0.0.1:50051 --c="CREATE TABLE t"
$CLI --target=127.0.0.1:50051 --c="UPSERT t hello world"
sleep 1
$CLI --target=127.0.0.1:50052 --c="SCAN t"          # replicated
$CLI --target=127.0.0.1:50052 --c="UPSERT t a b"    # rejected: read-only
```
