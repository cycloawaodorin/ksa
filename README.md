# KSA
AviUtlの拡張編集のためのスクリプト集です．

## インストール
拡張編集の script フォルダに scripts 以下のファイルをコピーします．
または，script フォルダに，scripts へのシンボリックリンクを作成します．

## Requirements
AviUtl version 1.00, 拡張編集Plugin version 0.92 でのみ動作確認しています．

## 使い方
### @ksa.obj
カスタムオブジェクト用のスクリプトです．

#### 折れ線矢印
本家の「ライン(移動軌跡)」のように長さを時間変化させるための折れ線矢印です．長さは折れ線全体の長さに対するパーセンテージを「描画率」のパラメータで調整します．「ライン(移動軌跡)」が鏃の中心を軸にしているのに対し，「折れ線矢印」は鏃の先端を軸にしているため，特定の座標を指したいときに使いやすいです．代わりに，折れる途中の描画が手抜きで汚いです．

<dl>
 <dt>線幅</dt>
  <dd>線の幅をピクセル数で指定します．</dd>
 <dt>鏃長</dt>
  <dd>鏃の長さを線幅に対するパーセンテージで指定します．</dd>
 <dt>鏃幅</dt>
  <dd>鏃の幅を線幅に対するパーセンテージで指定します．</dd>
 <dt>描画率</dt>
  <dd>折れ線全体に対して描画するパーセンテージを指定します．特定の頂点までのパーセンテージは別途計算の必要があります．</dd>
 <dt>色</dt>
  <dd>線の色を指定します．</dd>
 <dt>頂点数</dt>
  <dd>頂点数を2～16の範囲で指定します．頂点はアンカーポイントになっており，ドラッグで動かせます．</dd>
 <dt>座標</dt>
  <dd><code>{頂点1のx, 頂点1のy, 頂点2のx, ...}</code>の形で座標を指定します．アンカーポイントで動かす場合は気にしなくて大丈夫です．</dd>
</dl>

#### 矩形枠
「角度」が`0`の場合に，各辺がx軸またはy軸に平行な長方形の枠です．

<dl>
 <dt>線幅</dt>
  <dd>線の幅をピクセル数で指定します．</dd>
 <dt>角丸</dt>
  <dd>1なら角を丸くし，0なら尖らせます．丸みの半径は線幅の半分で固定です．</dd>
 <dt>アンカー数</dt>
  <dd>2なら対角線上の2頂点を，1なら1頂点をアンカーポイントとします．角度を変える場合は1にするとアンカーポイントと描画頂点がずれません．</dd>
 <dt>色</dt>
  <dd>線の色を指定します．</dd>
 <dt>座標</dt>
  <dd>アンカーポイントの座標が出力されます．数字を指定して頂点位置を決めてもよいです．</dd>
</dl>

#### 枠付き角丸四角形
角を丸められ，枠と塗りつぶし領域で独立に色や透明度を調整できる長方形オブジェクトです．

<dl>
 <dt>角半径</dt>
  <dd>角を丸くする場合の外径半径をピクセル数で指定します．長方形の短辺の長さより大きく指定しても，短辺の長さになります．`0`なら尖ります．</dd>
 <dt>枠太さ</dt>
  <dd>枠線の太さをピクセル数で指定します．長方形の短辺の長さより大きく指定すると，全体が枠になります．`0`なら枠なしです．</dd>
 <dt>内透明度</dt>
  <dd>塗りつぶし領域の透明度を指定します．0: 完全不透明，100: 完全透明．</dd>
 <dt>アンカー数</dt>
  <dd>2なら対角線上の2頂点を，1なら1頂点をアンカーポイントとします．角度を変える場合は1にするとアンカーポイントと描画頂点がずれません．</dd>
 <dt>内色</dt>
  <dd>塗りつぶしの色を指定します．</dd>
 <dt>枠色</dt>
  <dd>枠線の色を指定します．</dd>
 <dt>枠透明度<dt>
  <dd>枠線の透明度を0～100の数値で指定します．0: 完全不透明，100: 完全透明．</dd>
 <dt>座標</dt>
  <dd>アンカーポイントの座標が出力されます．アンカーポイントは外径を決定します．</dd>
</dl>

### @ksa.anm
アニメーション効果用のスクリプトです．

#### 透明度グラデーション
高透明度側と低透明度側の透明度を任意に指定できる透明度グラデーションです．
「斜めクリッピング」の「ぼかし」でグラデーションをかけ，透明度を調整して2回描画することで実現しています．

<dl>
 <dt>中心X，中心Y</dt>
  <dd>グラデーションの斜め線の中心点の座標です．</dd>
 <dt>角度</dt>
  <dd>グラデーションの斜め線の角度です．</dd>
 <dt>幅</dt>
  <dd>「斜めクリッピング」の「ぼかし」パラメータです．</dd>
 <dt>開始透明度，終了透明度</dt>
  <dd>グラデーション前後の透明度を指定します．透明度を逆にするのと「角度」を180ずらすのは同じ効果です．</dd>
</dl>

## Contributing
バグ報告等は https://github.com/cycloawaodorin/ksa にて．
