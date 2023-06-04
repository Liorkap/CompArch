#!/bin/bash

./cacheSim ./examples/example1_trace --mem-cyc 100 --bsize 3 --wr-alloc 1 --l1-size 4 --l1-assoc 1 --l1-cyc 1 --l2-size 6 --l2-assoc 0 --l2-cyc 5 >> ./examples/out1

./cacheSim ./examples/example2_trace --mem-cyc 50 --bsize 4 --wr-alloc 1 --l1-size 6 --l1-assoc 1 --l1-cyc 2 --l2-size 8 --l2-assoc 2 --l2-cyc 4 >> ./examples/out2

./cacheSim ./examples/example3_trace --mem-cyc 10 --bsize 2 --wr-alloc 1 --l1-size 4 --l1-assoc 1 --l1-cyc 1 --l2-size 4 --l2-assoc 2 --l2-cyc 5 >> ./examples/out3

diff ./examples/example1_output ./examples/out1
diff ./examples/example2_output ./examples/out2
diff ./examples/example3_output ./examples/out3

