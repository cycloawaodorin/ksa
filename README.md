# KSA
AviUtl+拡張編集 のためのスクリプト集です．

## インストール
[Releases](https://github.com/cycloawaodorin/ksa/releases) から `ksa.zip` (AviUtl無印用) または `ksa2.au2pkg.zip` (AviUtl ExEdit2用)をダウンロードします．

AviUtl 無印では，zipファイルを展開してできるフォルダを拡張編集の `script` フォルダにコピーします．

### ビルド
`ksa_ext.dll` をビルドするには，`src` に入り，`make` します．
さらに `make release` すると，ビルドした `ksa_ext.dll` を `ksa` へコピーし，`src` 内の生成物を削除します．
Makefile は MSYS2 環境を想定しているため，その他の環境でビルドするには，適宜修正してください．

## Requirements
AviUtl version 1.10, 拡張編集Plugin version 0.92 での使用を想定しています．
バグ，要望等は [Issue](https://github.com/cycloawaodorin/ksa/issues) で受け付けていますが．AviUtl ExEdit2 用に比べて対応優先度は下がります．

## 使い方
各スクリプトの詳細は [USAGE.md](./USAGE.md) を参照ください．

## Contributing
バグ報告等は https://github.com/cycloawaodorin/ksa にて．
