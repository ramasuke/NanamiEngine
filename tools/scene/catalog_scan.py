"""登録済み Component ヘッダを読み取って ``tools/scene/catalog.json`` を構築する。

正規表現/波括弧マッチングによるスキャナ - libclang なし - で、``tools.bt.catalog_scan``
と同じ方式。分類できないものは shape ``"unknown"`` として記録する。これは
``add-component``/``set-component-params`` がそのフィールドを名前で扱えるかどうかに
影響するだけで、往復の忠実性には一切影響しない (``reader.py``/``writer.py`` は
このカタログとは無関係に、全コンポーネントのデータを不透明な blob としてタグ付けする)。

対象範囲 (v1): ``ENGINE_REGISTER_COMPONENT`` マクロで登録されたコンポーネント -
``ComponentBase`` の **直接の** サブクラス - だけが ``add-component`` で追加可能として
カタログ化される (``Engine/Module/**``、``Packages/**``、``Assets/Scripts/**`` に
またがる約66ヘッダで確認済み)。中間の基底を持つコンポーネント (例:
``GamePlay::Npc::Enemy::Hyena : EnemyBase : ComponentBase``。マクロを使わず
``EnemyBase`` に対して cereal の3つのマクロを手で登録している) も tagged-blob の
reader/writer でロスなく往復するが、``add-component --type`` の対象には出さない。
こうしたゲームプレイ用スクリプトのコンポーネントは通常、任意の GameObject に
単体で付けるのではなく、プレハブ全体をコピーして (``instantiate-prefab``) 導入するため。

別の話として、``ENGINE_REGISTER_COMPONENT`` のポリモーフィック *登録* は常に
``ComponentBase`` を対象とする (マクロにハードコードされている) が、コンポーネントの
``save()`` は通常 **複数の** 基底クラスを、自身のフィールドより前にそれぞれ名前なしの
位置スロット (``value0``, ``value1``, ...) としてアーカイブする - 例えば
``ImageRenderer`` は ``ComponentBase`` (``value0``)、次に空のライフサイクル mixin
``IInitRenderable`` (``value1``) と ``IUserInterfaceRenderable`` (``value2``)、
その後 ``spriteFile_`` を書く。cereal はこれらのスロットを *位置で* 読むので、
ゼロから作るインスタンスはそのすべてを再現しなければならない。各エントリの
``bases`` はすべての ``cereal::base_class<X>(this)`` 呼び出しをアーカイブ順に、
占める ``valueN`` キーとともに列挙する (``immediate_base`` は互換性のため
``bases[0]`` として残す)。トップレベルの ``bases`` テーブルは基底の leaf ごとに、
自身の ``save()`` 本体が空か (``edits.add_component`` が空スロットとして出力できる
純粋なマーカー mixin か) と、その ``CEREAL_CLASS_VERSION`` を記録する。
独自の実シリアライズフィールドを持つ基底 (``mass_``/``isGravity_``/... を持つ
``ColliderBase`` 経由のすべての Collider、および ``NetworkComponent``/``EnemyBase``) は
(まだ) ``params`` に平坦化されていない: 往復はするが個別には設定できず、
エンジンが必要とするフィールドを欠いた構造体を出力しないよう、
``edits.add_component`` はそのような型の新規インスタンスをゼロから構築することを拒否する。
"""

from __future__ import annotations

import json
import re
import subprocess
from pathlib import Path
from typing import Any, Optional

_REPO = Path(__file__).resolve().parents[2]
CATALOG_PATH = Path(__file__).with_name("catalog.json")

COMPONENT_BASE_FQN = "NanamiEngine::Module::Component::ComponentBase"

# 広めのスキャンルート - マクロ名による事前フィルタ (scan() を参照) で安く済み、
# 同梱のサードパーティヘッダ (cereal, ImGui, DxLib, Jolt) はこのエンジン固有の
# マクロを参照しないので自然に除外される。
SCAN_ROOTS = ["Engine", "Packages", "Assets/Scripts"]

# このファイルはマクロを *定義* するだけ (`#define ENGINE_REGISTER_COMPONENT(TYPE,
# VERSION)`) - その引数リスト `(TYPE, VERSION)` を放っておくと、"TYPE" という
# 名前の型の偽の登録としてパースされてしまう。
_DEFINITION_FILE = "Engine/Module/Component/ComponentBase.h"

# 登録はコンポーネントの .cpp に `ENGINE_REGISTER_COMPONENT(T);` として置かれ、
# `CEREAL_CLASS_VERSION(T, V)` はヘッダに残る (T をシリアライズする場所すべてから
# 見える必要があるため)。旧来のヘッダ形式 `(T, V)` も引き続き受け付ける。
RE_ENGINE_REGISTER = re.compile(
    r"ENGINE_REGISTER_COMPONENT\s*\(\s*([\w:]+)\s*(?:,\s*(\d+)\s*)?\)"
)
RE_SAVE = re.compile(r"\bvoid\s+save\s*\(\s*Archive\s*&\s*\w+\s*,")
# archive(...) 呼び出し1つ分: base_class<X>(this) ラッパー、CEREAL_NVP(member)、
# または素の (名前なし) メンバー - 3つとも位置による "valueN" 配置に関係する。
RE_ARCHIVE_CALL = re.compile(
    r"archive\s*\(\s*(?:"
    r"cereal::base_class\s*<\s*(?P<base>[\w:]+)\s*>\s*\(\s*this\s*\)"
    r"|CEREAL_NVP\s*\(\s*(?P<nvp>\w+)\s*\)"
    r"|(?P<bare>[a-zA-Z_]\w*)"
    r")\s*\)"
)
RE_CLASS_VERSION = re.compile(r"CEREAL_CLASS_VERSION\s*\(\s*([\w:]+)\s*,\s*(\d+)\s*\)")
RE_FIELD_MACRO = re.compile(r"FIELD\s*\(\s*([\w:]+)\s*\)")
RE_FIELD_TMPL = re.compile(r"\bField\s*<\s*([\w:]+)\s*>")
RE_REACTIVE = re.compile(r"\bSerializableReactiveProperty\s*<\s*(.+)>")
RE_VECTOR = re.compile(r"std::vector\s*<\s*([\w:<>\s]+?)\s*>")
RE_ENUM = re.compile(r"\benum\s+(?:class\s+|struct\s+)?(\w+)\s*(?::\s*[\w:\s]+?)?\s*\{")
RE_RECORD = re.compile(r"(\benum\s+)?\b(?:class|struct)\s+(\w+)\b[^;{]*\{")
RE_ENUM_VALUE_EXPR = re.compile(r"^[\w\s()<>|+\-]+$")


def _leaf(name: str) -> str:
    return name.split("<", 1)[0].strip().rsplit("::", 1)[-1]


def _git_head() -> str:
    try:
        return subprocess.run(
            ["git", "rev-parse", "HEAD"], cwd=_REPO, capture_output=True, text=True, check=True
        ).stdout.strip()
    except Exception:  # noqa: BLE001
        return "unknown"


def _read(path: Path) -> str:
    raw = path.read_bytes()
    if raw[:3] == b"\xef\xbb\xbf":
        raw = raw[3:]
    for enc in ("utf-8", "cp932", "latin-1"):
        try:
            return raw.decode(enc)
        except UnicodeDecodeError:
            continue
    return raw.decode("utf-8", "replace")


def _balanced_block(text: str, open_idx: int) -> str:
    """``open_idx`` の位置 (またはその後) から始まる ``{...}`` ブロックを返す (波括弧を含む)。"""
    i = text.find("{", open_idx)
    if i < 0:
        return ""
    depth = 0
    for j in range(i, len(text)):
        c = text[j]
        if c == "{":
            depth += 1
        elif c == "}":
            depth -= 1
            if depth == 0:
                return text[i : j + 1]
    return text[i:]


def _strip_comments(text: str) -> str:
    text = re.sub(r"/\*.*?\*/", "", text, flags=re.S)
    text = re.sub(r"//[^\n]*", "", text)
    return text


def _find_class_body(text: str, leaf: str) -> str:
    """``class <leaf>`` の *定義* の ``{...}`` 本体。前方宣言 (``class X;`` /
    ``friend class X;``) はどの ``{`` よりも前に ``;`` があるのでスキップする。
    そうしないと後続の無関係なブロックを拾ってしまう (``ColliderBase``/``ComponentBase``
    を前方宣言しているだけのヘッダで、基底型スキャンが拾うのがまさにそれ)。"""
    for m in re.finditer(r"\bclass\s+(\w+)\b", text):
        if m.group(1) != leaf:
            continue
        brace = text.find("{", m.end())
        if brace < 0:
            return ""
        if ";" in text[m.end():brace]:
            continue
        return _balanced_block(text, m.end())
    return ""


def _enumerators(body: str) -> Optional[dict[str, int]]:
    """1つの enum 本体の ``{name: value}``。値がリテラル / 先行する列挙子を
    ``<< | + -`` で組み合わせたもの以外なら ``None``。"""
    values: dict[str, int] = {}
    nxt = 0
    for item in _strip_comments(body).split(","):
        item = item.strip("{} \t\r\n")
        if not item:
            continue
        name, _, expr = item.partition("=")
        name, expr = name.strip(), expr.strip()
        if not re.fullmatch(r"\w+", name):
            return None
        if expr:
            if not RE_ENUM_VALUE_EXPR.match(expr):
                return None
            expr = re.sub(r"(?<=\d)[uUlL]+\b", "", expr)
            try:
                nxt = int(eval(expr, {"__builtins__": {}}, dict(values)))
            except Exception:
                return None
        values[name] = nxt
        nxt += 1
    return values


def _scan_enums(headers: list[tuple[str, str]]) -> dict[str, Optional[dict[str, int]]]:
    """スキャンしたヘッダで宣言された全 enum の leaf -> その列挙子
    (パースできない場合、または同じ leaf が異なる値で複数回宣言されている場合は
    ``None``)。cereal は enum を基底の整数としてアーカイブするので、enum メンバーは
    ``int`` パラメータになる。どこかでクラス/構造体名でもある leaf は除外する
    - メンバーがどちらともとれるため。"""
    enums: dict[str, Optional[dict[str, int]]] = {}
    records: set[str] = set()
    for _rel, text in headers:
        stripped = _strip_comments(text)
        records.update(m.group(2) for m in RE_RECORD.finditer(stripped) if not m.group(1))
        for m in RE_ENUM.finditer(stripped):
            leaf = m.group(1)
            values = _enumerators(_balanced_block(stripped, m.end() - 1))
            if leaf in enums and enums[leaf] != values:
                values = None
            enums[leaf] = values
    return {k: v for k, v in enums.items() if k not in records}


def _classify_member(decl_type: str, known_types: set[str],
                     enums: Optional[dict[str, Optional[dict[str, int]]]] = None) -> dict:
    t = decl_type.strip()
    # NOTE: R4::SerializableReactiveProperty<T> は {"value": T} として保存される
    mr = RE_REACTIVE.search(t)
    if mr:
        info = _classify_member(mr.group(1), known_types, enums)
        info["reactive"] = True
        return info
    m = RE_FIELD_MACRO.search(t) or RE_FIELD_TMPL.search(t)
    if m:
        return {"shape": "field", "type": _leaf(m.group(1))}
    if "glm::vec3" in t:
        return {"shape": "vec3"}
    if "glm::vec2" in t:
        return {"shape": "vec2"}
    if "glm::quat" in t:
        return {"shape": "quat"}
    if "std::string" in t:
        return {"shape": "string"}
    if _leaf(t) == "Color32":
        return {"shape": "color32"}
    mv = RE_VECTOR.search(t)
    if mv:
        return {"shape": "vector", "elem": _leaf(mv.group(1))}
    if re.search(r"\bbool\b", t):
        return {"shape": "bool"}
    if re.search(r"\b(float|double)\b", t):
        return {"shape": "float"}
    if re.search(r"\b(int|short|long|unsigned)\b", t) or re.search(
        r"std::u?int\d+_t|std::size_t|size_t|std::uint32_t", t
    ):
        return {"shape": "int"}
    leaf = _leaf(t)
    if enums and leaf in enums:
        info: dict[str, Any] = {"shape": "int", "enum": leaf}
        if enums[leaf] is not None:
            info["values"] = enums[leaf]
        return info
    if leaf in known_types:
        return {"shape": "nested", "type": leaf}
    return {"shape": "unknown", "type": leaf or t}


def _parse_serializable(body: str, known_types: set[str],
                        enums: Optional[dict[str, Optional[dict[str, int]]]] = None
                        ) -> tuple[list[dict], list[dict], bool]:
    """クラス本体の ``save()`` から ``(params, bases, interleaved)`` を返す。

    ``bases`` はすべての ``archive(cereal::base_class<X>(this))`` 呼び出しを
    アーカイブ順に ``{"leaf": X, "key": "valueN"}`` として並べたもの。cereal は
    *名前なし* ノードに、名前なしノードだけが進めるオブジェクトごとのカウンタから
    ``value<N>`` と名付ける (``JSONOutputArchive::writeName``)。そのため基底スロットは
    ``value0..value{n-1}`` を占め、``CEREAL_NVP`` なしでアーカイブされたメンバーは
    最後の基底の後から同じカウントを続ける - これが ``params`` の番号が固定の 1 では
    なく ``len(bases)`` から始まる理由。``interleaved`` は base_class 呼び出しが
    メンバーの *後* に現れた場合に True (現状そういう例はない。
    ``edits.new_component`` は配置を推測せずそのような型を拒否する)。

    ``ComponentBase`` と、自身の ``save()`` 本体が空の基底 (:func:`_scan_bases` を参照)
    だけが ``edits.add_component`` でゼロから構築できる。独自の実フィールドを持つ基底
    (``ColliderBase`` 経由のすべての Collider) もロスなく往復する (``reader.py`` が
    不透明なネスト blob としてタグ付けする) が、エンジンが読めない構造体を作る危険を
    冒さないよう、そこでは拒否される。
    """
    body = _strip_comments(body)
    m = RE_SAVE.search(body)
    if not m:
        return [], [], False
    save_block = _balanced_block(body, m.end())

    bases: list[dict] = []
    order: list[tuple[str, bool]] = []
    interleaved = False
    for call in RE_ARCHIVE_CALL.finditer(save_block):
        base = call.group("base")
        if base:
            if order:
                interleaved = True
            bases.append({"leaf": _leaf(base), "key": f"value{len(bases)}"})
            continue
        named, bare = call.group("nvp"), call.group("bare")
        member = named or bare
        if not member or member == "this" or member.startswith("cereal"):
            continue
        order.append((member, bool(named)))

    _KEYWORDS = {"return", "const", "static", "virtual", "auto", "explicit",
                "constexpr", "inline", "mutable", "public", "private"}
    type_pat_tmpl = (
        r"(\[\[serialize\(\d+\)\]\]\s*)?"
        r"((?:FIELD\s*\([^)]*\)|Field\s*<[^>]*>|[\w:<>\s\*&])+?)\s+"
        r"MEMBER" + r"\s*(?:=|;|\{|\()"
    )

    def _decl_type(member: str) -> str:
        pat = re.compile(type_pat_tmpl.replace("MEMBER", re.escape(member)))
        best, best_score = "", -1
        for mm in pat.finditer(body):
            t = mm.group(2).strip()
            leaf = t.split()[-1] if t.split() else t
            score = 0
            if mm.group(1):
                score += 3
            if t and leaf not in _KEYWORDS and t not in _KEYWORDS:
                score += 2
            if "(" in t or "<" in t:
                score += 1
            if score > best_score:
                best, best_score = t, score
        return best

    params: list[dict] = []
    positional = len(bases)  # valueN カウンタは基底スロットの後から続く
    for member, named in order:
        info = _classify_member(_decl_type(member), known_types, enums)
        if named:
            key = member
        else:
            key = f"value{positional}"
            positional += 1
        params.append({"key": key, "member": member, "named": named, **info})
    return params, bases, interleaved


def _scan_bases(leaves: set[str], headers: list[tuple[str, str]]) -> dict[str, dict]:
    """いずれかのコンポーネントがアーカイブする全基底 leaf の定義ヘッダを探し、
    ``edits.add_component`` がそのスロットをゼロから出力するのに必要な情報を記録する:
    基底自身の ``save()`` 本体が **空** か (``IInitRenderable`` のような純粋なマーカー
    mixin - 素の ``{"cereal_class_version": N}`` オブジェクトとして書いて安全) と、その
    ``CEREAL_CLASS_VERSION`` (マクロがなければ 0 - cereal の ``detail::Version<T>`` の既定値)。
    ``fqn`` はそのマクロが名前を示している場合のみ分かる。*異なる* ``save()`` 本体で
    複数のヘッダに定義された leaf は ``ambiguous`` とし、エディタが安全側に失敗するようにする。
    """
    out: dict[str, dict] = {}
    sigs: dict[str, str] = {}
    for rel, text in headers:
        for leaf in leaves:
            if leaf not in text or not re.search(r"\bclass\s+" + re.escape(leaf) + r"\b", text):
                continue
            body = _find_class_body(text, leaf)
            if not body:
                continue
            stripped = _strip_comments(body)
            sm = RE_SAVE.search(stripped)
            if not sm:
                continue
            save_block = _balanced_block(stripped, sm.end())
            signature = re.sub(r"\s+", " ", save_block).strip()
            fqn, version = None, 0
            for vm in RE_CLASS_VERSION.finditer(text):
                if _leaf(vm.group(1)) == leaf:
                    fqn, version = vm.group(1), int(vm.group(2))
                    break
            if leaf in out:
                if sigs[leaf] != signature:
                    out[leaf]["ambiguous"] = True
                continue
            # 空でない基底は自身の基底クラスをアーカイブしうる
            # (ColliderBase も NetworkComponent も base_class<ComponentBase> から始まる)。
            # その連鎖を記録する: cereal の型ごと1回のバージョン管理を再現する側は
            # そこへ入り込まないと、どの ComponentBase がファイル内で最初かを取り違える。
            _own_params, own_bases, _own_interleaved = _parse_serializable(body, leaves)
            out[leaf] = {
                "fqn": fqn,
                "header": rel,
                "version": version,
                "empty": re.search(r"\barchive\s*\(", save_block) is None,
                "bases": own_bases,
            }
            sigs[leaf] = signature
    return out


def _iter_header_files() -> list[Path]:
    out: list[Path] = []
    for root in SCAN_ROOTS:
        root_path = _REPO / root
        if not root_path.exists():
            continue
        out.extend(root_path.rglob("*.h"))
    return out


def scan() -> dict[str, Any]:
    # FQN (常に一意) をキーにする - tools.bt のアクションと違い、コンポーネントには
    # 別の「エディタ表示名」マクロがなく、実際に2つの leaf 名が衝突する
    # (SceneContextBase、StatusPresenter はそれぞれ異なる FQN を2つ持つ) ので、
    # 素の leaf は主キーにできない。下の `by_leaf` は leaf をそれを使う全 FQN に
    # 対応付け、名前解決が曖昧なときに安全側に失敗できるようにする。
    components: dict[str, dict] = {}
    by_leaf: dict[str, list[str]] = {}
    known_leaves: set[str] = set()

    matches: list[tuple[Path, str, str, str, int]] = []  # path, rel, text, fqn, version
    all_headers: list[tuple[str, str]] = []  # rel, text - 基底型スキャン用
    for path in _iter_header_files():
        rel = path.relative_to(_REPO).as_posix()
        text = _read(path)
        all_headers.append((rel, text))
        if rel == _DEFINITION_FILE:
            continue
        cpp = path.with_suffix(".cpp")
        cpp_text = _read(cpp) if cpp.exists() else ""
        if "ENGINE_REGISTER_COMPONENT" not in text and "ENGINE_REGISTER_COMPONENT" not in cpp_text:
            continue
        header_versions = {vm.group(1): int(vm.group(2)) for vm in RE_CLASS_VERSION.finditer(text)}
        for m in RE_ENGINE_REGISTER.finditer(text + "\n" + cpp_text):
            fqn = m.group(1)
            if m.group(2) is not None:
                version = int(m.group(2))
            elif fqn in header_versions:
                version = header_versions[fqn]
            else:
                raise RuntimeError(f"{rel}: no CEREAL_CLASS_VERSION for {fqn} registered in {cpp.name}")
            matches.append((path, rel, text, fqn, version))
            known_leaves.add(_leaf(fqn))

    enums = _scan_enums(all_headers)
    base_leaves: set[str] = set()
    for path, rel, text, fqn, version in matches:
        leaf = _leaf(fqn)
        body = _find_class_body(text, leaf)
        params, bases, interleaved = (_parse_serializable(body, known_leaves, enums)
                                      if body else ([], [], False))
        base_leaves.update(b["leaf"] for b in bases)
        components[fqn] = {
            "class": leaf,
            "fqn": fqn,
            "leaf": leaf,
            "version": version,
            "base_fqn": COMPONENT_BASE_FQN,
            "immediate_base": bases[0]["leaf"] if bases else None,
            "bases": bases,
            "interleaved_bases": interleaved,
            "header": rel,
            "params": params,
        }
        by_leaf.setdefault(leaf, []).append(fqn)
    bases_table = _scan_bases(base_leaves, all_headers)

    # GameObject 型は ENGINE_REGISTER_COMPONENT されないので、ここで固定せず
    # ヘッダから CEREAL_CLASS_VERSION を読む - 古いバージョンだと全ファイルの
    # ルートオブジェクトを黙って誤記述してしまう。
    declared_versions: dict[str, int] = {}
    for _rel, text in all_headers:
        for vm in RE_CLASS_VERSION.finditer(text):
            declared_versions[vm.group(1)] = int(vm.group(2))
    gameobject_shapes = {}
    for fqn in (
        "NanamiEngine::Scene::SceneGameObject",
        "NanamiEngine::Module::GameObject::PrefabGameObject",
        "NanamiEngine::Scene::CopiedPrefabGameObject",
    ):
        if fqn not in declared_versions:
            raise RuntimeError(
                f"no CEREAL_CLASS_VERSION found for {fqn}; the header scan roots are "
                f"probably wrong"
            )
        gameobject_shapes[fqn] = {"leaf": _leaf(fqn), "version": declared_versions[fqn]}

    return {
        "generated_from": _git_head(),
        "components": dict(sorted(components.items())),
        "by_leaf": {k: sorted(v) for k, v in sorted(by_leaf.items())},
        "bases": dict(sorted(bases_table.items())),
        "gameobject_shapes": gameobject_shapes,
    }


def write_catalog(data: dict[str, Any], path: Path | None = None) -> Path:
    p = path or CATALOG_PATH
    p.write_text(json.dumps(data, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    return p


def _freshness_view(data: dict[str, Any]) -> str:
    # C++ ヘッダから導出したものすべて (`generated_from` は除く。これはファイルを
    # 最後に書いた時点の git HEAD にすぎない)。
    return json.dumps({"components": data.get("components"), "bases": data.get("bases")},
                      sort_keys=True)


def check_fresh() -> tuple[bool, str]:
    if not CATALOG_PATH.exists():
        return False, "catalog.json missing"
    current = _freshness_view(scan())
    on_disk = _freshness_view(json.loads(CATALOG_PATH.read_text(encoding="utf-8")))
    if current != on_disk:
        return False, "catalog.json is stale - run: python -m tools.scene regen-catalog"
    return True, "ok"


if __name__ == "__main__":
    write_catalog(scan())
    print(f"wrote {CATALOG_PATH}")
