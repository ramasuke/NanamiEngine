"""Re-export shim: the cereal-JSON codec moved to :mod:`tools.common.cereal_json`
(it was always format-generic, not BehaviourTree-specific - ``tools.scene`` needs
it too). Every existing ``from .cereal_json import X`` / ``tools.bt.cereal_json.X``
call site keeps working unchanged.
"""

from __future__ import annotations

from tools.common.cereal_json import (
    Num,
    OrderedObj,
    dumps,
    format_number,
    loads,
    read_text,
    self_check_roundtrip,
    to_file_bytes,
    write_file,
)

__all__ = [
    "Num",
    "OrderedObj",
    "dumps",
    "format_number",
    "loads",
    "read_text",
    "self_check_roundtrip",
    "to_file_bytes",
    "write_file",
]
