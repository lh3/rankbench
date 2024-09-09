Given a string $`B`$, the rank operation is defined by $`{\rm rank}(c,k)=|\{i<k:B[i]=c\}|`$.
It is essential for string matching.
This repo evaluates three BWT encodings I implemented, in attempt to find a good encoding for [ropebwt3][rb3].
It is **not** developed for end users.
The evaluated encodings:

* FMD: the [fermi][fermi] encoding, developed in 2012
* FMR: the [ropebwt2][ropebwt2] encoding, developed in 2014
* B2B: a new encoding inspired by the two-byte encoding from [block_RLBWT][blockRLBWT].
  I might have overlooked important details in the original implementation, though.

Timing on calculating 10 million ranks (data can be downloaded from [this Zenodo repo][zenodo]):

|Dataset       |Format|Size    |Time    |
|:-------------|:----:|-------:|-------:|
|mtb152        |FMD   | 19.0 M |  1.6 s |
|(M1, Mac)     |FMR   | 18.8 M |  3.0 s |
|              |B2B   | 15.7 M |  0.7 s |
|human100      |FMD   | 10.3 G |  4.7 s |
|(Intel, Linux)|FMR   |  9.5 G | 15.2 s |
|              |B2B   |  9.9 G |  2.9 s |
|CommonBacteria|FMD   | 27.6 G |  8.3 s |
|(Intel, Linux)|FMR   | 27.0 G | 19.8 s |
|              |B2B   | 89.4 G |  4.6 s |

where 1G=$`1024^3`$. Note that for each rank query, we are retrieving the ranks
of all symbols in one go. This is necessary for forward search and prefix
trie/DAWG traversal.

[fermi]: https://github.com/lh3/fermi
[ropebwt2]: https://github.com/lh3/ropebwt2
[blockRLBWT]: https://github.com/saskeli/block_RLBWT
[rb3]: https://github.com/lh3/ropebwt3
[zenodo]: https://zenodo.org/doi/10.5281/zenodo.11533210
