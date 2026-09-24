"""``python -m tools.dist <command>`` entry point."""

from __future__ import annotations

import argparse
import json
import sys
import tempfile
from pathlib import Path

_REPO = Path(__file__).resolve().parents[2]
if str(_REPO) not in sys.path:
    sys.path.insert(0, str(_REPO))

from tools.dist import manifest as mf  # noqa: E402
from tools.dist import refs  # noqa: E402
from tools.dist import upload as up  # noqa: E402
from tools.dist.config import CONFIG_PATH, load_config  # noqa: E402

DEFAULT_CACHE_NAME = ".manifest_hash_cache.json"
#: Build Settings の Client Version (BuildConfiguration::ClientVersion)。ゲーム本体に同梱される
CLIENT_VERSION_PATH = Path("ProjectConfig/Build/Runtime/ClientVersion.json")
DEFAULT_CLIENT_VERSION = "1.0.0"
_SAMPLE_LIMIT = 10


def _repo_root(args: argparse.Namespace) -> Path:
    return Path(args.repo_root).resolve() if args.repo_root else _REPO


def _client_version(repo_root: Path) -> str:
    """Build Settings の Client Version。未保存ならエンジンの既定値と同じ 1.0.0"""
    path = repo_root / CLIENT_VERSION_PATH
    if not path.is_file():
        return DEFAULT_CLIENT_VERSION
    value = json.loads(path.read_text(encoding="utf-8-sig")).get("ClientVersion", "")
    return value or DEFAULT_CLIENT_VERSION


def _print_unshipped_refs(unshipped: list[refs.UnshippedRef]) -> None:
    by_owner: dict[str, list[refs.UnshippedRef]] = {}
    for item in unshipped:
        by_owner.setdefault(item.path, []).append(item)
    print(f"ERROR: 配信されないファイルを参照している .efkefc / .mv1 が {len(by_owner)} 件あります。")
    print("       この PC では表示されるが、プレイヤーの PC ではテクスチャ / モデルが見つからない。")
    print("       参照先を Assets/ 内の配信されるコピーに張り替えること (.efkefc は .efkproj と同じフォルダへ")
    print("       コンパイルし直す)。manifest は書き出していません")
    for owner, items in by_owner.items():
        print(f"  {owner}")
        for item in items:
            print(f"    {item.ref}  ({item.reason})")


def _cmd_build(args: argparse.Namespace) -> int:
    repo_root = _repo_root(args)
    assets_root = (repo_root / args.assets_root).resolve()
    out_path = Path(args.out).resolve()

    cache_path = None if args.no_cache else Path(args.cache or (repo_root / DEFAULT_CACHE_NAME))
    cache = mf.HashCache(cache_path)

    result = mf.scan(assets_root, repo_root, cache)
    ref_report = refs.find_unshipped_refs(result, repo_root, assets_root, cache)
    cache.save()

    if ref_report.unshipped:
        _print_unshipped_refs(ref_report.unshipped)
        return 1

    required = args.required_client_version or _client_version(repo_root)
    if not up.VERSION_RE.match(required):
        print(f"ERROR: requiredClientVersion {required!r} は版として使えません")
        return 1
    base_url = args.base_url or load_config(CONFIG_PATH).files_base_url
    document = mf.build(result, args.version, required, base_url)
    mf.dump(document, out_path)

    print(f"scanned  {assets_root.relative_to(repo_root).as_posix()}")
    print(f"  assets      {result.asset_count:>5}  (.meta あり)")
    print(f"  companions  {result.companion_count:>5}  (.meta 無し / 相対参照される随伴ファイル)")
    print(f"  .meta       {len(result.folded_meta):>5}  (本体エントリに畳んだ)")
    print(f"  excluded    {len(result.excluded):>5}  (開発専用)")
    print(f"  hashed      {cache.misses:>5}  (cache hit {cache.hits})")
    print(f"  refs read   {cache.ref_misses:>5}  (cache hit {cache.ref_hits}, .efkefc / .mv1 の参照先)")
    print()
    print(f"manifest {args.version} -> {out_path}")
    print(f"  requiredClientVersion {required}")
    print(f"  entries     {len(result.entries):>5}")
    print(f"  total       {mf.format_bytes(result.total_bytes):>9}")
    print(f"  baseUrl     {base_url or '(未設定: upload できません)'}")

    if result.missing_guid:
        print(f"\nWARNING: .meta はあるが guid を読めなかったファイルが {len(result.missing_guid)} 件あります")
        for path in result.missing_guid[:_SAMPLE_LIMIT]:
            print(f"  {path}")

    if ref_report.unreadable:
        print(f"\nWARNING: 参照先を読み出せなかった .efkefc / .mv1 が {len(ref_report.unreadable)} 件あります "
              "(この PC にしか無いファイルを参照していても検出できません)")
        for path in ref_report.unreadable[:_SAMPLE_LIMIT]:
            print(f"  {path}")

    if result.non_ascii:
        # ダウンロード URL は baseUrl + <hash> なので影響しない。クライアントがローカルへ書くとき、
        # UTF-8 の path をそのまま std::filesystem::path にすると ACP (CP932) 扱いで化ける
        print(f"\nNOTE: 非ASCII のパスが {len(result.non_ascii)} 件あります "
              "(クライアントはローカルへ書くとき UTF-8 → UTF-16 変換が必要)")
        for path in result.non_ascii[:_SAMPLE_LIMIT]:
            print(f"  {path}")

    if result.shift_jis_meta:
        # 配信には影響しない (path はファイルシステムから取るため) が、UTF-8 に統一しておきたい
        print(f"\nNOTE: CP932 のまま残っている .meta が {len(result.shift_jis_meta)} 件あります")
        for path in result.shift_jis_meta[:_SAMPLE_LIMIT]:
            print(f"  {path}")

    return 0


def _cmd_show(args: argparse.Namespace) -> int:
    document = mf.load(Path(args.file))
    entries = mf.entries_of(document)
    assets = [e for e in entries if e.is_asset]

    print(f"schema                {document.get('schema')}")
    print(f"version               {document.get('version')}")
    print(f"requiredClientVersion {document.get('requiredClientVersion')}")
    print(f"baseUrl               {document.get('baseUrl') or '(relative)'}")
    print(f"entries               {len(entries)}  (assets {len(assets)} / companions {len(entries) - len(assets)})")
    print(f"total                 {mf.format_bytes(sum(e.total_size for e in entries))}")

    print(f"\nlargest {_SAMPLE_LIMIT}:")
    for entry in sorted(entries, key=lambda e: e.total_size, reverse=True)[:_SAMPLE_LIMIT]:
        print(f"  {mf.format_bytes(entry.total_size):>9}  {entry.path}")
    return 0


def _cmd_diff(args: argparse.Namespace) -> int:
    installed = mf.load(Path(args.installed))
    remote = mf.load(Path(args.remote))
    result = mf.diff(installed, remote)

    print(f"{installed.get('version')} -> {remote.get('version')}")
    if result.is_up_to_date:
        print("  更新なし")
        return 0

    print(f"  新規 {len(result.added)} / 変更 {len(result.changed)} / 削除 {len(result.removed_paths)}"
          f" / ダウンロード {mf.format_bytes(result.download_bytes)}")

    shown = 0
    for entry in result.added:
        if shown >= _SAMPLE_LIMIT:
            break
        shown += 1
        print(f"  + {entry.path} ({mf.format_bytes(entry.total_size)})")
    for entry in result.changed:
        if shown >= _SAMPLE_LIMIT:
            break
        shown += 1
        print(f"  ~ {entry.path} ({mf.format_bytes(entry.total_size)})")
    for path in result.removed_paths:
        if shown >= _SAMPLE_LIMIT:
            break
        shown += 1
        print(f"  - {path}")
    return 0


def _print_problems(problems: list[str]) -> None:
    if not problems:
        return
    print(f"\nERROR: 検証に失敗したファイルが {len(problems)} 件あります")
    for problem in problems[:_SAMPLE_LIMIT]:
        print(f"  {problem}")


def _cmd_upload(args: argparse.Namespace) -> int:
    config = load_config(CONFIG_PATH)
    repo_root = _repo_root(args)
    manifest_path = Path(args.manifest).resolve()
    remote = args.remote or config.remote
    if not remote:
        print("ERROR: 配信先 (remote) がありません。dist_config.json か --remote で指定してください")
        return 1

    document = mf.load(manifest_path)
    version = document.get("version", "")
    if not up.VERSION_RE.match(version):
        print(f"ERROR: version {version!r} はファイル名に使えません")
        return 1

    base_url = document.get("baseUrl", "")
    if not base_url.endswith("/files/"):
        print(f"ERROR: baseUrl が files/ を指していません: {base_url!r}")
        print("       クライアントは baseUrl + <hash> で取りに来るので、build し直してください")
        return 1
    if remote == config.remote and config.files_base_url and base_url != config.files_base_url:
        print(f"WARNING: baseUrl ({base_url}) が dist_config.json の公開 URL ({config.files_base_url}) と違います")

    rclone = up.Rclone(config.rclone, remote)
    blobs = up.blobs_of(document, repo_root)
    versioned = f"manifest-{version}.json"
    try:
        if rclone.versioned_conflicts(manifest_path, versioned):
            print(f"ERROR: {versioned} はすでに別の内容で存在します。--version を上げて build し直してください")
            return 1
        live = rclone.read_live_manifest()
        existing = rclone.list_blob_hashes()
    except up.UploadError as error:
        print(f"ERROR: {error}")
        return 1

    if live is not None:
        locked = up.locked_file_changes(live, document)
        required = document.get("requiredClientVersion", "")
        if locked and not up.is_newer_version(required, live.get("requiredClientVersion", "")):
            print(f"ERROR: 公開中の {live.get('version')} から、ゲームの実行中は差し替えられないファイルが変わります。")
            print("       フォントは起動時に Windows へ登録されて終了まで外れないので、タイトル画面の更新では置き換えられず、")
            print("       全員の適用が失敗し続けます。新しい zip を配ったうえで、--required-client-version を")
            print(f"       公開中の {live.get('requiredClientVersion')!r} より上げて build し直してください")
            for path in locked[:_SAMPLE_LIMIT]:
                print(f"  {path}")
            return 1

    todo = up.plan(blobs, existing)
    print(f"manifest {version}  ({manifest_path})")
    print(f"remote   {remote}")
    print(f"  blobs referenced   {len(blobs):>5}")
    print(f"  already on remote  {len(blobs) - len(todo):>5}")
    print(f"  to upload          {len(todo):>5}  ({mf.format_bytes(sum(b.size for b in todo))})")

    if args.dry_run:
        problems = up.verify(todo)
        _print_problems(problems)
        print("\n(dry-run: 何も上げていません)")
        return 1 if problems else 0

    try:
        with tempfile.TemporaryDirectory(prefix="nanami-dist-") as staging:
            problems = up.stage(todo, Path(staging))
            if problems:
                _print_problems(problems)
                print("\n中止しました (何も上げていません)")
                return 1
            if todo:
                print()
                rclone.upload_blobs(Path(staging))

        # マニフェストが参照するものが全部そろってからでないと、manifest.json は置かない
        missing = set(blobs) - rclone.list_blob_hashes()
        if missing:
            print(f"\nERROR: 上げたあとも remote に無いブロブが {len(missing)} 件あります。manifest は置いていません")
            return 1

        if rclone.upload_file_once(manifest_path, versioned, up.BLOB_CACHE_CONTROL):
            print(f"\n  uploaded  {versioned}")
        else:
            print(f"\n  unchanged {versioned} (同じ内容がすでにある)")
        if args.no_release:
            print("  --no-release のため manifest.json は差し替えていません")
            return 0

        rclone.upload_file(manifest_path, "manifest.json", up.MANIFEST_CACHE_CONTROL)
        print(f"  released  manifest.json -> {version}")
    except up.UploadError as error:
        print(f"\nERROR: {error}")
        return 1

    if remote == config.remote and config.manifest_url:
        print(f"\n  {config.manifest_url}")
    return 0


def _cmd_selftest(_args: argparse.Namespace) -> int:
    from tools.dist.selftest import main as selftest_main
    return selftest_main()


def build_parser() -> argparse.ArgumentParser:
    p = argparse.ArgumentParser(prog="tools.dist", description=__doc__)
    sub = p.add_subparsers(dest="command", required=True)

    sp = sub.add_parser("build", help="Assets/ を走査して manifest.json を書き出す")
    sp.add_argument("--version", required=True, help="このリリースのバージョン (例 1.1.0)")
    sp.add_argument("--required-client-version", default="",
                    help="これ未満のクライアントを弾く。省略時は Build Settings の Client Version "
                         "(ProjectConfig/Build/Runtime/ClientVersion.json、無ければ 1.0.0)")
    sp.add_argument("--base-url", default="",
                    help="クライアントが baseUrl + <hash> で取りに来る URL。既定は dist_config.json の公開 URL + files/")
    sp.add_argument("--assets-root", default="Assets", help="走査するルート (既定 Assets)")
    sp.add_argument("--out", default="manifest.json", help="出力先 (既定 manifest.json)")
    sp.add_argument("--repo-root", default="", help="リポジトリルート (既定 このファイルから自動判定)")
    sp.add_argument("--cache", default="", help=f"ハッシュキャッシュの場所 (既定 <repo>/{DEFAULT_CACHE_NAME})")
    sp.add_argument("--no-cache", action="store_true", help="キャッシュを使わず全ファイルを読み直す")
    sp.set_defaults(func=_cmd_build)

    sp = sub.add_parser("show", help="マニフェストの中身を要約表示する")
    sp.add_argument("file")
    sp.set_defaults(func=_cmd_show)

    sp = sub.add_parser("diff", help="2つのマニフェストの差分 (クライアントが落とす量) を出す")
    sp.add_argument("installed", help="手元側 (installed.json 相当)")
    sp.add_argument("remote", help="配信側 (新しい manifest.json)")
    sp.set_defaults(func=_cmd_diff)

    sp = sub.add_parser("upload", help="参照ファイルを配信先へ上げ、最後に manifest.json を差し替える")
    sp.add_argument("manifest", nargs="?", default="manifest.json", help="build が書いたマニフェスト (既定 manifest.json)")
    sp.add_argument("--remote", default="", help="rclone の remote (既定 dist_config.json の remote)")
    sp.add_argument("--repo-root", default="", help="リポジトリルート (既定 このファイルから自動判定)")
    sp.add_argument("--dry-run", action="store_true", help="何を上げるかの表示と中身の検証だけ行う")
    sp.add_argument("--no-release", action="store_true",
                    help="ファイルと manifest-<version>.json は上げるが manifest.json は差し替えない")
    sp.set_defaults(func=_cmd_upload)

    sp = sub.add_parser("selftest", help="run the correctness gate")
    sp.set_defaults(func=_cmd_selftest)

    return p


def main(argv: list[str] | None = None) -> int:
    args = build_parser().parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
