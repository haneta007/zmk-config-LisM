# LisM ZMK作業 引き継ぎ書

更新日: 2026-07-22

## 1. 最初に確認する場所

- GitHubリポジトリ: https://github.com/haneta007/zmk-config-LisM
- 作業ブランチ: `feat/auto-mouse-layer-trackball`
- Pull Request: https://github.com/haneta007/zmk-config-LisM/pull/1
- PR状態: Open、merge可能、未merge
- 引き継ぎ書作成前の実装HEAD: `742d36b7a4a1bce05f650fa9baa815cfae2354b9`
- 最新CI: GitHub Actions `Build ZMK firmware` run #9 成功
- CI URL: https://github.com/haneta007/zmk-config-LisM/actions/runs/29717163234

新PCでの取得手順:

```powershell
git clone https://github.com/haneta007/zmk-config-LisM.git
cd zmk-config-LisM
git switch feat/auto-mouse-layer-trackball
git pull --ff-only
```

## 2. 現在実装済みの内容

### トラックボール操作中のMouse Layer自動切替

- トラックボール入力があると `Mouse Layer` (`layer_1`) を自動で有効化する。
- 最後の入力から1000ms後に自動解除する。
- 右中央トラックボールと左ペリフェラルから届く入力の両方に適用済み。
- 実装場所: `snippets/trackball-central/trackball.overlay`
- 使用機能: `&zip_temp_layer 1 1000`
- Number Layer (`layer_2`) のスクロール変換は既存のまま優先される。
- DYA Studio版では `rmouse` / `lmouse` のランタイム入力プロセッサーが同じ既定値を持ち、解除時間・対象レイヤー・有効/無効を変更できる。

### トラックボールの可変速度

- PAW3222センサー自体は明示可能な最小値 `608 CPI` に固定。
- 小さい動きは608 CPI相当、大きい動きは最大約1000 CPI相当へ滑らかに加速する。
- 1イベントごとのX/Y移動量で毎回判定し、過去の高速状態は保持しない。

| 1イベントの絶対移動量 | 動作 |
|---:|---|
| 0〜2 | 608 CPI相当、加速なし |
| 3〜11 | 608〜1000 CPI相当へ線形加速 |
| 12以上 | 約1000 CPI相当 |

大きく動かしたあとにゆっくり止める場合も、移動量が小さくなるにつれて608 CPI相当へ戻る。トラックボールが惰性で速く回っている間だけ高速になる。

### DYA Studioのトラックボール設定

- 対象ファームウェア: `lism_right_central_trackball_studio.uf2`
- DYA Studioへの接続: USBまたはBluetooth
- `rmouse`: 右ポインターの倍率・向き・Mouse Layer自動切替。既定は1倍、`layer_1`、解除1000ms。
- `lmouse`: 左ポインターの倍率・向き・Mouse Layer自動切替。既定は1倍、`layer_1`、解除1000ms。
- `rscroll`: `layer_2`での右スクロール。既定は1/16倍、Y軸スナップ（縦優先）。
- `lscroll`: `layer_2`での左スクロール。既定は1/16倍、X軸スナップ（横優先）。
- 軸スナップの既定値はしきい値100、タイムアウト1000ms。
- PAW3222の608 CPIと608〜1000 CPI相当の可変加速カーブは固定。DYAのポインター倍率は加速後に適用されるため、1未満の倍率で608 CPI相当より低い実効速度にもできる。
- 設定はランタイム入力プロセッサーに保存される。

主な追加ファイル:

- `snippets/dya-trackball-central/dya-trackball.overlay`
- `snippets/dya-trackball-central/dya-trackball.conf`
- `snippets/dya-trackball-central/snippet.yml`
- `config/west.yml`
- `build.yaml`

主な実装ファイル:

- `src/input_processor_pointer_accel.c`
- `dts/bindings/input_processors/lism,input-processor-pointer-accel.yaml`
- `snippets/trackball-central/trackball.overlay`
- `zephyr/module.yml`
- `CMakeLists.txt`
- `Kconfig`

### キーマップ

- 2026-07-20に作業ブランチの `config/lism.keymap` を `origin/main` と完全一致させた。
- 同期コミット: `742d36b chore: sync keymap with main`
- 現在の `&mt` は `tapping-term-ms = <300>`、`flavor = "balanced"`。
- 現在の `&lt` は `quick-tap-ms = <300>` のみ。
- 履歴上の `903fc91 feat: tune hold tap timings` では別の値に変更しているが、最終状態ではmainの値へ戻っている。

## 3. ファームウェアとビルド

右側がCentral、左側がPeripheralの構成。

- 右中央・通常版: `lism_right_central_trackball.uf2`
- 右中央・ZMK/DYA Studio版: `lism_right_central_trackball_studio.uf2`
- 左ペリフェラル: `lism_left_peripheral_trackball.uf2`

DYA/ZMK Studioを使う場合、右側にはStudio版を使用する。左側は通常のペリフェラル版を使用する。成果物は従来どおり全10種類で、DYAのトラックボール設定は右中央のトラックボールStudio版だけに追加している。

GitHub Actions run #9では、DYA対応追加前の全10ファームウェアのビルドとartifact mergeが成功済み。DYA対応追加後のCIビルドはまだ未実施。新しいビルドを取得する場合は、変更をpushした後のActionsで `firmware` artifactをダウンロードする。

ローカルビルドはリポジトリのREADMEに従い、VS Code Dev ContainersとDocker Desktopを使う。以前のPCでは `west`、Docker、ローカルZMKビルド環境がなかったため、検証はGitHub Actionsを使用した。

## 4. キーマップ可視化・変更受付

### Google Sheets

- シート名: `LisM Keymap Change Requests`
- Spreadsheet ID: `1K6-0Rg583wObXBQfKgfKKWp6wKtMYe510s2taJVQ9Qg`
- URL: https://docs.google.com/spreadsheets/d/1K6-0Rg583wObXBQfKgfKKWp6wKtMYe510s2taJVQ9Qg/edit
- 保存先として指定されたDriveフォルダ: https://drive.google.com/drive/folders/10xTmRcdjmY-06AYHSUXSxAsewVRp74Ib

目的:

- 現行キーマップを物理配置に近い形で見ながら変更内容を入力する。
- 入力欄はプルダウン方式。
- `Keymap Editor` タブにキーボード写真を埋め込み、実キーとの対応を分かりやすくしている。
- 写真上で一番下のキーがない位置は14mmトラックボールとして扱う。

以前の作業で使ったローカル成果物:

- `LisM Keymap Change Requests.xlsx`
- `lism-photo-guide-annotated.png`
- `lism_keymap_change_requests_seed.csv`

これらは旧PCの作業フォルダ直下にあるため、新PCではDriveまたは旧PCからコピーする。

### Notion

- LisMキーマップを人が読める形で可視化したページを作成済み。
- URL: https://app.notion.com/p/381ab3d52d128118a2d3d63231726f32
- 元データは `config/lism.keymap`、`keymap-drawer/lism.yaml`、`keymap-drawer/lism.svg`。

## 5. Git履歴

作業ブランチ上の主要コミット:

```text
742d36b chore: sync keymap with main
f93c3d3 feat: add adaptive trackball acceleration
797d756 feat: set trackball cpi
903fc91 feat: tune hold tap timings
d1cb67f feat: enable auto mouse layer for trackball
```

PR #1は上記変更をまとめて `main` へ取り込むためのもの。現在は未merge。

## 6. 次に行うこと

1. 1秒化とDYA対応のコミットをpushする。
2. 新しいGitHub Actionsで全10ファームウェアのビルド成功を確認する。
3. 新しいartifactから、右側へ `lism_right_central_trackball_studio.uf2`、左側へ `lism_left_peripheral_trackball.uf2` を書き込む。
4. 実機で細かい位置合わせ、大きな移動、停止前の減速、Mouse Layer自動切替を確認する。
5. Number Layerで右が縦優先、左が横優先のスクロールになることと、DYA Studioから4項目を変更・保存できることを確認する。
6. 固定の可変加速を調整する場合は、`accel-start = <2>`、`accel-full = <12>`、`max-cpi = <1000>` を変更する。

## 7. Bluetooth再接続問題について

「スリープ復帰後にLisMが再接続できず、PC再起動で直る」という問題がある。ただし、前回の診断は問題が発生するPCとは別のPCで実行してしまったため、取得したMediaTek/Realtekアダプター情報は原因判断に使用しない。

問題が発生する新PC上で、再発中かつPC再起動前に次を確認する:

- Bluetoothアダプター名、ドライバーバージョン、デバイス状態
- スリープ方式 (`powercfg /a`)
- `System` イベントログのBluetooth、Kernel-PnP、Kernel-Powerイベント
- LisM以外のBLE機器も接続不能か
- Bluetoothサービスまたはアダプター再起動だけで復旧するか

LisMの電源入れ直しでも直らずPC再起動で直るなら、Windows側のBluetoothスタックまたはアダプタードライバーが主な候補。ただし問題PC上で再診断するまで未確定。

## 8. 新しいCodexチャットへの依頼文

新PCで次のように依頼する:

```text
このリポジトリの LisM_HANDOFF_2026-07-21.md を最初に全文読んでください。
現在のブランチ、git status、PR #1、最新GitHub Actionsを再確認し、記載内容より現在の実データを優先してください。
既存の未コミット変更は勝手に戻さず、日本語で進捗を報告してください。
```

旧PC固有のパスや診断結果は新PCへそのまま当てはめず、リポジトリ・GitHub・Driveの現在状態を再確認すること。
