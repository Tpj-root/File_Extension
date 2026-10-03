# File_Extension








### Build & run

```

g++ -std=c++17 -O2 -o tpcodec tpcodec.cpp

# convert your sample
./tpcodec frxXAUUSD_1790922600_2026-10-02.csv frxXAUUSD_1790922600_2026-10-02.tp

# read back
./tpcodec -r frxXAUUSD_1790922600_2026-10-02.tp


```




Using it inside other code


```
#include "tpcodec.cpp"   // or split into .h/.cpp

tp::TpReader r("frxXAUUSD_..._.tp");
r.open();

tp::Tick t;
while (r.next(t)) {
    // t.time  (int64_t, unix seconds)
    // t.price (double)
}

```



```

[ TpHeader (60 bytes, packed) ]
[ int64 first_time ][ int64 first_price_scaled ]
[ varint(Δtime) varint(Δprice) ] * (N-1)

```







### Result

```
when@master:~/Desktop/MY_GIT/File_Extension$ ./tpcodec frxXAUUSD_1790274600_2026-09-25.csv frxXAUUSD_1790274600_2026-09-25.tp
Converted 82421 ticks
  CSV : 1640322 bytes
  TP  : 165669 bytes  (10.0998% of CSV)
  decimals detected : 2
when@master:~/Desktop/MY_GIT/File_Extension$ 



when@master:~/Desktop/MY_GIT/File_Extension$ ls -alh
total 1.8M
drwxrwxr-x  3 when when 4.0K Oct  3 20:02 .
drwxrwxr-x 49 when when 4.0K Oct  3 19:59 ..
-rw-rw-r--  1 when when 1.6M Sep 26 17:17 frxXAUUSD_1790274600_2026-09-25.csv
-rw-rw-r--  1 when when 162K Oct  3 20:02 frxXAUUSD_1790274600_2026-09-25.tp



```




```

when@master:~/Desktop/MY_GIT/File_Extension$ ./tpcodec -r frxXAUUSD_1790274600_2026-09-25.tp 
File     : frxXAUUSD_1790274600_2026-09-25.tp
Version  : 1
Flags    : 1
Count    : 82421
Start    : 1790274600
End      : 1790360999
MinPrice : 4255.04
MaxPrice : 4315.64
Scale    : 100

First 5 ticks:
1790274600,4274.65
1790274601,4274.58
1790274602,4274.68
1790274603,4274.63
1790274604,4274.7
...
Last 5 ticks:
1790360995,4287.02
1790360996,4287.37
1790360997,4287.44
1790360998,4287.35
1790360999,4287.53
when@master:~/Desktop/MY_GIT/File_Extension$ cat frxXAUUSD_1790274600_2026-09-25.csv | wc -l
82422
when@master:~/Desktop/MY_GIT/File_Extension$ 

```