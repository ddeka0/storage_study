# storage_study
Experiments with disk IO and performance measurements

    dd@dd:~/study/storage/experiments$ g++ -std=c++20 main.cpp -DW -o file_rw
    dd@dd:~/study/storage/experiments$ g++ -std=c++20 ext_merge_optimized.cpp -DV2 -lglog -o ext_merge_optimized
    dd@dd:~/study/storage/experiments$ for i in {1..10}; do rm numbers.txt && rm sorted_numbers.txt && ./file_rw && time ./ext_merge_optimized; done
    dd@dd:~/study/storage/experiments$ for i in {1..10}; do rm numbers.txt && rm sorted_numbers.txt && ./file_rw && time ./ext_merge_optimized; done

    real	0m11.213s
    user	0m0.096s
    sys 0m0.043s

    real	0m11.292s
    user	0m0.104s
    sys 0m0.038s

    real	0m11.066s
    user	0m0.097s
    sys 0m0.046s

    real	0m11.127s
    user	0m0.099s
    sys 0m0.041s

    real	0m11.573s
    user	0m0.087s
    sys 0m0.055s

    real	0m10.888s
    user	0m0.098s
    sys 0m0.046s

    real	0m11.078s
    user	0m0.098s
    sys 0m0.050s

    real	0m11.812s
    user	0m0.101s
    sys 0m0.041s

    real	0m11.833s
    user	0m0.090s
    sys 0m0.050s

    real	0m11.309s
    user	0m0.092s
    sys 0m0.051s
    dd@dd:~/study/storage/experiments$ g++ -std=c++20 ext_merge_optimized.cpp -lglog -o ext_merge_optimized
    dd@dd:~/study/storage/experiments$ for i in {1..10}; do rm numbers.txt && rm sorted_numbers.txt && ./file_rw && time ./ext_merge_optimized; done

    real	0m20.488s
    user	0m0.095s
    sys 0m0.075s

    real	0m21.364s
    user	0m0.099s
    sys 0m0.083s

    real	0m21.341s
    user	0m0.122s
    sys 0m0.059s

    real	0m21.311s
    user	0m0.106s
    sys 0m0.074s

    real	0m22.482s
    user	0m0.095s
    sys 0m0.064s

    real	0m22.027s
    user	0m0.111s
    sys 0m0.067s

    real	0m20.105s
    user	0m0.120s
    sys 0m0.054s

    real	0m21.732s
    user	0m0.086s
    sys 0m0.090s

    real	0m20.540s
    user	0m0.118s
    sys 0m0.061s

    real	0m21.633s
    user	0m0.111s
    sys 0m0.053s
