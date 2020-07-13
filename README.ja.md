# shogi-piece-placement

利かずの駒並べ探索エンジン。単方向／双方向の利かずの駒並べ問題を短時間で求解できます。

## スタートガイド

### 必要条件

- Intel AVX2命令が使えるCPU（Haswell以降）
- gcc
- make
- Google Test（optional; 検査用）

### 実行方法

レポジトリをクローンし、`make`を実行すれば実行可能ファイルが作成されます。

```sh
$ git clone https://github.com/ToshinoriTsuboi/shogi-piece-placement
$ cd shogi-piece-placement
$ make
```

以下のようにして駒並べの探索ができます。

```sh
$ echo 'P18L4N4S4G4K2R2B2' | ./shogi-piece-placement.out --
3720
G1LLLLP1G/1R7/P1PSSSP1G/7R1/K1PSPPP1P/3N1N3/K1P1P1P1P/3P1P2N/G1PBPBP1N b - 1
```

### 探索内容の指定

並べたい駒の集合は以下のように指定します。

```sh
S45                  #=> 銀45枚
+R9                  #=> 龍9枚
P18L4N4S4G4K2R2B2    #=> 通常使う40枚
```

駒を表すアルファベットは以下のとおりです。

```
P : 歩
L : 香
N : 桂
S : 銀
G : 金
K : 玉
R : 飛
B : 角
X : 石
Q : クイーン
```

- 後手の駒は小文字
- 成駒は駒名の前に`+`

以下のコマンドライン引数で探索方法を指定できます。

```
-a:            駒の上下反転をすべて全探索する
-n node_limit: 探索ノード数の上限値を設定する
-v:            詳細な探索情報を表示する
--:            ファイルからではなく、標準入力から配置する駒を読む
```

## ライセンス

このプロジェクトはGPLv3の元にライセンスされています。
詳しくは[LICENSE.txt](LICENSE.txt)を参照。

（PieceType、Bitboard関係のコードで[Apery](https://github.com/HiraokaTakuya/apery)を参考にしたため）