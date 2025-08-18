#!/bin/bash

source utils.sh

schedstart_bin="schedstart"
NR_RUNS=50
FILENAME_NR_SHIFT=0

check_if_schedstart_exists $schedstart_bin

if test $# -ne 3; then 
        usage
fi

scheduler_framework=$1
benchmark_csv_file=$2
output_dir=$3

if [ ! -d "$output_dir" ]; then
    mkdir -p "$output_dir"
    if [ $? -ne 0 ]; then
        echo "Error: Could not create output directory ${output_dir}."
        exit 1
    fi
fi

if test $scheduler_framework != "ext" && test $scheduler_framework != "ipanema"; then
        echo "Wrong shceduler framework: ${scheduler_framework}. Must be ext or ipanema."
        exit 1
fi

if test ! -f $benchmark_csv_file; then
        echo "Error: Benchmark CSV file ${benchmark_csv_file} does not exist."
        exit 1
fi

install_benchmarks "./pts-benchmarks"

log_file="${output_dir}/logs.txt"

echo "@$NR_RUNS;$scheduler_framework;$benchmark_csv_file;$(date)" >> $log_file

while IFS=';' read name command input_file; do
        benchmark_ouput_dir="${output_dir}/${name}/${scheduler_framework}"

        mkdir -p "$benchmark_ouput_dir"
        if [ $? -ne 0 ]; then
            echo "Error: Could not create benchmark output directory ${benchmark_ouput_dir}."
            exit 1
        fi

        for i in $(seq 1 $NR_RUNS); do
                output_file="${benchmark_ouput_dir}/$((i + FILENAME_NR_SHIFT)).txt"

                clear_caches
                $schedstart_bin $scheduler_framework 0 $command < $input_file | tee $output_file

                echo "Run $i/$NR_RUNS"
        done
done < $benchmark_csv_file



