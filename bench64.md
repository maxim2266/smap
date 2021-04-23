### Benchmarks
Tested on Linux Mint 20.1, with Intel© Core™ i5-8500T CPU @ 2.10GHz.

In the tables below `Mops/s` stands for millions of operations per second.

#### _1000 keys:_
key length, in bytes | `smap_set` (Mops/s) | `smap_get` (Mops/s) | `smap_del` (Mops/s)
-------------------- | ------------------- | ------------------- | -------------------
10 | 21.276595 | 62.5 | 29.411764
32 | 20.408163 | 58.823529 | 29.411764
100 | 13.69863 | 45.454545 | 24.999999
316 | 8.695652 | 22.727272 | 13.513513
1000 | 2.710027 | 9.009009 | 6.802721
3162 | 0.826446 | 2.832861 | 2.127659
10000 | 0.231213 | 0.759878 | 0.592066
31623 | 0.069449 | 0.222024 | 0.18018
100000 | 0.02215 | 0.069314 | 0.05708
316228 | 0.006875 | 0.02226 | 0.018729
1000000 | 0.002162 | 0.007031 | 0.006022

#### _10000 keys:_
key length, in bytes | `smap_set` (Mops/s) | `smap_get` (Mops/s) | `smap_del` (Mops/s)
-------------------- | ------------------- | ------------------- | -------------------
10 | 18.761726 | 54.054054 | 27.247956
32 | 21.413276 | 54.945054 | 25.252525
100 | 12.31527 | 35.971223 | 22.47191
316 | 5.85823 | 17.857142 | 12.376237
1000 | 2.135839 | 6.016847 | 4.452359
3162 | 0.741179 | 2.119991 | 1.837559
10000 | 0.227857 | 0.694927 | 0.547105

#### _100000 keys:_
key length, in bytes | `smap_set` (Mops/s) | `smap_get` (Mops/s) | `smap_del` (Mops/s)
-------------------- | ------------------- | ------------------- | -------------------
10 | 16.21271 | 49.925112 | 27.99552
32 | 19.751135 | 46.992481 | 27.593818
100 | 10.484378 | 25.726781 | 19.634792
316 | 4.651379 | 11.198208 | 10.314595
1000 | 1.949545 | 4.536793 | 3.500297
3162 | 0.729708 | 1.901863 | 1.50759

#### _1000000 keys:_
key length, in bytes | `smap_set` (Mops/s) | `smap_get` (Mops/s) | `smap_del` (Mops/s)
-------------------- | ------------------- | ------------------- | -------------------
10 | 8.274241 | 19.344604 | 10.067046
32 | 8.008328 | 17.407046 | 9.599232
100 | 6.137868 | 12.578299 | 8.105172
316 | 3.758141 | 6.660672 | 4.953609
1000 | 1.851783 | 3.759469 | 2.747418

#### _10000000 keys:_
key length, in bytes | `smap_set` (Mops/s) | `smap_get` (Mops/s) | `smap_del` (Mops/s)
-------------------- | ------------------- | ------------------- | -------------------
10 | 6.724244 | 14.684007 | 7.662758
32 | 6.692893 | 14.082206 | 7.537709
100 | 5.105872 | 10.449943 | 6.452004