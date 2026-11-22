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
  requests that segment again after reconnecting.

DDL replicates over the same stream: a table created on the leader is
materialized on the follower the first time one of its rows is applied.

## Guarantees

Replication is **asynchronous**: a write can be acknowledged to the client
before it is shipped. On leader loss the unshipped WAL tail can be lost. There
is no automatic failover or quorum.

The replication protocol currently has no cluster ID, node ID, history or
leader epoch, version negotiation, record checksum, or durable follower
acknowledgement. It does not verify that a follower has joined the intended
cluster or history, or expose separate received, durable, and applied
positions. Operators must not assume that reconnecting to another leader is
safe, or use the stream's sent position as evidence that a follower has
persisted or applied data. Replication does not currently provide a
crash-consistent snapshot or a verified promotion procedure.

The wire position is an inclusive `(segment_no, page_no)` pair; only
nonnegative segment numbers and page zero are supported because the current WAL
uses one page per segment. A stream sends the raw page bytes, including a
possibly growing partial tail, and may send the same partial page again.
These bytes use the existing WAL event encoding, with zero padding when a page
is filled; there is no independent record framing or integrity check on the
wire. The follower overwrites and fsyncs each received page before applying its
events. Reconnect starts at the first local missing or incomplete segment,
which may replay a partial page. This is not an acknowledged applied-position
checkpoint, and does not guarantee correct replay under every crash or storage
failure.

## WAL retention

The leader's WAL cleaner considers the positions of **connected** followers
when deciding which persisted segments to retain. Those positions reflect sent
pages, not follower acknowledgements, and the cleaner's retention decision is
not synchronized with new streams. When no follower is connected, WAL cleaning
is unconstrained. A follower that falls behind retention needs a new bootstrap,
which is not yet supported.

If a requested segment is absent while a later segment is retained, the leader
terminates the RPC with `FAILED_PRECONDITION` and `snapshot required`; it does
not stream past the gap. A request with a negative segment or a nonzero page is
rejected with `INVALID_ARGUMENT`. The current implementation cannot identify a
missing segment when no later segment remains, or distinguish a request beyond
the leader's end from a follower waiting for future data. There is no
automatic snapshot transfer.

## Bootstrapping a follower

A fresh follower with an empty data directory requests segment 0. This works
only while the leader retains the required history. There is currently no
supported snapshot/bootstrap procedure for a follower that has fallen behind
WAL retention. In particular, copying a live data directory is not a
consistent snapshot, and copying a directory after WAL cleanup does not supply
a verified resume position. Stop and investigate a `snapshot required` error;
do not rely on the follower being complete.

## Manual promotion (follower → leader)

There is no verified promotion or history-fencing protocol. A configuration
change alone cannot establish that the follower has every acknowledged write
or prevent another leader from accepting writes. Promotion and repointing
followers can lose data or join incompatible histories.

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
