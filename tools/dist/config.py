"""tools/dist の設定 (``dist_config.json``)。

配信先 (rclone の remote とバケット) と、プレイヤーが読みに来る公開 URL をここに置き、
コードにはハードコードしない。
"""

from __future__ import annotations

import json
from dataclasses import dataclass
from pathlib import Path

CONFIG_PATH = Path(__file__).resolve().parent / "dist_config.json"


@dataclass(frozen=True)
class DistConfig:
    remote: str
    public_base_url: str
    rclone: str

    @property
    def files_base_url(self) -> str:
        """manifest.json の ``baseUrl``。クライアントは ``baseUrl + <hash>`` で本体を取りに来る。"""
        if not self.public_base_url:
            return ""
        return self.public_base_url.rstrip("/") + "/files/"

    @property
    def manifest_url(self) -> str:
        if not self.public_base_url:
            return ""
        return self.public_base_url.rstrip("/") + "/manifest.json"


def load_config(path: Path) -> DistConfig:
    data = json.loads(path.read_text(encoding="utf-8")) if path.exists() else {}
    return DistConfig(
        remote=data.get("remote", ""),
        public_base_url=data.get("publicBaseUrl", ""),
        rclone=data.get("rclone", "rclone"),
    )
