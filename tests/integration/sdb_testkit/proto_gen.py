"""Generate (and import) Python gRPC stubs from the project's .proto files.

The C++ build owns ``proto/*.proto``; rather than vendoring a second copy of
generated Python, we compile the stubs on demand into a gitignored directory
and put it on ``sys.path``. Regeneration only happens when a ``.proto`` is newer
than the last successful generation, so repeated test runs are cheap.

Override locations with env vars when running outside the repo layout:

    STRUCTUREDB_PROTO_DIR   directory containing the .proto files
    STRUCTUREDB_PROTO_OUT   directory to write generated *_pb2.py into
"""

from __future__ import annotations

import os
import sys
import types
from pathlib import Path

# tests/integration/sdb_testkit/proto_gen.py -> repo root is three parents up.
_REPO_ROOT = Path(__file__).resolve().parents[3]
_INTEGRATION_DIR = Path(__file__).resolve().parents[1]


def _proto_dir() -> Path:
    env = os.environ.get("STRUCTUREDB_PROTO_DIR")
    return Path(env) if env else _REPO_ROOT / "proto"


def _out_dir() -> Path:
    env = os.environ.get("STRUCTUREDB_PROTO_OUT")
    return Path(env) if env else _INTEGRATION_DIR / ".generated"


def _needs_regen(protos: list[Path], marker: Path) -> bool:
    if not marker.exists():
        return True
    newest_proto = max(p.stat().st_mtime for p in protos)
    return marker.stat().st_mtime < newest_proto


def _generate(proto_dir: Path, out_dir: Path, protos: list[Path]) -> None:
    from grpc_tools import protoc  # imported lazily so the dep is only needed here

    out_dir.mkdir(parents=True, exist_ok=True)
    args = [
        "grpc_tools.protoc",
        f"-I{proto_dir}",
        f"--python_out={out_dir}",
        f"--grpc_python_out={out_dir}",
        *[str(p) for p in protos],
    ]
    rc = protoc.main(args)
    if rc != 0:
        raise RuntimeError(f"protoc failed (exit {rc}) for {[p.name for p in protos]}")
    (out_dir / ".gen_ok").touch()


_loaded: types.SimpleNamespace | None = None


def load() -> types.SimpleNamespace:
    """Ensure stubs exist, import them, and return a namespace of the modules.

    Returns a ``SimpleNamespace`` with: ``table_pb2``, ``table_grpc``,
    ``tx_pb2``, ``tx_grpc``, ``repl_pb2``, ``repl_grpc``.
    """
    global _loaded
    if _loaded is not None:
        return _loaded

    proto_dir = _proto_dir()
    out_dir = _out_dir()
    protos = sorted(proto_dir.glob("*.proto"))
    if not protos:
        raise FileNotFoundError(f"no .proto files found under {proto_dir}")

    if _needs_regen(protos, out_dir / ".gen_ok"):
        _generate(proto_dir, out_dir, protos)

    # Generated *_pb2_grpc.py use top-level imports (``import table_service_pb2``),
    # so the output directory itself must be importable as a search path.
    if str(out_dir) not in sys.path:
        sys.path.insert(0, str(out_dir))

    import importlib

    table_pb2 = importlib.import_module("table_service_pb2")
    table_grpc = importlib.import_module("table_service_pb2_grpc")
    tx_pb2 = importlib.import_module("transaction_service_pb2")
    tx_grpc = importlib.import_module("transaction_service_pb2_grpc")
    repl_pb2 = importlib.import_module("replication_service_pb2")
    repl_grpc = importlib.import_module("replication_service_pb2_grpc")

    _loaded = types.SimpleNamespace(
        table_pb2=table_pb2,
        table_grpc=table_grpc,
        tx_pb2=tx_pb2,
        tx_grpc=tx_grpc,
        repl_pb2=repl_pb2,
        repl_grpc=repl_grpc,
    )
    return _loaded
