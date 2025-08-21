#!/bin/bash

if [ $# -ne 1 ] || [ ! -d "$1" ]; then
        echo "Usage: $0 <directory>"
        exit 1
fi

input_dir="$1"

declare -A parsing_functions
parsing_functions["x265"]="Video Input: Bosphorus 1080p:"
parsing_functions["build-linux-kernel"]="Build: defconfig:"
parsing_functions["ffmpeg"]="Encoder: libx264 - Scenario: Live:"
parsing_functions["stockfish"]="Chess Benchmark:"
parsing_functions["compress-7zip"]=""

parsing_functions["compress-7zip-compress"]="Test: Compression Rating:"
parsing_functions["compress-7zip-decompress"]="Test: Decompression Rating:"

parsing_functions["npb"]="Test / Class: BT.C:"
parsing_functions["rbenchmark"]="Test Results:"
parsing_functions["dav1d"]="Video Input: Summer Nature 4K:"
parsing_functions["stress-ng"]="Test: Pthread:"
parsing_functions["cloverleaf"]="Input: clover_bm:"
parsing_functions["blender"]="Blend File: BMW27 - Compute: CPU-Only:"
parsing_functions["cassandra"]="Test: Writes:"
parsing_functions["clickhouse"]=""
parsing_functions["clickhouse-firstrun"]="100M Rows Hits Dataset, First Run / Cold Cache:"
parsing_functions["clickhouse-secondrun"]="100M Rows Hits Dataset, Second Run:"
parsing_functions["clickhouse-thirdrun"]="100M Rows Hits Dataset, Third Run:"

for benchmark_dir in $input_dir/*/; do
        benchmark_name=$(basename "$benchmark_dir")
        for sched in ext ipanema; do

                current_wd="$benchmark_dir/$sched"
                outfile="${current_wd}/parsed.txt"
                if test "$benchmark_name" = "hackbench"; then
                        cat $current_wd/*.txt | grep "Time:" | cut -d ' ' -f 2 > $outfile
                else

                        if [[ ! -v parsing_functions[$benchmark_name] ]] ; then
                                echo "No parsing function defined for $benchmark_name"
                                continue
                        fi

                        if test "$benchmark_name" = "compress-7zip"; then
                                
                                cmp_key="${benchmark_name}-compress"
                                key="${parsing_functions[$cmp_key]}"
                                cat $current_wd/*.txt | grep "$key" -A 1 | tr -s ' ' | sed -E "s/ (.*)/\1/" | grep -E "^[0-9]" > "${outfile}-compress.txt"
                                cmp_key="${benchmark_name}-decompress"
                                key="${parsing_functions[$cmp_key]}"
                                cat $current_wd/*.txt | grep "$key" -A 1 | tr -s ' ' | sed -E "s/ (.*)/\1/" | grep -E "^[0-9]" > "${outfile}-decompress.txt"
                        elif test "$benchmark_name" = "clickhouse"; then

                                for metric in "firstrun" "secondrun" "thirdrun"; do
                                        cmp_key="${benchmark_name}-${metric}"
                                        key="${parsing_functions[$cmp_key]}"
                                        cat $current_wd/*.txt | grep "$key" -A 1 | tr -s ' ' | sed -E "s/ (.*)/\1/" | grep -E "^([0-9]+\.?[0-9]+)$" > "${outfile}-${metric}.txt"
                                done
                                # cmp_key="${benchmark_name}-firstrun"
                                # key="${parsing_functions[$cmp_key]}"
                                # cat $current_wd/*.txt | grep "$key" -A 1 | tr -s ' ' | sed -E "s/ (.*)/\1/" | grep -E "^[0-9]" > "${outfile}-compress.txt"

                        else
                                key="${parsing_functions[$benchmark_name]}"
                                cat $current_wd/*.txt | grep "$key" -A 1 | tr -s ' ' | sed -E "s/ (.*)/\1/" | grep -E "^[0-9]" > $outfile
                        fi
                        

                fi
        done

done