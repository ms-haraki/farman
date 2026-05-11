# アーカイブ内ブラウジング 設計メモ

SPEC.md「アーカイブ内ブラウジング」(SPEC.md:412〜) の実装に向けた設計案。
libarchive で `.zip` / `.tar` / `.tar.gz` / `.tar.bz2` / `.tar.xz` を **読み取り専用の仮想 FS** として
ファイルリストにそのまま表示する。

---

## 設計方針

### 仮想モードの実現方式

候補:

- **A. `VirtualArchiveModel` を `FileListModel` から派生 / 兄弟クラスとして用意**
- **B. `FileListModel` 自身に「アーカイブモード」フラグを持たせる**

**採用は B**。理由:

- `FileListPane` / `FileListView` / `FileListDelegate` / ソート・即時フィルタ・選択・
  カラム表示 ON/OFF はそのまま流用できる
- 1 ペインの操作中に通常 FS → アーカイブ内 → 通常 FS を行き来するので、モデルを
  付け替えるのは Pane 側のロジックが複雑になる
- 仮想モード固有の振る舞いは `data() / setPath() / refresh()` に集約でき、
  Pane / View 側の変更は最小限

### パス表記

仮想 FS への入口は **`!` 区切り**（SPEC.md:435 で規定）:

```
/abs/path/to/archive.zip                       ← 通常 FS のパス（アーカイブ自体）
/abs/path/to/archive.zip!/                     ← アーカイブのルート
/abs/path/to/archive.zip!/inner/dir/           ← アーカイブ内のサブディレクトリ
~/x/archive.zip!/inner/dir/file.txt            ← アーカイブ内のファイル
```

- `!` は通常のパスに使われない区切り文字
- `ArchivePath` ユーティリティで `(archivePath, innerPath)` への分解 / 再合成を統一
- `QFileSystemWatcher` / Bookmark / History は文字列パスを通すだけなので、
  Bookmark に `~/x.zip!/inner` を保存しても、後で開けば動く（フェーズ D で検証）

---

## クラス構成（新規 / 変更）

### 新規

- `src/utils/ArchivePath.{h,cpp}`
  - `splitArchivePath(QString)` → `{archivePath, innerPath, isArchive}`
  - `joinArchivePath(archivePath, innerPath)` → `"<archive>!/<inner>"`
  - `isArchiveExtension(path)` … 既存 `FileManagerPanel.cpp:1620〜` の拡張子リストを集約

- `src/core/ArchiveEntry.h`
  - 1 エントリのメタデータ。POD。
  ```cpp
  struct ArchiveEntry {
    QString   pathInArchive;   // "foo/bar.txt" (アーカイブ相対)
    QString   name;            // "bar.txt"
    bool      isDir;
    qint64    size;            // 圧縮前。-1 (dir)
    qint64    compressedSize;  // -1 (取得不可)
    QDateTime mtime;
    QString   permissions;     // "rwxr-xr-x" 形式 or 空
    QString   linkTarget;      // シンボリックリンクの場合
  };
  ```

- `src/core/ArchiveContext.{h,cpp}` (`shared_ptr` で共有)
  - 1 アーカイブにつき 1 インスタンス。中身は libarchive のメタデータキャッシュ。
  ```cpp
  class ArchiveContext {
  public:
    QString                    archivePath;          // 元 zip の絶対パス
    QHash<QString, ArchiveEntry> entries;            // pathInArchive → entry
    QStringList                 childrenOf(QString innerDir);  // ディレクトリ列挙
    QDateTime                   archiveMtime;        // 元 zip の更新時刻 (再ロード判定用)
  };
  ```

- `src/core/workers/ArchiveListWorker.{h,cpp}`
  - libarchive で全エントリのメタデータだけ列挙 (本体データは読まない)。
  - 出力: `shared_ptr<ArchiveContext>`。
  - 進捗: エントリ件数を逐次 (巨大アーカイブ向け)。

- `src/core/workers/ArchiveSingleExtractWorker.{h,cpp}`
  - 既存 `ArchiveExtractWorker` は「全件」しか扱えない。これを継承するか共通化し、
    「指定エントリ集合だけを target ディレクトリへ書き出す」モードを追加。
  - 既存ワーカーは「アーカイブ全展開ダイアログ (`u` キー)」専用として残す。

### 既存への変更

- **`src/core/FileItem.{h,cpp}`**
  - 2 つのコンストラクタを共存させる:
    1. `FileItem(const QFileInfo&)` … 通常 FS のエントリ（現状）
    2. `FileItem(const ArchiveEntry&, std::shared_ptr<ArchiveContext>)` … アーカイブ内
  - メソッドは仮想化せず、内部で if 分岐:
    ```cpp
    qint64 FileItem::size() const {
      if (m_archiveEntry) return m_archiveEntry->size;
      return m_info.size();
    }
    ```
  - `isInArchive() const` を追加（外部から判定できるよう）。

- **`src/model/FileListModel.{h,cpp}`**
  - メンバ追加:
    ```cpp
    std::shared_ptr<ArchiveContext> m_archiveContext;  // 有効ならアーカイブモード
    QString                          m_archiveInnerPath; // "/" or "/inner/dir"
    ```
  - `setPath(QString path)` の入口で `ArchivePath::splitArchivePath()` を呼んで分岐:
    - アーカイブパス検出 → `m_archiveContext` を作成 (or 流用) → 内部エントリで `m_allEntries` を構築
    - 通常パス → 既存処理（`m_archiveContext = nullptr` にリセット）
  - `data(BackgroundRole)` で archive モードのときに「アーカイブ内であること」を示す
    薄い tint をかけるかは検討（最初は無し、フェーズ D の磨きで）。
  - `refresh()` はアーカイブモードのとき:
    - `archiveMtime` をチェックして変わっていなければ no-op
    - 変わっていれば `ArchiveListWorker` を再実行（フェーズ C 以降）

- **`src/ui/FileListPane`**
  - アドレスバーの表示で `!` を含むパスを正しく出す。読取専用ラベルは
    そのまま、編集モードは将来の補完対応まで保留。

- **`src/ui/FileManagerPanel.cpp`**
  - `handleEnterKey()`:
    - カレントがディレクトリ → 既存処理
    - カレントがアーカイブ拡張子のファイル & **通常 FS にいる** → `navigatePane(active, "<archive>!/")`
    - カレントが「アーカイブ内のディレクトリ」 → `navigatePane(active, "<archive>!/<inner-dir>")`
    - カレントが「アーカイブ内のファイル」 → 一時展開してビュアー
  - `handleBackspaceKey()`:
    - アーカイブ内のサブディレクトリ → 1 段親 (まだアーカイブ内)
    - アーカイブのルート → 通常 FS の「アーカイブを置いていた親ディレクトリ」へ、
      カーソルはアーカイブファイル名に合わせる
  - **コピー (`c`)**:
    - srcPane がアーカイブモード → 新規 `ArchiveSingleExtractWorker` で
      対象エントリだけを反対パネル currentPath へ書き出す
    - dstPane がアーカイブモード → 「Read-only archive」警告して中止
  - **書込系コマンド** (`m` 移動 / `d` 削除 / `k` mkdir / `n` 新規 / `r` リネーム / `a` 属性):
    - srcPane or dstPane がアーカイブモード → 警告して中止
    - 判定ヘルパ `requireWritablePane(pane)` を導入し、各コマンドの冒頭で呼ぶ

---

## ファイルを開く（Enter on archive entry）

```
[Enter on file inside archive]
  ↓
QtConcurrent::run(ArchiveSingleExtractWorker, [this entry] → tmpFile)
  + QFutureWatcher with nested QEventLoop  (ViewerPanel::openFile と同じパターン)
  ↓
ViewerPanel::openFile(tmpFile)
  ↓ (close)
unlink(tmpFile)
```

- `QStandardPaths::TempLocation/farman/archive-<hash>/<entry>` に展開
- ViewerPanel が閉じたタイミングで削除（ViewerPanel が一時ファイルパスを覚えて
  closeEvent で QFile::remove）
- アプリ終了時に `farman/archive-*` ディレクトリを掃除

---

## メタ情報の表示

`FileListModel::data()` の各カラム実装で `FileItem` 側のメソッドを呼ぶだけなので、
`FileItem` がアーカイブエントリ由来でも自動で値が返る。

- **Size**: 圧縮前サイズ
- **Modified**: アーカイブエントリの mtime
- **Permissions**: アーカイブが Unix permission を持っていれば表示
- **Owner / Group / Link Target**: アーカイブの場合は空文字
- **圧縮率**: `compressed/size` を **Tooltip** に出す（フェーズ D）

---

## 段階的実装

### フェーズ A — 列挙と表示（MVP）

1. `ArchivePath` ユーティリティ
2. `ArchiveEntry` / `ArchiveContext`
3. `ArchiveListWorker`
4. `FileItem` のアーカイブエントリ対応
5. `FileListModel` の `setPath` 仮想モード分岐
6. Enter で潜る / Backspace で戻る
7. アドレスバーの `!` パス表示
8. 即時フィルタと `f` 検索（自動で動くはず）

ここまでで「.zip を Enter で覗いて、中をブラウズして戻れる」状態。

### フェーズ B — ファイル展開ビュー

9. Enter（アーカイブ内ファイル）で一時展開
10. テンポラリの後始末（ViewerPanel close + 起動時 / 終了時のクリーン）

### フェーズ C — コピーと書込ガード

11. `c` キーでアーカイブ内 → 通常 FS への展開コピー
12. 他コマンド (`m d k n r a`) に書込ガード

### フェーズ D — 磨き

13. 圧縮前後サイズ・圧縮率の Tooltip
14. パスワード付き検出時のエラーダイアログ
15. 巨大アーカイブのバックグラウンドロード進捗
16. Bookmark / History がアーカイブ内パスをラウンドトリップできるか検証

---

## ディレクトリ比較との相互排他

アーカイブ内ブラウジング中はディレクトリ比較を発動できない（詳細は
[directory-comparison.md](directory-comparison.md) 「アーカイブ内ブラウジング
との相互排他」節を参照）。

- 比較モード中にアーカイブを Enter で開くと、`navigatePane()` が走るので
  比較モードが自動解除される
- アーカイブモード中は `View → Compare Directories...` を disabled にする
- `FileListModel` に `isInArchiveMode() const` を公開し、判定窓口を提供する

---

## 制約・非対応（v1.0）

- **パスワード付きアーカイブ**: 非対応。検出時はエラーダイアログ。
- **書き込み**: 一切しない。
- **ネスト**: アーカイブ内のアーカイブを Enter で更に潜るのは未対応。
  通常 FS と同じく Enter は「展開してビュアー」になり、結果として
  バイナリビュアーが開く。
- **7z / rar / iso**: ビルドする libarchive の機能依存。Homebrew / apt 標準で
  動く範囲のみ。
- **大文字小文字**: アーカイブ内エントリ名は大文字小文字区別あり (zip は仕様上)。
  通常 FS と区別が変わるが、フィルタ・ソートの `case sensitive` 設定は
  そのまま流用（仕様上の整合性は許容）。

---

## 主な意思決定の根拠

- **B 案（FileListModel フラグ）採用**: 既存の Pane / View / 即時フィルタ /
  ソート / 列ON/OFF を素直に流用できることが最大のメリット。コードの
  分岐は `setPath` と `data()` の数か所に局所化できる。
- **`shared_ptr<ArchiveContext>`**: 同じ zip を左右両ペインで開いたとき、
  メタデータを 2 回読まずに済む。複数ペイン間でキャッシュ共有。
- **`FileItem` を仮想化しない**: 1 つのクラスに「QFileInfo モード」と
  「アーカイブエントリモード」の 2 状態を内包。クラス階層を増やすより
  読みやすい。
- **書込操作は警告で中止**: SPEC.md:447 に従い、潔く完全 read-only。
