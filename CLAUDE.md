# CLAUDE.md

このファイルは、Claude Code (claude.ai/code) がこのリポジトリで作業する際のガイダンスを提供します。

## プロジェクト概要

Farman は Qt6 ベースの C++ アプリケーションです。ビルドシステムには CMake を使用し、C++20 標準に準拠しています。Qt は macOS の Homebrew 経由で `/opt/homebrew/opt/qt` にインストールされています。

## ビルドシステム

### 前提条件
- CMake 3.21 以上
- Qt6 (Core および Widgets モジュール)
- C++20 対応コンパイラ

### プロジェクトのビルド方法

設定とビルド:
```bash
cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
cmake --build build
```

アプリケーションの実行:
```bash
./build/farman
```

クリーンビルド:
```bash
rm -rf build
cmake -B build -DCMAKE_PREFIX_PATH=/opt/homebrew/opt/qt
cmake --build build
```

## コードアーキテクチャ

詳細は `ARCHITECTURE.md` を参照。仕様は `SPEC.md` を参照。

### アーキテクチャ原則
- **レイヤー分離**: UI / ビジネスロジック / モデル / データアクセス層を分離
- **Model-View パターン**: Qt の Model/View フレームワークを活用
- **非同期処理**: ファイル操作は QThread でバックグラウンド実行
- **プラグインアーキテクチャ**: ビュアーは動的ロード可能なプラグインとして実装

### ディレクトリ構造
```
src/
├── main.cpp
├── ui/          # UI層 (MainWindow, FilePane, etc.)
├── models/      # モデル層 (FileSystemModel, FilterSortProxy)
├── operations/  # ファイル操作 (Copy, Move, Delete)
├── viewers/     # ビュアーシステム (プラグイン対応)
├── settings/    # 設定管理 (Settings, KeyBindings, ColorScheme)
├── core/        # コア機能 (Bookmark, History, Search)
└── utils/       # ユーティリティ
```

### 主要設計パターン
- **Singleton**: `ViewerManager::instance()`, `Settings::instance()`
- **Observer**: Qt Signal/Slot によるイベント通知
- **Strategy**: `FileOperation` 派生クラス (Copy/Move/Delete)
- **Proxy**: `FilterSortProxy` によるフィルタ・ソート機能
- **Plugin**: `IViewerPlugin` インターフェースによる拡張機能

### CMake 設定
- Qt の適切な統合のために `qt_add_executable()` を使用
- CMake の AUTOMOC、AUTORCC、AUTOUIC を有効化し、Qt のメタオブジェクト、リソース、UI の自動コンパイルを実現
- Qt6::Core および Qt6::Widgets にリンク

## 開発ノート

### コードスタイル
- インデント: スペース 2 つ (タブ不使用)
- 文字エンコーディング: UTF-8
- 言語: C++20 標準を強制

### Qt 統合
- AUTOMOC が有効化されており、Q_OBJECT マクロを含むファイルで moc が自動実行される
- AUTORCC が .qrc リソースファイルを自動処理
- AUTOUIC が .ui フォームファイルを自動処理

### Claude Code への指示
- **日本語で出力してください**: このプロジェクトでは、Claude Code からの全ての出力を日本語で行ってください。
