# shogi-piece-placement

An shogi-piece-layout search engine. Quickly solves unidirectional and bidirectional piece-placing problems.

## Gettins Started

### Prerequisites

- CPU with Intel AVX2 instructions (Haswell or later)
- gcc
- make
- Google Test(optional)

### How to Execute

Clone the repository and Run `make` to create an executable file.

```sh
$ git clone https://github.com/ToshinoriTsuboi/shogi-piece-placement
$ cd shogi-piece-placement
$ make
```

You can search as follows.

```sh
$ ./shogi-piece-placement.out 'P18L4N4S4G4K2R2B2'
G1LLLLP1G/1R7/P1PSSSP1G/7R1/K1PSPPP1P/3N1N3/K1P1P1P1P/3P1P2N/G1PBPBP1N b - 1
```

### Search Settings

You can specify the set of pieces you want to search as follows.

```sh
S45                  #=> 45 Silvers
+R9                  #=> 9 Dragons
P18L4N4S4G4K2R2B2    #=> Standard 40 pieces
```

The alphabet for the pieces is as follows.

```
P : Pawn   (歩; Fu)
L : Lance  (香; Kyo)
N : Knight (桂: Kei)
S : Silver (銀: Gin)
G : Gold   (金: Kin)
K : King   (玉: Gyoku)
R : Rook   (飛; Hisha)
B : Bishop (角; Kaku)
X : Stone  (virtual piece)
Q : Queen
```

- Backward-looking pieces are lower case
- `+` represents the promoted piece

You can specify search methods with the following command line arguments

```
-a:            Explore all the up and down flips of the pieces.
-n node_limit: To set an upper limit for the number of search nodes
--:            Read the pieces to be placed from the standard input, not from the argument
```

## License

This project is licensed under the GPLv3 - see the [LICENSE.txt](LICENSE.txt) file for details.

(For referring to [Apery](https://github.com/HiraokaTakuya/apery) in PieceType and Bitboard)