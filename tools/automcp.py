#!/usr/bin/env python3
"""Shim so ``python tools/automcp.py ...`` works the same as ``python -m tools.automcp ...``."""
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.automcp.__main__ import main  # noqa: E402

if __name__ == "__main__":
    raise SystemExit(main())
