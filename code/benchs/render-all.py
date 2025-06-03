import os
import subprocess
import sys


input_dir=sys.argv[1]
if not os.path.exists(input_dir):
        print(f"Directory {input_dir} does not exists. Exiting...")
        exit(1)


output_dir=sys.argv[2]
if not os.path.exists(output_dir):
        print(f"Directory {output_dir} does not exists. Exiting...")
        exit(1)


benchmarks_info = {
        "x265" : [
                {
                        "title": "x264 Bosphorus 1080p - FPS (higher is better)",
                        "metric" : "Video Input: Bosphorus 1080p:",
                        "filename" : "x264_1080p.png"
                }
        ],
        "dragonflydb" : [
                {
                        "title" : "DragonflyDB Clients per thread 10, set:get 1:1 - QPS (higher is better)",
                        "metric": "Clients Per Thread: 10 - Set To Get Ratio: 1:1:",
                        "filename" : "dragonflydb.png"
                }
        ],
        "compress-7zip" : [
                {
                        "title" : "7zip - Compression (higher is better)",
                        "metric": "Test: Compression Rating:",
                        "filename" : "compress-7zip-compress.png"
                },
                {
                        "title" : "7zip - Decompression (higher is better)",
                        "metric": "Test: Decompression Rating:",
                        "filename" : "compress-7zip-decompress.png"
                }
        ],
        "build-linux-kernel": [
                {
                        "title": "Linux Kernel defconfig - Build Time (lower is better)",
                        "metric": "Build: defconfig:",
                        "filename" : "build-linux-kernel.png"
                }
        ],
        "dav1d": [
                {
                        "title": "Dav1d Summer Nature 4K - FPS (higher is better)",
                        "metric" : "Video Input: Summer Nature 4K:",
                        "filename" : "dav1d_4k.png"
                }
        ],
        "ffmpeg": [
                {
                        "title": "FFmpeg x264, live - FPS (higher is better)",
                        "metric" : "Encoder: libx264 - Scenario: Live:",
                        "filename" : "ffmpeg_x264_live.png"
                }
        ],
        "stockfish": [
                {
                        "title": "Stockfish - Nodes Per Second (higher is better)",
                        "metric" : "Chess Benchmark:",
                        "filename" : "stockfish.png"
                }
        ],
        "memcached": [
                 {
                        "title": "Memcached set:get 1:1 - QPS (higher is better)",
                        "metric": "Set To Get Ratio: 1:1:",
                        "filename" : "memcached.png"
                 }
        ]
}


# Execute the shell command and capture the output
command_output = subprocess.check_output("ls input_*.txt | cut -d _ -f 2 | cut -d . -f 1", shell=True, text=True)
command_output = command_output.strip().split('\n')

for bench in command_output:
        
        if bench not in benchmarks_info: continue
        
        benchmark_plot_data = benchmarks_info[bench]

        perf_dir=f'./data/{bench}/'

        if not os.path.exists(perf_dir):
                print(f"Directory {perf_dir} does not exists. Skipping...")
                continue
        
        saakm_dir = os.path.join(perf_dir, 'saakm')
        scx_dir = os.path.join(perf_dir, 'scx')

        if not os.path.exists(saakm_dir):
                print(f"Directory {saakm_dir} does not exist. Skipping...")
                continue

        if not os.path.exists(scx_dir):
                print(f"Directory {scx_dir} does not exist. Skipping...")
                continue

        cmd=lambda directory, algo, sep: os.system(f'cat {directory}/* | grep "{sep}" -A 1 | grep -E "[0-9]" | tr -s " " | grep -v : > /tmp/{algo}')

        for benchmark in benchmark_plot_data:
                print(benchmark)
                print(benchmark["metric"])
                cmd(saakm_dir, 'saakm', benchmark['metric'])
                cmd(scx_dir, 'scx', benchmark['metric'])

                print(f'python boxplot.py /tmp/saakm /tmp/scx "{benchmark["title"]}" "{output_dir}/{benchmark["filename"]}"')
                os.system(f'python boxplot.py /tmp/saakm /tmp/scx "{benchmark["title"]}" "{output_dir}/{benchmark["filename"]}"')


                os.remove("/tmp/saakm")
                os.remove("/tmp/scx")
