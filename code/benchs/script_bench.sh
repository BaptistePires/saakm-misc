#!/bin/bash
#
nr=10

outdir=$1
cmd='phoronix-test-suite run compress-7zip FORCE_TIMES_TO_RUN=1 < /home/baptiste/saakm-benchs/benchs/input.txt'
cmd_hb='hackbench -l 15000 -g 50'
cmd_build='phoronix-test-suite run build-linux-kernel FORCE_TIMES_TO_RUN=1 < /home/baptiste/saakm-benchs/benchs/input_build.txt'
cmd_x265='phoronix-test-suite run x265 FORCE_TIMES_TO_RUN=1 < /home/baptiste/saakm-benchs/benchs/input_build.txt'
cmd_stockfish='phoronix-test-suite run stockfish FORCE_TIMES_TO_RUN=1 < /home/baptiste/saakm-benchs/benchs/input.txt'
cmd_memcached='phoronix-test-suite run memcached FORCE_TIMES_TO_RUN=1 < /home/baptiste/saakm-benchs/benchs/input_memcached.txt'
cmd_dragonflydb='phoronix-test-suite run dragonflydb FORCE_TIMES_TO_RUN=1 < /home/baptiste/saakm-benchs/benchs/input_dragonfly.txt'



hostname=$(hostname)

if [[ $hostname == *"debian"* ]]; then
	scx=~/scxstart
	saakm=~/ipastart
else
	scx=/home/baptiste/bins/scxstart
	saakm=/home/baptiste/bins/ipastart
fi




bench_list="compress-7zip build-linux-kernel"



if [[ $1 == "scx" ]]; then
	sched=$scx
elif [[ $1 == "saakm" ]]; then
	sched=$saakm
else
	echo "Error: First parameter must be 'scx' or 'saakm'"
	exit 1
fi


sched_str=$1



for bench in $bench_list; do
	install_cmd="phoronix-test-suite install ${bench}"
	echo "Installing benchmark: ${bench}"
	$install_cmd
	mkdir -p ${bench}/{saakm,scx}
done
base_path=$(pwd)

echo "Benchmark setup information:"
echo "Number of runs: $nr"
echo "Output directory: $outdir"
echo "Scheduler command: $sched"
echo "Benchmarks to run: $bench_list"
echo "Hostname: $hostname"
echo "Scheduler string: $sched_str"


for i in $(seq 1 $nr); do

	for bench in $bench_list; do

		cmd="phoronix-test-suite run ${bench} FORCE_TIMES_TO_RUN=1 < $(pwd)/input_${bench}.txt"
		sysctl vm.drop_caches=3
		file_name=$bench/$sched_str/$i
		($sched 0 $cmd) | tee $file_name

		echo "done $i/$nr"
	done
	
done
