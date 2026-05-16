# Changelog

All notable changes to **farman** are documented in this file.

形式は [Keep a Changelog](https://keepachangelog.com/ja/1.1.0/)、
バージョンは [Semantic Versioning](https://semver.org/lang/ja/) に従う。

リリース毎のコミット / PR 単位の詳細は [GitHub Releases](https://github.com/ms-haraki/farman/releases)
を参照 (`release.yml` の `generate_release_notes` が前回タグからの差分を
自動でリリースノートにまとめる)。本ファイルは「ユーザーから見える主な
変更点」だけを要約する役割を担う。

## [Unreleased]

最初の正式リリース **v1.0.0** に向けて開発中。

### Added

- **UI / 操作**
  - 2 ペイン / シングルペインの UI 切替、ペイン間の同期ブラウズ
  - キーボード駆動の主要操作 (`c`/`m`/`k`/`d`/`r`/`n`/`a`/`f`/`p`/`u` 等)
  - 任意のキーバインドを完全カスタマイズ可能 + プリセット
- **ファイル操作**
  - コピー / 移動 / 削除 (ゴミ箱 or 完全削除) / リネーム / 一括リネーム
    (テンプレート + 連番 + 正規表現置換 + プレビュー)
  - 進捗ダイアログ / キャンセル対応 / 上書きモード (Ask / 自動上書き / 自動リネーム)
- **アーカイブ**
  - 作成・展開 (zip / tar / tar.gz / tar.bz2 / tar.xz)
  - 暗号化 zip の展開 (パスワード入力 + 検証付き)
  - **アーカイブ内ブラウジング**: 仮想 FS (`archive.zip!/inner/`) として閲覧、
    選択ファイルだけを反対ペインへ抽出コピー
- **検索 / ナビゲーション**
  - バックグラウンド再帰検索、結果からの直接ジャンプ
  - ブックマーク / ディレクトリ履歴 (永続化)
  - アドレスバー + パス補完
- **ディレクトリ比較**
  - 左右ペインの差分を行ごとに色分け表示
  - 同期ブラウズと併用可能、コピー後も比較モード維持
- **ビュアー (組み込み)**
  - テキスト (エンコード自動判定 / 行番号 / ワードラップ)
  - 画像 (ズーム / Fit to Window / 透明背景 Checker / GIF・WebP アニメーション)
  - バイナリ (16 進ダンプ + アドレス列・文字列列カラーリング)
  - 任意ビュアーで開く (Ctrl+Enter) / OS 既定アプリで実行 (Shift+Enter)
- **外観 / カスタマイズ**
  - ライト / ダークテーマ、テーマプリセット
  - アドレスバー・カーソル・カテゴリ別ファイル色・行高 の細かい外観設定
- **その他**
  - ログペイン (日次ローテーション + 保持日数設定)
  - 国際化 (英語 / 日本語、Auto は OS 設定追従)
  - 外部変更の自動反映 (QFileSystemWatcher + デバウンス)
- **CI / 配布**
  - 3 OS の自動ビルド (`build.yml`、macOS arm64 / Linux x86_64 / Windows x86_64)
  - タグ push → GitHub Releases 自動公開 (`release.yml`、draft 公開で安全運用)

### Security

- アーカイブ展開時の **Zip Slip 攻撃**を多層防御で拒否:
  - `..` セグメント / 絶対パス / Windows backslash 経由の脱出をエントリ
    名段階・展開先パス組立段階・libarchive write_disk 段階で検査
  - libarchive の `ARCHIVE_EXTRACT_SECURE_SYMLINKS` を有効化
  - 出力ディレクトリの上位 symlink (macOS `/tmp` → `/private/tmp` 等) を
    `QFileInfo::canonicalFilePath()` で実体に解決してから libarchive に渡す
- ディレクトリのコピー / 移動先がコピー元自身 / 配下のとき拒否 (canonical
  path 比較による再帰展開バグの防止)
- 壊れたアーカイブの部分読み込みを「正常」扱いせず、致命エラーを通知して
  停止する

[Unreleased]: https://github.com/ms-haraki/farman/commits/main
