#!/usr/bin/env python3
"""``python tools/dist.py ...`` を ``python -m tools.dist ...`` と同じように動かすためのシム。"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.dist.__main__ import main  # noqa: E402

if __name__ == "__main__":
    raise SystemExit(main())
