#!/usr/bin/env python3
"""ctest entry point for the Python integration suite.

Creates (and caches) a local virtualenv, installs the pinned requirements into
it, points the suite at the built server binary and runs pytest. Designed to be
invoked by CMake/ctest, but also usable by hand:

    python3 run_tests.py --server-binary path/to/structuredb-server -- -k replication
"""

from __future__ import annotations

import argparse
import hashlib
import os
import subprocess
import sys
import venv
from pathlib import Path

HERE = Path(__file__).resolve().parent
REQUIREMENTS = HERE / "requirements.txt"


def _venv_python(venv_dir: Path) -> Path:
    if os.name == "nt":  # pragma: no cover - posix CI
        return venv_dir / "Scripts" / "python.exe"
    return venv_dir / "bin" / "python"


def ensure_venv(venv_dir: Path) -> Path:
    python = _venv_python(venv_dir)
    if not python.exists():
        print(f"[integration] creating venv at {venv_dir}", flush=True)
        venv.EnvBuilder(with_pip=True).create(venv_dir)
    return python


def ensure_deps(python: Path, venv_dir: Path) -> None:
    """Install requirements only when they change (hash marker)."""
    digest = hashlib.sha256(REQUIREMENTS.read_bytes()).hexdigest()
    marker = venv_dir / ".deps.sha256"
    if marker.exists() and marker.read_text().strip() == digest:
        return
    print("[integration] installing python dependencies", flush=True)
    subprocess.check_call(
        [str(python), "-m", "pip", "install", "-q", "--disable-pip-version-check",
         "--upgrade", "pip"]
    )
    subprocess.check_call(
        [str(python), "-m", "pip", "install", "-q", "--disable-pip-version-check",
         "-r", str(REQUIREMENTS)]
    )
    marker.write_text(digest)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--server-binary", required=True,
                        help="path to the built structuredb-server")
    parser.add_argument("--venv", default=str(HERE / ".venv"),
                        help="virtualenv location (default: tests/integration/.venv)")
    parser.add_argument("pytest_args", nargs="*",
                        help="extra args forwarded to pytest (after --)")
    args = parser.parse_args()

    server_binary = Path(args.server_binary).resolve()
    if not server_binary.is_file():
        print(f"[integration] server binary not found: {server_binary}", file=sys.stderr)
        return 2

    venv_dir = Path(args.venv).resolve()
    python = ensure_venv(venv_dir)
    ensure_deps(python, venv_dir)

    env = os.environ.copy()
    env["STRUCTUREDB_SERVER_BIN"] = str(server_binary)
    env.setdefault("STRUCTUREDB_PROTO_DIR", str((HERE / ".." / ".." / "proto").resolve()))

    cmd = [str(python), "-m", "pytest", str(HERE / "tests")]
    cmd += args.pytest_args
    print(f"[integration] running: {' '.join(cmd)}", flush=True)
    return subprocess.call(cmd, cwd=str(HERE), env=env)


if __name__ == "__main__":
    raise SystemExit(main())
