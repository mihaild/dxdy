https://dxdy.ru/topic129325-105.html

-O2

i | transform q25 | transform q75 | cycle q25 | cycle q75
--| ------------- | ------------- | --------- | ---------
0 | 0.032 | 0.033 | 0.03 | 0.031
5 | 0.037 | 0.038 | 0.037 | 0.039
10 | 0.321 | 0.34 | 0.263 | 0.264
15 | 10.343 | 10.634 | 12.046 | 13.371
20 | 704.098 | 713.75 | 710.217 | 716.984
25 | 24703.6 | 26327.7 | 24827.4 | 25512.1

-Ofast -march=native -mtune=native

i | transform q25 | transform q75 | cycle q25 | cycle q75
--| ------------- | ------------- | --------- | ---------
0 | 0.03 | 0.031 | 0.03 | 0.03
5 | 0.033 | 0.035 | 0.033 | 0.033
10 | 0.117 | 0.117 | 0.114 | 0.116
15 | 9.608 | 9.792 | 9.573 | 9.679
20 | 681.272 | 700.48 | 678.865 | 693.007
25 | 23999.5 | 24532 | 24350.1 | 24863
