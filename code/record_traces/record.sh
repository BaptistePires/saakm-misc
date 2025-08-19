#!/bin/bash

nr_runs=10

if [[ $1 == "scx" ]]; then
        sched="scxstart"
elif   [[ $1  == "saakm" ]]; then
        sched="saakmstart"
else
        echo "scheduler : $1 does not exist"
fi


benchs_pts="compress-7zip build-linux-kernel x265 dav1d ffmpeg dragonflydb"

base_path=$(pwd)
sched_str=$1

for bench in $benchs_pts; do
	install_cmd="phoronix-test-suite install ${bench}"
	echo "Installing benchmark: ${bench}"
	$install_cmd
	mkdir -p ${bench}/{saakm,scx}
done

echo 1 > /proc/sys/kernel/sched_schedstats

for i in $(seq 1 $nr_runs); do

	for bench in $benchs_pts; do

		cmd="phoronix-test-suite run ${bench} FORCE_TIMES_TO_RUN=1 < $(pwd)/input_${bench}.txt"
		sysctl vm.drop_caches=3
		file_name=$bench/$sched_str/$i
		
                (trace-cmd record -e sched:* -o $file_name $cmd) | tee logs

		echo "done $i/$nr"
	done
	
done
