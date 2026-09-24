#!/usr/bin/env python3
"""``python tools/engine_dist.py ...`` を ``python -m tools.engine_dist ...`` と同じように動かすためのシム。"""

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))

from tools.engine_dist.__main__ import main  # noqa: E402

if __name__ == "__main__":
    raise SystemExit(main())
