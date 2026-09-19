#!/usr/bin/env python3
"""Shim so ``python tools/dist.py ...`` works the same as ``python -m tools.dist ...``."""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.dist.__main__ import main  # noqa: E402

if __name__ == "__main__":
    raise SystemExit(main())
