"""複数ノードをまとめる汎用ノード（BlackBoardGate / ActionTimeline など）を model 上で組み立てる。

移行スクリプト（tools/bt/migrations/）と selftest が使う。どれも新しい model ノードを返すだけで、
ツリーへの付け替えは呼び出し側が行う。
"""

from __future__ import annotations

from typing import Iterable, Optional

from . import catalog as catalog_mod
from . import model
from .blob import Ptr, Ver
from .cereal_json import Num, OrderedObj
from .edits import action_blob

TIMELINE = "Timeline::ActionTimeline"
RANDOM_WRITE = "Other::RandomWriteBlackBoard<Int>"
ANGLE_DISPATCH = "Basic::PlayerAngleDispatch"


def _mint() -> str:
    from . import meta as _meta
    return _meta.mint_guid()


def new_action(cat: catalog_mod.Catalog, spec: str, *, name: Optional[str] = None,
               pos=(0.0, 0.0), guid: Optional[str] = None) -> model.Action:
    entry = cat.resolve_action(spec)
    if entry is None:
        raise ValueError(f"unknown action type: {spec!r} (run regen-catalog?)")
    return model.Action(guid=guid or _mint(), pos=tuple(pos), name=name or entry["class"],
                        type_fqn=entry["fqn"], action_version=int(entry.get("version", 0)),
                        params=action_blob(cat, entry))


def gate(child=None, *, conditions: Iterable[tuple[str, int]] = (),
         writes_on_start: Iterable[tuple[str, int]] = (),
         writes_on_success: Iterable[tuple[str, int]] = (),
         once: bool = False, pos=(0.0, 0.0), guid: Optional[str] = None) -> model.BlackBoardGate:
    return model.BlackBoardGate(guid=guid or _mint(), pos=tuple(pos), child=child,
                                conditions=list(conditions),
                                writes_on_start=list(writes_on_start),
                                writes_on_success=list(writes_on_success),
                                once=once)


def nested_action(action: model.Action) -> Ptr:
    """ActionNode 直下の action を、別の action のメンバー（unique_ptr<ActionBase>）として持てる形にする。"""
    leaf = action.type_fqn.rsplit("::", 1)[-1]
    return Ptr(exact=False, null=False, fqn=action.type_fqn, wrapper="unique",
               data=Ver(("type", leaf), int(action.action_version), action.params))


def cue(at_secs: float, action: model.Action, *, wait_done: bool = False,
        keep_ticking: bool = False) -> OrderedObj:
    return OrderedObj([
        ("at_secs_", Num.of_float(float(at_secs))),
        ("waitDone_", bool(wait_done)),
        ("keepTicking_", bool(keep_ticking)),
        ("action_", nested_action(action)),
    ])


def timeline(cat: catalog_mod.Catalog, cues: list[OrderedObj], *, duration_secs: float = 0.0,
             once: bool = False, fail_on_child_failure: bool = False, name: Optional[str] = None,
             pos=(0.0, 0.0), guid: Optional[str] = None) -> model.Action:
    node = new_action(cat, TIMELINE, name=name, pos=pos, guid=guid)
    node.params["cues_"] = list(cues)
    node.params["duration_secs_"] = Num.of_float(float(duration_secs))
    node.params["failOnChildFailure_"] = bool(fail_on_child_failure)
    node.params["once_"] = bool(once)
    return node


def random_write(cat: catalog_mod.Catalog, key: str, choices: list[tuple[int, int]], *,
                 name: Optional[str] = None, pos=(0.0, 0.0)) -> model.Action:
    """``choices`` は (value, weight) の組。"""
    node = new_action(cat, RANDOM_WRITE, name=name, pos=pos)
    node.params["keyName_"] = key
    node.params["choices_"] = [OrderedObj([("value_", Num.of_int(int(v))), ("weight_", Num.of_int(int(w)))])
                               for v, w in choices]
    return node


def angle_dispatch(cat: catalog_mod.Catalog, key: str,
                   ranges: list[tuple[float, float, bool, int]], *,
                   fallback: Optional[int] = None, name: Optional[str] = None,
                   pos=(0.0, 0.0)) -> model.Action:
    """``ranges`` は (minDegree, maxDegree, useAbsolute, value)。上から順に最初に当てはまったものを書く。"""
    node = new_action(cat, ANGLE_DISPATCH, name=name, pos=pos)
    node.params["keyName_"] = key
    node.params["ranges_"] = [OrderedObj([("minDegree_", Num.of_float(float(lo))),
                                          ("maxDegree_", Num.of_float(float(hi))),
                                          ("useAbsolute_", bool(ab)),
                                          ("value_", Num.of_int(int(v)))])
                              for lo, hi, ab, v in ranges]
    node.params["useFallback_"] = fallback is not None
    node.params["fallbackValue_"] = Num.of_int(int(fallback or 0))
    return node
