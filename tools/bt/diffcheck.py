"""Structural comparison helpers for the round-trip self-test."""

from __future__ import annotations

from typing import Any

from .cereal_json import Num, OrderedObj


class SemanticMismatch(AssertionError):
    pass


def _kind(v: Any) -> str:
    if isinstance(v, bool):
        return "bool"
    if isinstance(v, Num):
        return "int" if v.is_int else "float"
    if isinstance(v, OrderedObj):
        return "obj"
    if isinstance(v, list):
        return "list"
    if isinstance(v, str):
        return "str"
    if v is None:
        return "null"
    return type(v).__name__


def assert_semantically_equal(a: Any, b: Any, path: str = "$") -> None:
    ka, kb = _kind(a), _kind(b)
    if ka != kb:
        raise SemanticMismatch(f"{path}: kind {ka} != {kb}")
    if ka == "obj":
        keys_a, keys_b = a.keys(), b.keys()
        if keys_a != keys_b:
            only_a = [k for k in keys_a if k not in keys_b]
            only_b = [k for k in keys_b if k not in keys_a]
            raise SemanticMismatch(
                f"{path}: key order/set differs\n"
                f"    expected {keys_a}\n    got      {keys_b}\n"
                f"    only-expected {only_a}  only-got {only_b}"
            )
        for k in keys_a:
            assert_semantically_equal(a[k], b[k], f"{path}.{k}")
    elif ka == "list":
        if len(a) != len(b):
            raise SemanticMismatch(f"{path}: list len {len(a)} != {len(b)}")
        for i, (x, y) in enumerate(zip(a, b)):
            assert_semantically_equal(x, y, f"{path}[{i}]")
    elif ka in ("int", "float"):
        if a.value != b.value:
            raise SemanticMismatch(f"{path}: number {a.render()} != {b.render()}")
    else:
        if a != b:
            raise SemanticMismatch(f"{path}: {a!r} != {b!r}")


def _collect(node: Any, path: str, out: dict) -> None:
    if isinstance(node, OrderedObj):
        for k, v in node.items():
            if k == "polymorphic_id":
                out["poly"].append((path, v.value if isinstance(v, Num) else v))
            elif k == "cereal_class_version":
                out["ver"].append((path, v.value if isinstance(v, Num) else v))
            elif k == "id" and path.endswith(".ptr_wrapper"):
                out["ptr"].append((path, v.value if isinstance(v, Num) else v))
            _collect(v, f"{path}.{k}", out)
    elif isinstance(node, list):
        for i, v in enumerate(node):
            _collect(v, f"{path}[{i}]", out)


def assert_bookkeeping_equal(a: Any, b: Any) -> None:
    ca: dict = {"poly": [], "ptr": [], "ver": []}
    cb: dict = {"poly": [], "ptr": [], "ver": []}
    _collect(a, "$", ca)
    _collect(b, "$", cb)
    for kind in ("poly", "ptr", "ver"):
        if ca[kind] != cb[kind]:
            # first divergence
            for i, (x, y) in enumerate(zip(ca[kind], cb[kind])):
                if x != y:
                    raise SemanticMismatch(
                        f"bookkeeping[{kind}] diverges at #{i}:\n"
                        f"    expected {x}\n    got      {y}"
                    )
            raise SemanticMismatch(
                f"bookkeeping[{kind}] length {len(ca[kind])} != {len(cb[kind])}"
            )
