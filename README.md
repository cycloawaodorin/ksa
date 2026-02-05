# KSA
AviUtl+拡張編集 / AviUtl ExEdit2 のためのスクリプト集です．

## インストール
[Releases](https://github.com/cycloawaodorin/ksa/releases) から `ksa.zip` (AviUtl無印用) または `ksa2.au2pkg.zip` (AviUtl ExEdit2用)をダウンロードします．

AviUtl 無印では，zipファイルを展開してできるフォルダを拡張編集の `script` フォルダにコピーします．

AviUtl ExEdit2 では，zipファイルをプレビュー画面にドラッグ&ドロップし，インストールを許可します．

### ビルド
`ksa_ext.dll` をビルドするには，`src` に入り，`make` します．
さらに `make release` すると，ビルドした `ksa_ext.dll` を `ksa` へコピーし，`src` 内の生成物を削除します．
`ksa_ext.mod2` をビルドするには，`src2` に入り，`make release` します．
Makefile は MSYS2 環境を想定しているため，その他の環境でビルドするには，適宜修正してください．

## Requirements
無印版は，AviUtl version 1.10, 拡張編集Plugin version 0.92 での使用を想定しています．
AviUtl ExEdit2 版は，本体の更新に伴い，うまく動かなくなる可能性があります．
最新版で動かない場合は，[Issue](https://github.com/cycloawaodorin/ksa/issues) を立てるなどでご連絡ください．

## 使い方
各スクリプトの詳細は USAGE.md を参照ください．

## Contributing
バグ報告等は https://github.com/cycloawaodorin/ksa にて．
