"""``.enemyBehaviourData.meta`` sidecar files.

A ``.meta`` is a cereal-JSON ``std::shared_ptr<AssetBase>`` holding the
``EnemyBehaviourFile`` ScriptableObject: a stable asset ``guid_`` (what an enemy
prefab's ``behaviourData_`` field references) plus ``contentPath_`` back to the
data file. Written by ``File::OnSave()`` in the engine; reproduced here so
``new-tree`` yields an asset the editor and prefabs can bind to immediately.
"""

from __future__ import annotations

import uuid
from pathlib import Path

from .cereal_json import Num, OrderedObj, dumps, loads, read_text, to_file_bytes

ASSET_FQN = "NanamiEngine::Module::Asset::EnemyBehaviourFile"
DATA_EXT = ".enemyBehaviourData"
META_EXT = ".enemyBehaviourData.meta"
DEFAULT_DIR = "Assets/Data/EnemyBehaviour"
_ID = 0x80000001  # 2147483649 - first shared-ptr id / first polymorphic type id


def mint_guid() -> str:
    return str(uuid.uuid4()).upper()


def content_path_for(name: str, target_dir: Path, repo_root: Path) -> str:
    """Match the sibling ``.meta`` separator convention, or fall back to default.

    Observed form: backslash-separated directory, forward slash before the file,
    e.g. ``Assets\\Data\\EnemyBehaviour/Hyena.enemyBehaviourData``.
    """
    for sib in sorted(target_dir.glob("*" + META_EXT)):
        try:
            cp = read_meta(sib)["content_path"]
        except Exception:  # noqa: BLE001
            continue
        prefix = cp.rsplit("/", 1)[0] if "/" in cp else cp.rsplit("\\", 1)[0]
        return f"{prefix}/{name}{DATA_EXT}"
    try:
        rel = target_dir.resolve().relative_to(repo_root.resolve())
        dir_bs = str(rel).replace("/", "\\")
    except ValueError:
        dir_bs = DEFAULT_DIR.replace("/", "\\")
    return f"{dir_bs}/{name}{DATA_EXT}"


def render_meta(name: str, guid: str, content_path: str) -> str:
    data = OrderedObj()
    data["cereal_class_version"] = Num.of_int(0)
    data["value0"] = OrderedObj([("cereal_class_version", Num.of_int(0))])
    data["contentPath_"] = content_path
    data["guid_"] = OrderedObj([("cereal_class_version", Num.of_int(0)), ("value_", guid)])

    inner = OrderedObj()
    inner["polymorphic_id"] = Num.of_int(_ID)
    inner["polymorphic_name"] = ASSET_FQN
    inner["ptr_wrapper"] = OrderedObj([("id", Num.of_int(_ID)), ("data", data)])
    return dumps(OrderedObj([("value0", inner)]))


def write_meta(path: Path, name: str, guid: str, content_path: str) -> None:
    path.write_bytes(to_file_bytes(render_meta(name, guid, content_path)))


def read_meta(path: Path) -> dict:
    obj = loads(read_text(path))
    d = obj["value0"]["ptr_wrapper"]["data"]
    cp = d["contentPath_"]
    guid = d["guid_"]["value_"]
    fname = Path(path).name
    name = fname[: -len(META_EXT)] if fname.endswith(META_EXT) else Path(cp).name.split(".")[0]
    return {"name": name, "guid": guid, "content_path": cp}
