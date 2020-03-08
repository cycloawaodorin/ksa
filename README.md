# KSA
AviUtlの拡張編集のためのスクリプト集です．

## インストール
拡張編集の script フォルダに scripts 以下のファイルをコピーします．
または，script フォルダに，scripts へのシンボリックリンクを作成します．
KSA という名前の scripts へのシンボリックリンクを使うことを推奨します．

### ビルド
ksa_ext.dll をビルドするには，src に入り，`make`します．
さらに `make release` すると，ビルドした ksa_ext.dll を scripts へコピーし，src 内の生成物を削除します．
Makefile は MSYS2 環境を想定しているため，その他の環境でビルドするには，適宜修正してください．

## Requirements
AviUtl version 1.10, 拡張編集Plugin version 0.92 での使用を想定しています．

## 使い方
各スクリプトの詳細は USAGE.md を参照ください．

## Contributing
バグ報告等は https://github.com/cycloawaodorin/ksa にて．
