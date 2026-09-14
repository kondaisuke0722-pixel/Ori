# O1–O4（＋O7）の作業ディレクトリ

```
make          ビルド（スタブのままでも通る）
make check    C と Python の記録を diff
make check S=2   ステージ T2 だけ
```

`oracle.py` はプロジェクトの `origami.py` を import する。
同じディレクトリに置くか、`PYTHONPATH` を通すこと。
`origami.py` は **書き換えなくてよい**。

## 誰が何を書くか

| ファイル | 状態 |
|---|---|
| `ori_geom.c` | **あなたが書く。** 11 個の `TODO` |
| `ori_internal.h` | 完成。状態の構造体と上限値 |
| `ori_state.c` | 完成。new/fold/undo/check/dump |
| `test_ori.c` | 完成。記録を出力する |
| `oracle.py` | 完成。同じ記録を Python から出力する |

`make check` が落ちたら、原因は `ori_geom.c` にある。
唯一の例外は `ori_state.c` の `rebuild()` で、重複する点のうち**どれが残るか**を決めている。
線の対を `(i, j>i)` の順に走査して**最初に見つかったものを残す**。
これは `itertools.combinations` と同じ順序で、変えると Python と座標が食い違う。

## 進める順序

| ステージ | 必要なもの | 通ると何が言えるか |
|---|---|---|
| T0 | `ori__canon` `ori__intersect` `ori__inside` | 初期状態が参照実装と一致 |
| T1 | ＋ `ori__line_through` `ori__perp_bisector` `ori_chord` | 原始関数が全部正しい |
| T2 | ＋ `ori_axiom1`〜`4`（`7`） | 公理が全組み合わせで一致。添字の範囲外も検査 |
| T3 | T2 と同じ | 5折りの列を公理経由で再現。`ori_fold`/`ori_undo` が動く |
| T4 | T2 と同じ | 候補数が受け入れ基準に一致 |

T4 の期待値（測定済み）：

```
depth 0 P  4 L 4 | O1   2 O2   4 O3  6 O4  0 O7   2 | union  10
depth 1 P  6 L 5 | O1   6 O2   9 O3 11 O4  0 O7   3 | union  23
depth 2 P  7 L 6 | O1   5 O2  15 O3 17 O4  4 O7  16 | union  40
depth 3 P  9 L 7 | O1  11 O2  27 O3 27 O4 13 O7  37 | union  79
depth 4 P 11 L 8 | O1  20 O2  40 O3 37 O4 15 O7  50 | union 116
depth 5 P 17 L 9 | O1  59 O2 101 O3 50 O4 31 O7 121 | union 264
```

depth 5 の 264 が要件書 §7 の「完全に一致しなければならない」264 である。
O7 を今回書かない場合、union は 10 / 23 / 34 / 67 / 95 / **205** になる。

## 引っかかりやすいところ

1. **負のゼロ。** `intersect` は原点の隅に対して `-0.0` を返す。
   `memcmp` で線や点を比較すると `-0.0` と `+0.0` が別物になり、重複線が state に入って `ori_check` が落ちる。
   同一性は `ori__lkey` / `ori__pkey`（整数キー）で判定すること。ダンプ側は `ori__dump_fix` が処理済み。

2. **`ori_nearest_point` の比較は `<`、`<=` ではない。**
   T1 の `nearest 1` は距離が完全に同値になる問い合わせで、`<=` だと最後の最小値が返り Python と食い違う。

3. **添字の範囲外。** Python は例外を投げ、負の添字を黙って巻き戻す。
   参照実装との diff では検出できない。T2 の `oor` 行がこれを検査する。

4. **`-ffp-contract=off`。** 付けないと clang が `a*b - c*d` を FMA に融合し、下位ビットが変わる。
